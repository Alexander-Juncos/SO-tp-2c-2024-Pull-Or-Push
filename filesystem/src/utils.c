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
t_bitmap* bitmap;

bool fin_programa = 0;

unsigned int cantidad_indices_max;
char* PATH_BASE;

// ==========================================================================
// ====  Funciones Internas:  ===============================================
// ==========================================================================

bool iniciar_fs()
{
    imprimir_mensaje("FS en Proceso. Revisar carga de archivo bitmap preexistente");

    // variables
    fs = malloc(sizeof(t_file_system));
    char *ruta_aux = string_new();
    int aux_tamanio;
    int file_desc;

    // Descargar config
    fs->cant_bloques = config_get_int_value(config, "BLOCK_COUNT");
    fs->tam_bloques = config_get_int_value(config, "BLOCK_SIZE");
    PATH_BASE = config_get_string_value(config, "MOUNT_DIR");

    // crear - localizar directorio MOUNT_DIR (de config)
    mkdir(PATH_BASE, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); // En teoria esto deberia crear la carpeta del FS si no existiera
    // si existe da EERROR y no hace nada

    string_append(&ruta_aux, PATH_BASE); // como el directorio ya es valido...

    // Abrir - Crear archivo bloques.dat con tamaño = BLOCK_SIZE * BLOCK_COUNT 
    string_append(&ruta_aux, "bloques.dat");
    aux_tamanio = (fs->tam_bloques * fs->cant_bloques) - 1; // tamaño en bytes 0-->tam_tot-1 

    fs->f_bloques = fopen(ruta_aux, "rb+"); // busca si existe
    if (fs->f_bloques == NULL) // si no existe hay q crearlo
    {
        fs->f_bloques = fopen(ruta_aux, "wb+");
        file_desc = fileno(fs->f_bloques);
        ftruncate(file_desc, aux_tamanio); // fijamos su tamaño
    }

    ruta_aux = string_substring_until(ruta_aux,string_length(PATH_BASE)); // me quedo nueva mente con path base

    // inicio el bitmap
    iniciar_bitmap(ruta_aux);

    // setear cantidad de indices que puede tener un bloque de indices
    cantidad_indices_max = fs->tam_bloques / sizeof(uint32_t);

    // inicio el mutex
    pthread_mutex_init(&mutex_fs, NULL);

    // liberar lo q sea necesario
    free(ruta_aux);
    return true;
}

// ==========================================================================
// ====  Funciones Auxiliares:  =============================================
// ==========================================================================

void iniciar_bitmap(char* ruta)
{
    struct stat stat_buf;
    int file_desc;
    int aux_tamanio = fs->cant_bloques / 8; // convierte bytes a bits
    if (aux_tamanio % 8 != 0) 
        aux_tamanio++; // compenso si hubo perdida
    string_append(&ruta, "bitmap.dat");

    // veo si existe un archivo bitmap
    bitmap->f = fopen(ruta, "rb+");
    if (bitmap->f == NULL)
    { // Si no existe lo creamos
        bitmap->f = fopen(ruta, "wb+");
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
        log_trace(log_fs_gral, "bitmap.dat sobre-escrito - Creando nuevo FS.");
    }
    else {
        log_trace(log_fs_gral, "bitmap.dat coincide con config, existe FS previo - Cargando.");
    }

    // hacemos el mappeo al espacio con el q se creara el bitmap
    bitmap->espacio_bitmap = mmap(NULL, stat_buf.st_size, PROT_WRITE | PROT_READ, MAP_SHARED, file_desc, 0);

    // creamos el bitmap
    bitmap->bitarray = bitarray_create_with_mode(bitmap->espacio_bitmap, aux_tamanio, LSB_FIRST);
    // cuando se crea el bitmap, lo que contuviera el espacio_bitmap no se tendria q modificar... x lo que si se
    // cargo deberia estar bien, y si no existia ftruncate lo inicia con bytes 0

    // Lo imprimimos para testear
    imprimir_bitmap();
}

void actualizar_f_bitmap() // por ahora sincroniza todo el bitmap... podria hacerse de otra forma pero seria + complejo
{
    struct stat stat_buf;
    int file_desc;

    file_desc = fileno(bitmap->f);
    fstat(file_desc, &stat_buf);

    msync(bitmap->espacio_bitmap, stat_buf->st_size, MS_SYNC);
}

void imprimir_bitmap()
{
    for (unsigned int i=0; i < bitarray_get_max_bit(bitmap->bitarray); i++)
    {
        if (bitarray_test_bit(bitmap->bitarray, i)){
            printf("%i: TRUE",i);
        }
        else
        {
            printf("%i: FALSE",i);
        }

        if (i > 1 && i % 20 == 0)
        {
            printf("\n");
        }
    }
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
	liberar_conexion(log_fs_gral, "Servidor Multihilo",socket_escucha); 
	config_destroy(config);
}

void retardo_acceso()
{
    unsigned int tiempo_en_microsegs = config_get_int_value(config, "RETARDO_ACCESO_BLOQUE")*MILISEG_A_MICROSEG;
    usleep(tiempo_en_microsegs);
}