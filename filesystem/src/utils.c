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

    // crear - localizar directorio MOUNT_DIR/files (de config)
    ruta_aux = obtener_path_absoluto("files");
    mkdir(ruta_aux, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); // En teoria esto deberia crear la carpeta del FS si no existiera
    free(ruta_aux);

    // Abrir - Crear archivo bloques.dat con tamaño = BLOCK_SIZE * BLOCK_COUNT 
    ruta_aux = obtener_path_absoluto("bloques.dat");
    aux_tamanio = (fs->tam_bloques * fs->cant_bloques) ; // tamaño en bytes tam_tot

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
    FILE* f_metadata;
    unsigned int bloque_indice;
    t_config* metadata;
    char* aux_config;

    // preparo para buscar bloques
    unsigned int* vector_bloques;
    unsigned int cant_bloques = size / fs->tam_bloques;
    if (size % fs->tam_bloques != 0)
        cant_bloques++;
    
    
    // verifico si la cant_bloques es valida, si no va a entrar en un unico bloque de indices => ERROR
    if (cant_bloques > cantidad_indices_max)
    {
        log_error(log_fs_gral, "cantidad de bloques necesaria [%d] - supera el numero maximo de indices posible [%d].",
                    cant_bloques, cantidad_indices_max);
        return false;
    }

    cant_bloques++; // agrego el bloque de indices

    // obtengo la ruta absoluta
    char* ruta_absoluta = obtener_path_absoluto_metadata(ruta);

    // busco bloques libres
    pthread_mutex_lock(&mutex_bitmap);
    vector_bloques = bloques_libres(cant_bloques);
    // pthread_mutex_unlock(&mutex_bitmap);       // amplio la ZC para mayor seguridad

    // si la lista no esta iniciada es xq no hay bloques suficientes - ABORTAR
    if (vector_bloques == NULL)
    {
        pthread_mutex_unlock(&mutex_bitmap);  // si fallo libero la ZC
        log_error(log_fs_gral, "ERROR en la Creacion de archivo: \"%s\" - No hay bloques suficientes - cantidad requerida: %i",
                                ruta, cant_bloques);
        free(ruta_absoluta);
        return false;
    }

    // marco como ocupados los bloques
    // pthread_mutex_lock(&mutex_bitmap);
    marcar_bloques_libres(vector_bloques, cant_bloques, ruta);
    pthread_mutex_unlock(&mutex_bitmap);

    // obtengo el bloque indice
    bloque_indice = *vector_bloques;

    // Crear el archivo de metadata (config) contiene un bloque de indices y su size en bytes
    f_metadata = fopen (ruta_absoluta, "w"); // crea archivo de texto (revisar si path no debe modificarse antes)
    fclose(f_metadata);

    metadata = config_create(ruta_absoluta);    

    // para evitar memory leaks
    aux_config = string_itoa(size); 
    config_set_value(metadata, "SIZE", aux_config);
    free(aux_config);
    aux_config = string_itoa(bloque_indice);
    config_set_value(metadata, "INDEX_BLOCK", aux_config);
    free(aux_config);
    config_save(metadata);

    // LOG OBLIGATORIO - Creación de Archivo
    log_info(log_fs_oblig, "## Archivo Creado: %s - Tamaño: %d", ruta, size);

    // LOG OBLIGATORIO - ACCESO A BLOQUE INDICE
    log_info(log_fs_oblig,"## Acceso Bloque - Archivo: %s - Tipo Bloque: ÍNDICE - Bloque File System %d",
                                ruta, bloque_indice);

    pthread_mutex_lock(&mutex_fs);
    // Agrego al bloque_indice los bloques libres
    fseek(fs->f_bloques, bloque_indice * fs->tam_bloques, SEEK_SET);

    // escribo los indices de los bloques
    fwrite((vector_bloques+1), sizeof(uint32_t), cant_bloques-1, fs->f_bloques);
    pthread_mutex_unlock(&mutex_fs);
    log_debug(log_fs_gral, "Indices escritos en bloque indice - bloque indice: %d", bloque_indice);

    // REVISAR *******************************************************************************************    
    // Como ya estan los indices en el bloque indice, ya puedo escribir
    pthread_mutex_lock(&mutex_fs);
    escribir_bloques(ruta, bloque_indice, data, cant_bloques);
    pthread_mutex_unlock(&mutex_fs);
    
    // cerrar archivo metadata y libero estructuras
    config_destroy(metadata);
    free(ruta_absoluta);
    free(vector_bloques);

    // LOG OBLIGATORIO
    log_info(log_fs_oblig,"## Fin de solicitud - Archivo: %s", ruta);

    return true;
}

