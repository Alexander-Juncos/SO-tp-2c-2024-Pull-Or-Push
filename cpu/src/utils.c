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

void terminar_programa()
{
	liberar_conexion(log_cpu_gral, "Memoria", socket_memoria);
	liberar_conexion(log_cpu_gral, "Kernel del puerto Dispatch", socket_kernel_dispatch);
	liberar_conexion(log_cpu_gral, "Kernel del puerto Interrupt", socket_kernel_interrupt);
	log_destroy(log_cpu_oblig);
	log_destroy(log_cpu_gral);
	config_destroy(config);
}