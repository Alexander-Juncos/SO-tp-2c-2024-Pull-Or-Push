#include "utils.h"

// ==========================================================================
// ====  Variables globales:  ===============================================
// ==========================================================================
t_log* log_fs_oblig;
t_log* log_fs_gral;
t_config* config;

int socket_escucha = 1;

t_file_system* fs;
pthread_mutex_t mutex_fs;
pthread_mutex_t mutex_bitmap;
t_bitmap* bitmap;

bool fin_programa = 0;

unsigned int cantidad_indices_max;
unsigned int ultimo_bloque_revisado = 0;
char* PATH_BASE;

// ==========================================================================
// ====  Funciones Internas:  ===============================================
// ==========================================================================

bool iniciar_fs()
{
    imprimir_mensaje("FS en Proceso. Revisar carga de archivo bitmap preexistente");

    // variables
    fs = malloc(sizeof(t_file_system));
    char *ruta_aux;
    int aux_tamanio;
    int file_desc;

    // Descargar config
    fs->cant_bloques = config_get_int_value(config, "BLOCK_COUNT");
    fs->tam_bloques = config_get_int_value(config, "BLOCK_SIZE");
    PATH_BASE = config_get_string_value(config, "MOUNT_DIR");

    // crear - localizar directorio MOUNT_DIR (de config)
    mkdir(PATH_BASE, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); // En teoria esto deberia crear la carpeta del FS si no existiera
    // si existe da EERROR y no hace nada

    // Abrir - Crear archivo bloques.dat con tamaño = BLOCK_SIZE * BLOCK_COUNT 
    ruta_aux = obtener_path_absoluto("bloques.dat");
    aux_tamanio = (fs->tam_bloques * fs->cant_bloques) - 1; // tamaño en bytes 0-->tam_tot-1 

    log_debug(log_fs_gral, "ruta bloques: %s", ruta_aux);
    fs->f_bloques = fopen(ruta_aux, "rb+"); // busca si existe
    if (fs->f_bloques == NULL) // si no existe hay q crearlo
    {
        log_debug(log_fs_gral,"Creando archivo bloques");
        fs->f_bloques = fopen(ruta_aux, "wb+");
        file_desc = fileno(fs->f_bloques);
        ftruncate(file_desc, aux_tamanio); // fijamos su tamaño
    }

    // inicio el bitmap
    iniciar_bitmap();

    // setear cantidad de indices que puede tener un bloque de indices
    cantidad_indices_max = fs->tam_bloques / sizeof(uint32_t);

    // inicio el mutex
    pthread_mutex_init(&mutex_fs, NULL);

    // liberar lo q sea necesario
    free(ruta_aux);
    return true;
}

bool memory_dump(char* ruta, int size, void* data)
{
    // preparo para buscar bloques
    t_list* lista_bloques;
    unsigned int cant_bloques = size / fs->tam_bloques;
    if (size % fs->tam_bloques != 0)
        cant_bloques++;

    // obtengo la ruta absoluta
    char* ruta_absoluta = obtener_path_absoluto(ruta);

    // busco bloques libres
    pthread_mutex_lock(&mutex_bitmap);
    lista_bloques = bloques_libres(cant_bloques);
    pthread_mutex_unlock(&mutex_bitmap);

    // si la lista no esta iniciada es xq no hay bloques suficientes - ABORTAR
    if (lista_bloques == NULL)
    {
        log_error(log_fs_gral, "ERROR en la Creacion de archivo: \"%s\" - No hay bloques suficientes - cantidad requerida: %i", ruta, cant_bloques);
        free(ruta_absoluta);
        free(ruta);
        return false;
    }

    // marco como ocupados los bloques
    pthread_mutex_lock(&mutex_bitmap);
    marcar_bloques_libres(lista_bloques, ruta);
    pthread_mutex_unlock(&mutex_bitmap);

    // Crear el archivo de metadata (config) contiene un bloque de indices y su size en bytes

    // imprimir en el archivo bloques.dat la data bloque a bloque y x cada bloque agregar su referencia en el bloque de indices

    // cerrar archivo metadata 


    free(ruta_absoluta);
    free(ruta);
    return true;
}

// ==========================================================================
// ====  Funciones Externas:  ===============================================
// ==========================================================================