void escribir_bloques(char* nombre, unsigned int bloque_indice, void* data, unsigned int cant_bloques)
{
    void* ptr_data;
    cant_bloques--; // resto el bloque de indices

    // tomo todo el bloque de indices
    uint32_t* bloque = malloc(fs->tam_bloques); // es lo mismo que sizeof(uint32_t)*cantidad_indices_max
    
    fseek(fs->f_bloques, bloque_indice * fs->tam_bloques, SEEK_SET);

    // LOG OBLIGATORIO - ACCESO A BLOQUE INDICE
    log_info(log_fs_oblig,"## Acceso Bloque - Archivo: %s - Tipo Bloque: ÍNDICE - Bloque File System %u",
                                nombre, bloque_indice);

    fread((void*)bloque, fs->tam_bloques, 1, fs->f_bloques); // cargo TODO el bloque indices (un unico acceso)

    for (int i=0; i<cant_bloques; i++)
    {
        ptr_data = (data + fs->tam_bloques * i);

        // LOG OBLIGATORIO - ACCESO A BLOQUE
        log_info(log_fs_oblig,"## Acceso Bloque - Archivo: %s - Tipo Bloque: DATOS - Bloque File System %u",
                                nombre, *(unsigned int*)(bloque + i));

        // *(uint32_t*)(bloque + i * sizeof(uint32_t)) => permite obtener cada indice
        fseek(fs->f_bloques, (*(uint32_t*)(bloque + i)) * fs->tam_bloques, SEEK_SET);
        fwrite(ptr_data,fs->tam_bloques, 1, fs->f_bloques);
    }

    // limpiar flag fin de lectura (x las dudas)
    // clearerr(fs->f_bloques);
    /* 
        Ya que si al leer el bloque indice x alguna razon estuviera en el ultimo bloque, al leer el ultimo indice
        el flag de EOF no permitiria realizar ninguna otra lectura...
    */
    free(bloque);
}

// ==========================================================================
// ====  Funciones Externas:  ===============================================
// ==========================================================================

bool rutina_memory_dump(t_list* param)
{
    char* nombre = string_new();
    char* aux_itoa; // para evitar memory leaks
    int pid;
    int tid;
    int size;
    char* timestamp;
    void* data;
    bool resultado;

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
    aux_itoa = string_itoa(pid);
    string_append(&nombre, aux_itoa);
    free(aux_itoa);
    string_append(&nombre, "-");
    aux_itoa = string_itoa(tid);
    string_append(&nombre, aux_itoa);
    free(aux_itoa);
    string_append(&nombre, "-");
    string_append(&nombre, timestamp);
    string_append(&nombre, ".dmp");

    // agrego un log
    log_debug(log_fs_gral, "Pedido rutina memory_dump >> Nombre: %s - Tamaño: %d", nombre, size);
    
    // realizo el memory_dump
    resultado = memory_dump(nombre, size, data);

    free(nombre);
    // no se libera timestamp ni data xq pertenecen a lista

    return resultado;
}

// ==========================================================================
// ====  Funciones Auxiliares:  =============================================
// ==========================================================================

