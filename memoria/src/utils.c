#include "utils.h"

// ==========================================================================
// ====  Variables globales:  ===============================================
// ==========================================================================

int socket_cpu = 1;
int socket_escucha = 1;
int socket_cliente_temp = 1; // para que los hilos puedan tomar su cliente, protegido x semaforo (a implementar)
int socket_kernel = 1;

t_config *config; 
t_log *log_memoria_oblig; 
t_log *log_memoria_gral; 

// ==========================================================================
// ====  Funciones Internas:  ===============================================
// ==========================================================================



// ==========================================================================
// ====  Funciones Auxiliares:  =============================================
// ==========================================================================

void retardo_operacion()
{
    unsigned int tiempo_en_microsegs = config_get_int_value(config, "RETARDO_RESPUESTA")*MILISEG_A_MICROSEG;
    usleep(tiempo_en_microsegs);
}

void iniciar_logs(bool testeo)
{
    log_memoria_gral = log_create("Memoria_general.log", "Memoria", testeo, LOG_LEVEL_DEBUG);
    
    // Log obligatorio
    char * nivel;
    nivel = config_get_string_value(config, "LOG_LEVEL");
    if (strcmp(nivel, "LOG_LEVEL_TRACE")== 0){
        log_memoria_oblig = log_create("Memoria_obligatorio.log", "Memoria", true, LOG_LEVEL_TRACE);
    } else if (strcmp(nivel, "LOG_LEVEL_DEBUG") == 0){
        log_memoria_oblig = log_create("Memoria_obligatorio.log", "Memoria", true, LOG_LEVEL_DEBUG);
    } else if (strcmp(nivel, "LOG_LEVEL_INFO") == 0){
        log_memoria_oblig = log_create("Memoria_obligatorio.log", "Memoria", true, LOG_LEVEL_INFO);
    } else if (strcmp(nivel, "LOG_LEVEL_WARNING") == 0){
        log_memoria_oblig = log_create("Memoria_obligatorio.log", "Memoria", true, LOG_LEVEL_WARNING);
    } else if (strcmp(nivel, "LOG_LEVEL_ERROR") == 0){
        log_memoria_oblig = log_create("Memoria_obligatorio.log", "Memoria", true, LOG_LEVEL_ERROR);
    } else {
        printf("LOG_LEVEL de config desconocido...");
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
	liberar_conexion(log_memoria_gral, "CPU", socket_cpu);
	liberar_conexion(log_memoria_gral, "Mi propio Servidor Escucha", socket_escucha);
	log_destroy(log_memoria_oblig);
	log_destroy(log_memoria_gral);
	config_destroy(config);
}