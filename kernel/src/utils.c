#include <utils.h>

// ==========================================================================
// ====  Variables globales:  ===============================================
// ==========================================================================

int socket_cpu_dispatch = 1;
int socket_cpu_interrupt = 1;

t_config *config = NULL;
int quantum_de_config;

t_log* log_kernel_oblig = NULL;
t_log* log_kernel_gral = NULL;

// ==========================================================================
// ====  Funciones Utils:  ==================================================
// ==========================================================================

void iniciar_logs(bool testeo)
{
    log_kernel_gral = log_create("kernel_general.log", "Kernel", testeo, LOG_LEVEL_DEBUG);
    
    // Log obligatorio
    char * nivel;
    nivel = config_get_string_value(config, "LOG_LEVEL");
    if (strcmp(nivel, "LOG_LEVEL_TRACE")== 0){
        log_kernel_oblig = log_create("kernel_obligatorio.log", "Kernel", true, LOG_LEVEL_TRACE);
    } else if (strcmp(nivel, "LOG_LEVEL_DEBUG") == 0){
        log_kernel_oblig = log_create("kernel_obligatorio.log", "Kernel", true, LOG_LEVEL_DEBUG);
    } else if (strcmp(nivel, "LOG_LEVEL_INFO") == 0){
        log_kernel_oblig = log_create("kernel_obligatorio.log", "Kernel", true, LOG_LEVEL_INFO);
    } else if (strcmp(nivel, "LOG_LEVEL_WARNING") == 0){
        log_kernel_oblig = log_create("kernel_obligatorio.log", "Kernel", true, LOG_LEVEL_WARNING);
    } else if (strcmp(nivel, "LOG_LEVEL_ERROR") == 0){
        log_kernel_oblig = log_create("kernel_obligatorio.log", "Kernel", true, LOG_LEVEL_ERROR);
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
	// Y por ultimo, hay que liberar lo que utilizamos (conexiones, log y config)
	 // con las funciones de las commons y del TP mencionadas en el enunciado /
	liberar_conexion(log_kernel_gral, "CPU Dispatch", socket_cpu_dispatch);
	liberar_conexion(log_kernel_gral, "CPU Interrupt", socket_cpu_interrupt);
	log_destroy(log_kernel_oblig);
	log_destroy(log_kernel_gral);
	config_destroy(config);
}