void iniciar_bitmap()
{
    bitmap = malloc(sizeof(t_bitmap));

    struct stat stat_buf; // para obtener datos del archivo bitmap si existiera
    int file_desc;
    int aux_tamanio = fs->cant_bloques / 8; // convierte bytes a bits
    if (fs->cant_bloques % 8 != 0) 
        aux_tamanio++; // compenso si hubo perdida
    char * ruta = obtener_path_absoluto("bitmap.dat");

    // log_debug(log_fs_gral, "Tamaño real: %d - Esperado: %d - resto: %f", fs->cant_bloques / 8, aux_tamanio);

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
        log_debug(log_fs_gral, "bitmap.dat sobre-escrito - Creado nuevo FS.");
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
    // imprimir_bitmap();
    contar_bloques_libres_totales();

    free(ruta);
}

void contar_bloques_libres_totales() {
    unsigned int bloque_actual = 0;  // Inicia en el primer bloque
    unsigned int cantidad = 0;

    // Recorrer todos los bloques y contar los libres
    for (bloque_actual = 0; bloque_actual < fs->cant_bloques; bloque_actual++) {
        if (!bitarray_test_bit(bitmap->bitarray, bloque_actual)) {
            cantidad++;
        }
    }

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
    // imprimir_bitmap();

    msync(bitmap->espacio_bitmap, stat_buf.st_size, MS_SYNC);
}

unsigned int* bloques_libres (unsigned int cantidad)
{
    unsigned int* bloques;
    unsigned int contador = 0;

    if (cantidad > bitmap->bloques_libres_tot) 
    {
        log_error(log_fs_gral, "Cantidad de bloques libres FS: %d - es menor a la cantidad requerida: %d",
                                bitmap->bloques_libres_tot, cantidad);
        return NULL;
    }

    bloques = malloc(sizeof(unsigned int) * cantidad); // reservo un vector de [cantidad] int
    
    // reviso el bitmap desde la posición 0 (mas simple)
    for (unsigned int i=0; i < fs->cant_bloques; i++)
    {
        if (bitarray_test_bit(bitmap->bitarray, i) == false) // bloque libre
        {
            *(bloques + contador) = i; // escribo en bloques[contador] el indice del bloque libre
            contador++;
        }

        // si se obtienen los bloques necesarios retorno;
        if (contador == cantidad)
        {
            return bloques;
        }
    }

    // nunca deberia llegar a este punto
    log_error(log_fs_gral, "ERROR: no encontro los bloques libres al revisar todo el bitmap, esto no deberia pasar");
    free(bloques);
    return NULL;
}

void marcar_bloques_libres(unsigned int* v_bloques, unsigned int cant_bloques, char* archivo)
{
    unsigned int bloque;
    unsigned int bloques_libres_iniciales = bitmap->bloques_libres_tot;

    log_debug(log_fs_gral, "Bloques libres actuales: %d", bitmap->bloques_libres_tot);

    for (unsigned int i=0; i < cant_bloques; i++)
    {
        bloque = *(v_bloques + i);
        bitarray_set_bit(bitmap->bitarray, bloque);
        bitmap->bloques_libres_tot--;

        // LOG OBLIGATORIO
        log_info(log_fs_oblig, "## Bloque asignado: %d - Archivo: \"%s\" - Bloques Libres: %d", 
                                bloque, archivo, bitmap->bloques_libres_tot);
    }

    log_debug(log_fs_gral, "Se marcaron %d bloques - bloques libres: [%d]>>[%d]",
                            cant_bloques, bloques_libres_iniciales, bitmap->bloques_libres_tot);
    actualizar_f_bitmap();
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
    pthread_mutex_destroy(&mutex_bitmap);
    pthread_mutex_destroy(&mutex_fs);
    fclose(fs->f_bloques);
    bitarray_destroy(bitmap->bitarray);
    fclose(bitmap->f);
    free(bitmap->espacio_bitmap);
    free(bitmap);
    free(fs);
    free(PATH_BASE);
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

char *obtener_path_absoluto_metadata(char *ruta){
    char *aux = string_new();
    string_append(&aux, PATH_BASE);
    string_append(&aux, "/files/");
    string_append(&aux, ruta);
    return aux;
}