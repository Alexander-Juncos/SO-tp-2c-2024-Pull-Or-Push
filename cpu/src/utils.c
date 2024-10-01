#include <utils.h>

// ==========================================================================
// ====  Variables globales:  ===============================================
// ==========================================================================

int socket_escucha_puerto_dispatch = 1;
int socket_escucha_puerto_interrupt = 1;
int socket_kernel_dispatch = 1;
int socket_kernel_interrupt = 1;
int socket_memoria = 1;

t_log* log_cpu_oblig; 
t_log* log_cpu_gral; 
t_config* config;

t_contexto_exec contexto_exec;
t_interrupcion tipo_interrupcion = NINGUNA;
pthread_mutex_t mutex_interrupcion;

// ==========================================================================
// ====  Funciones Internas:  ===============================================
// ==========================================================================

// ==========================================================================
// ====  Funciones Externas:  ===============================================
// ==========================================================================

char* fetch (void)
{
    t_paquete* paq = crear_paquete(OBTENER_INSTRUCCION);
    agregar_a_paquete(paq, &(contexto_exec.pid), sizeof(int));
    agregar_a_paquete(paq, &(contexto_exec.tid), sizeof(int));
    agregar_a_paquete(paq, &(contexto_exec.PC), sizeof(uint32_t));
    enviar_paquete(paq, socket_memoria);
    eliminar_paquete(paq);

    if(recibir_codigo(socket_memoria) != OBTENER_INSTRUCCION){
        log_error(log_cpu_gral,"Error en respuesta de siguiente instruccion");
    }
    t_list* list = recibir_paquete(socket_memoria);
    char* instruccion = list_get(list,0);
    list_destroy(list);

    // logs grales y obligatorio
    log_info(log_cpu_gral, "PID: %d - TID: %d - FETCH - Program Counter: %d", contexto_exec.pid, contexto_exec.PC.PC);
    log_info(log_cpu_oblig, "## TID: <%d> - FETCH - Program Counter: <%d>",contexto_exec.tid,contexto_exec.PC);
    log_info(log_cpu_gral, "Instruccion recibida: %s", instruccion);

    return instruccion;
}

// ==========================================================================
// ====  Funciones Auxiliares:  =============================================
// ==========================================================================

void iniciar_logs(bool testeo)
{
    log_cpu_gral = log_create("cpu_general.log", "CPU", testeo, LOG_LEVEL_DEBUG);
    
    // Log obligatorio
    char * nivel;
    nivel = config_get_string_value(config, "LOG_LEVEL");
    log_cpu_oblig = log_create("cpu_obligatorio.log", "CPU", true, log_level_from_string(nivel));

    /*
        Ver luego si se quiere manejar caso de que el config este mal () y como cerrar el programa.
    */

    free(nivel);		
}

void terminar_programa() // revisar
{
	liberar_conexion(log_cpu_gral, "Memoria", socket_memoria);
	liberar_conexion(log_cpu_gral, "Kernel del puerto Dispatch", socket_kernel_dispatch);
	liberar_conexion(log_cpu_gral, "Kernel del puerto Interrupt", socket_kernel_interrupt);
	log_destroy(log_cpu_oblig);
	log_destroy(log_cpu_gral);
	config_destroy(config);
}