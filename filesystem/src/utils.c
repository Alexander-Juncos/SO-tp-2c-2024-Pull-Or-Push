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

pthread_mutex_t mutex_socket_cliente_temp;
int socket_cliente_temp = 1;

// ==========================================================================
// ====  Funciones Internas:  ===============================================
// ==========================================================================

bool iniciar_fs()
{
    imprimir_mensaje("Place Holder: iniciar_fs");
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