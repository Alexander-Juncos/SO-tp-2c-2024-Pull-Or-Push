#include <utils.h>

// ==========================================================================
// ====  Variables globales:  ===============================================
// ==========================================================================

int socket_cpu_dispatch = 1;
int socket_cpu_interrupt = 1;

t_list* cola_new = NULL;
t_list* cola_ready = NULL;
t_pcb* proceso_exec = NULL;
t_tcb* hilo_exec = NULL;
t_list* cola_blocked = NULL;
t_list* cola_exit = NULL;

t_config *config = NULL;
int quantum_de_config;

t_log* log_kernel_oblig = NULL;
t_log* log_kernel_gral = NULL;

// ==========================================================================
// ====  Funciones Comunicación:  ===========================================
// ==========================================================================

void enviar_orden_de_interrupcion(t_interrupt_code interrupt_code) {
	t_paquete* paquete = crear_paquete(INTERRUPCION);
	agregar_a_paquete(paquete, &interrupt_code, sizeof(t_interrupt_code));
	// agregar_a_paquete(paquete, &(proceso_exec->pid), sizeof(int)); // Segun como hagamos protocolo creo q tendria q tener pid y tid
	enviar_paquete(paquete, socket_cpu_interrupt);
	eliminar_paquete(paquete);
}

// ==========================================================================
// ====  Funciones Utils:  ==================================================
// ==========================================================================

void crear_hilo(int pid) {
	// DESARROLLANDO
}

void iniciar_logs(bool testeo)
{
    log_kernel_gral = log_create("kernel_general.log", "Kernel", testeo, LOG_LEVEL_DEBUG);
    
    // Log obligatorio
    char * nivel;
    nivel = config_get_string_value(config, "LOG_LEVEL");
    log_kernel_oblig = log_create("kernel_obligatorio.log", "Kernel", true, log_level_from_string(nivel));

    /*
        Ver luego si se quiere manejar caso de que el config este mal () y como cerrar el programa.
    */

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
