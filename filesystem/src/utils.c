#include "utils.h"

// ==========================================================================
// ====  Variables globales:  ===============================================
// ==========================================================================
t_log* log_fs_oblig;
t_log* log_fs_gral;
t_config* config;

t_file_system* fs;

// ==========================================================================
// ====  Funciones Internas:  ===============================================
// ==========================================================================



// ==========================================================================
// ====  Funciones Auxiliares:  =============================================
// ==========================================================================

void iniciar_logs(bool testeo)
{
    log_fs_gral = log_create("FileSytem_general.log", "FileSytem", testeo, LOG_LEVEL_DEBUG);
    
    // Log obligatorio
    char * nivel;
    nivel = config_get_string_value(config, "LOG_LEVEL");
    if (strcmp(nivel, "LOG_LEVEL_TRACE")== 0){
        log_fs_oblig = log_create("FileSytem_obligatorio.log", "FileSytem", true, LOG_LEVEL_TRACE);
    } else if (strcmp(nivel, "LOG_LEVEL_DEBUG") == 0){
        log_fs_oblig = log_create("FileSytem_obligatorio.log", "FileSytem", true, LOG_LEVEL_DEBUG);
    } else if (strcmp(nivel, "LOG_LEVEL_INFO") == 0){
        log_fs_oblig = log_create("FileSytem_obligatorio.log", "FileSytem", true, LOG_LEVEL_INFO);
    } else if (strcmp(nivel, "LOG_LEVEL_WARNING") == 0){
        log_fs_oblig = log_create("FileSytem_obligatorio.log", "FileSytem", true, LOG_LEVEL_WARNING);
    } else if (strcmp(nivel, "LOG_LEVEL_ERROR") == 0){
        log_fs_oblig = log_create("FileSytem_obligatorio.log", "FileSytem", true, LOG_LEVEL_ERROR);
    } else {
        printf("LOG_LEVEL de config desconocido...")
        /*
            Ver si se quiere manejar caso de que el config este mal () y como cerrar el programa
        */
    }
    free(nivel);		
}

void terminar_programa()
{
	// Y por ultimo, hay que liberar lo que utilizamos (conexion, log y config) 
	// con las funciones de las commons y del TP mencionadas en el enunciado /
	// liberar_conexion(log_io_gral, nombre,socket); 
	config_destroy(config);
}