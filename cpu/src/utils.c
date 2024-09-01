#include <utils.h>

// ==========================================================================
// ====  Variables globales:  ===============================================
// ==========================================================================

int socket_memoria = 1;
int socket_kernel_dispatch = 1;
int socket_kernel_interrupt = 1;

t_log* log_cpu_oblig; 
t_log* log_cpu_gral; 
t_config* config;

// ==========================================================================
void iniciar_logs(bool testeo)
{
    log_cpu_gral = log_create("cpu_general.log", "CPU", testeo, LOG_LEVEL_DEBUG);
    
    // Log obligatorio
    char * nivel;
    nivel = config_get_string_value(config, "LOG_LEVEL");
    if (strcmp(nivel, "LOG_LEVEL_TRACE")== 0){
        log_cpu_oblig = log_create("cpu_obligatorio.log", "CPU", true, LOG_LEVEL_TRACE);
    } else if (strcmp(nivel, "LOG_LEVEL_DEBUG") == 0){
        log_cpu_oblig = log_create("cpu_obligatorio.log", "CPU", true, LOG_LEVEL_DEBUG);
    } else if (strcmp(nivel, "LOG_LEVEL_INFO") == 0){
        log_cpu_oblig = log_create("cpu_obligatorio.log", "CPU", true, LOG_LEVEL_INFO);
    } else if (strcmp(nivel, "LOG_LEVEL_WARNING") == 0){
        log_cpu_oblig = log_create("cpu_obligatorio.log", "CPU", true, LOG_LEVEL_WARNING);
    } else if (strcmp(nivel, "LOG_LEVEL_ERROR") == 0){
        log_cpu_oblig = log_create("cpu_obligatorio.log", "CPU", true, LOG_LEVEL_ERROR);
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
	liberar_conexion(log_cpu_gral, "Memoria", socket_memoria);
	liberar_conexion(log_cpu_gral, "Kernel del puerto Dispatch", socket_kernel_dispatch);
	liberar_conexion(log_cpu_gral, "Kernel del puerto Interrupt", socket_kernel_interrupt);
	log_destroy(log_cpu_oblig);
	log_destroy(log_cpu_gral);
	config_destroy(config);
}