bool rutina_memory_dump(t_list* param)
{
    char* nombre = string_new();
    int tamanio;
    int pid;
    int tid;
    int size;
    char* timestamp;
    void* data;

    data = list_get(param, 0);
    pid = *(int*) data;
    data = list_get(param, 1);
    tid = *(int*) data;
    data = list_get(param, 2);
    timestamp = data;
    data = list_get(param, 3);
    size = *(int*) data;
    data = list_get(param, 4); // la informacion en el espacio usuario del proceso

    // formo el nombre para el archivo // <PID>-<TID>-<TIMESTAMP>.dmp
    string_append(&nombre, string_itoa(pid));
    string_append(&nombre, "-");
    string_append(&nombre, string_itoa(tid));
    string_append(&nombre, "-");
    string_append(&nombre, timestamp);
    string_append(&nombre, ".dmp");
    
    return memory_dump(nombre, size, data);
}

// ==========================================================================
// ====  Funciones Auxiliares:  =============================================
// ==========================================================================

void iniciar_bitmap()
{
    bitmap = malloc(sizeof(t_bitmap));

    struct stat stat_buf;
    int file_desc;
    int aux_tamanio = fs->cant_bloques / 8; // convierte bytes a bits
    if (aux_tamanio % 8 != 0) 
        aux_tamanio++; // compenso si hubo perdida
    char * ruta = obtener_path_absoluto("bitmap.dat");

    // veo si existe un archivo bitmap
    log_debug(log_fs_gral, "ruta bitmap: %s", ruta);
    bitmap->f = fopen(ruta, "rb+");
    if (bitmap->f == NULL)
    { // Si no existe lo creamos
        log_debug(log_fs_gral,"Creando archivo bitmap");
        bitmap->f = fopen(ruta, "wb+");
        file_desc = fileno(bitmap->f);
        ftruncate(file_desc, aux_tamanio);
    }
    file_desc = fileno(bitmap->f);
    
    // verificamos si el tamaño del bitmap guardado coincide con el que necesita el programa
    fstat(file_desc, &stat_buf);
    log_debug(log_fs_gral, "Tamaño bitmap.dat: %d - tamaño bitmap calculado: %d", stat_buf.st_size , aux_tamanio);
    if (stat_buf.st_size != aux_tamanio)
    { // si no coinciden
        fclose(bitmap->f);

        // lo sobreescribo
        bitmap->f = fopen(ruta, "wb+");
        file_desc = fileno(bitmap->f);
        fstat(file_desc, &stat_buf);
        ftruncate(file_desc, aux_tamanio);
        log_debug(log_fs_gral, "bitmap.dat sobre-escrito - Creando nuevo FS.");
    }
    else {
        log_debug(log_fs_gral, "bitmap.dat coincide con el esperado por config - Cargando.");
    }

    // hacemos el mappeo al espacio con el q se creara el bitmap
    bitmap->espacio_bitmap = mmap(NULL, stat_buf.st_size, PROT_WRITE | PROT_READ, MAP_SHARED, file_desc, 0);

    // creamos el bitmap
    bitmap->bitarray = bitarray_create_with_mode(bitmap->espacio_bitmap, aux_tamanio, LSB_FIRST);
    // cuando se crea el bitmap, lo que contuviera el espacio_bitmap no se tendria q modificar... x lo que si se
    // cargo deberia estar bien, y si no existia ftruncate lo inicia con bytes 0

    // inicio su mutex
    pthread_mutex_init(&mutex_bitmap, NULL);

    // Lo imprimimos para testear
    imprimir_bitmap();
    contar_bloques_libres_totales();
}

void contar_bloques_libres_totales()
{
    unsigned int inicio_busqueda = 0;
    unsigned int bloque_actual = inicio_busqueda;
    unsigned int cantidad;
    
    do
    {
        if (bloque_actual > fs->cant_bloques){
            bloque_actual = 0;
        }

        if (!(bitarray_test_bit(bitmap->bitarray, bloque_actual)))
            cantidad++;

        bloque_actual++;
    } while (inicio_busqueda != bloque_actual);

    bitmap->bloques_libres_tot = cantidad;
}

void actualizar_f_bitmap() // por ahora sincroniza todo el bitmap... podria hacerse de otra forma pero seria + complejo
{
    struct stat stat_buf;
    int file_desc;

    file_desc = fileno(bitmap->f);
    fstat(file_desc, &stat_buf);

    // esto es opcional pero puede funcionar como seguro
    contar_bloques_libres_totales();

    msync(bitmap->espacio_bitmap, stat_buf.st_size, MS_SYNC);

}

