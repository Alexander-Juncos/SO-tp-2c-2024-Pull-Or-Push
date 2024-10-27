#include "utils.h"

// ==========================================================================
// ====  Variables globales:  ===============================================
// ==========================================================================
t_log* log_fs_oblig;
t_log* log_fs_gral;
t_config* config;

int socket_escucha = 1;

t_file_system* fs;

bool fin_programa = 0;

unsigned int cantidad_indices_max;
void *espacio_bitmap; // para funcionamiento interno bitmap
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
        ftruncate(fs->f_bloques, aux_tamanio); // fijamos su tamaño
    }

    ruta_aux = string_substring_until(ruta_aux,string_length(PATH_BASE)); // me quedo nueva mente con path base

    // Inicializo el bitmap
    aux_tamanio = fs->cant_bloques / 8; // convierte bytes a bits
    if (aux_tamanio % 8 != 0) 
        aux_tamanio++; // compenso si hubo perdida
    espacio_bitmap = malloc(aux_tamanio);
    fs->bitmap = bitarray_create_with_mode(espacio_bitmap, aux_tamanio, LSB_FIRST);

    // Abrir - Crear archivo bitmap.dat
    string_append(&ruta_aux, "bitmap.dat");
    fs->f_bitmap = fopen(ruta_aux, "rb+");
    if (fs->f_bitmap==NULL)
    { // si no existe lo creamos
        fs->f_bitmap = fopen(ruta_aux, "wb+");
        aux_tamanio = bitarray_get_max_bit(fs->bitmap);
        fwrite(&aux_tamanio, sizeof(size_t), 1, fs->f_bitmap); // almacena el tamaño bitmap (para si se abre archivo desp comprobar)
        actualizar_f_bitmap();
    } 
    else 
    { // existe archivo hay que cargar a bitmap // REVISAR NO CREO Q ESTE BIEN
        size_t tam_bitmap_prev; 
        fread(&tam_bitmap_prev, sizeof(size_t),1,fs->f_bitmap);
        // verificar si el tamaño del bitmap en archivo es diferente al bitmap q intentamos crear
        if (tam_bitmap_prev != aux_tamanio){
            fclose(fs->f_bitmap);
            fs->f_bitmap = fopen(ruta_aux, "wb+"); // lo sobreescribimos
            aux_tamanio = bitarray_get_max_bit(fs->bitmap);
            fwrite(&aux_tamanio, sizeof(size_t), 1, fs->f_bitmap); 
            actualizar_f_bitmap();
        } else {
            fgets(fs->bitmap->bitarray, aux_tamanio, fs->f_bitmap); // tomamos el bitmap almacenado
        }
    }

    // setear cantidad de indices que puede tener un bloque de indices
    cantidad_indices_max = fs->tam_bloques / sizeof(uint32_t);

    // liberar lo q sea necesario
    
    return true;
}

// ==========================================================================
// ====  Funciones Auxiliares:  =============================================
// ==========================================================================

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