t_list* bloques_libres (unsigned int cantidad)
{
    t_bloques_libres* bloqs_lib;
    unsigned int inicio_busqueda = ultimo_bloque_revisado;
    t_list* lista = list_create();
    
    do
    {
        ultimo_bloque_revisado++;

        if (ultimo_bloque_revisado > fs->cant_bloques){
            ultimo_bloque_revisado = 0;
        }

        if (bitarray_test_bit(bitmap->bitarray, ultimo_bloque_revisado))
        { // el bloque esta ocupado, guardo lo q conte y limpio
            // si la estructura no esta limpia hay q cargarla y limpiarla
            if (bloqs_lib != NULL)
            {
                list_add(lista, bloqs_lib);
                bloqs_lib = NULL;
            }
        }
        else // el bloque esta libre, lo agrego
        { // Si el bloque esta libre lo cargo en estructura
            cantidad--;
            // anido xq la vida es una
            if(bloqs_lib == NULL)
            {
                bloqs_lib = malloc(sizeof(t_bloques_libres));
                bloqs_lib->bloque = ultimo_bloque_revisado;
                bloqs_lib->cant_bloques = 1;
            }
            else // ya habia bloques libres previos (continuos)
            {
                bloqs_lib->cant_bloques++;
            } 
        }
    } while (inicio_busqueda != ultimo_bloque_revisado && cantidad > 0);

    if (cantidad == 0) // Si encontro todos los bloques q necesitaba
    {
        if (bloqs_lib != NULL)
        {
            list_add(lista, bloqs_lib);
            bloqs_lib = NULL;
        }
        return lista;
    }
    
    // Si no lo encontro hay q limpiar y devolver NULL
    list_destroy_and_destroy_elements(lista, free);
    if (bloqs_lib != NULL)
        free(bloqs_lib);
    return NULL;    
}

void marcar_bloques_libres(t_list* lista, char* archivo)
{
    t_bloques_libres* bloques;
    unsigned int contador;

    for (unsigned int i=0; i<list_size(lista); i++)
    {
        bloques = list_get(lista, i);
        contador = 0;

        while (contador != bloques->cant_bloques)
        {
            bitarray_set_bit(bitmap->bitarray, bloques->bloque + contador);
            bitmap->bloques_libres_tot--;
            log_info(log_fs_oblig, "## Bloque asignado: %d - Archivo: \"%s\" - Bloques Libres: %d", 
                                    bloques->bloque + contador, archivo, bitmap->bloques_libres_tot);
        }
    }
}

void imprimir_bitmap()
{
    for (unsigned int i=0; i < bitarray_get_max_bit(bitmap->bitarray); i++)
    {
        if (bitarray_test_bit(bitmap->bitarray, i)){
            printf("%i: T ",i);
        }
        else
        {
            printf("%i: F ",i);
        }

        if (i > 1 && i % 20 == 0)
        {
            printf("\n");
        }
    }
    printf("\n");
}

void iniciar_logs(bool testeo)
{
    log_fs_gral = log_create("fs_general.log", "FS", testeo, LOG_LEVEL_DEBUG);
    
    // Log obligatorio
    char * nivel;
    nivel = config_get_string_value(config, "LOG_LEVEL");
    log_fs_oblig = log_create("fs_obligatorio.log", "FS", true, log_level_from_string(nivel));

    /*
        Ver luego si se quiere manejar caso de que el config este mal () y como cerrar el programa.
    */

    free(nivel);		
}

void terminar_programa()
{
	// Y por ultimo, hay que liberar lo que utilizamos (conexion, log y config) 
	// con las funciones de las commons y del TP mencionadas en el enunciado /
    fclose(fs->f_bloques);
    bitarray_destroy(bitmap->bitarray);
    fclose(bitmap->f);
    free(bitmap);
    free(fs);
	liberar_conexion(log_fs_gral, "Servidor Multihilo",socket_escucha); 
	// config_destroy(config); comento para q no rompa al liberar el nivel de log
}

void retardo_acceso()
{
    unsigned int tiempo_en_microsegs = config_get_int_value(config, "RETARDO_ACCESO_BLOQUE")*MILISEG_A_MICROSEG;
    usleep(tiempo_en_microsegs);
}

char *obtener_path_absoluto(char *ruta){
    char *aux = string_new();
    string_append(&aux, PATH_BASE);
    string_append(&aux, "/");
    string_append(&aux, ruta);
    return aux;
}