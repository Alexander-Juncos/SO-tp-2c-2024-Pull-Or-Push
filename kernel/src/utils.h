#ifndef UTILS_KERNEL_H_
#define UTILS_KERNEL_H_

// revisar si no sobran
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdint.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <assert.h>
#include <readline/readline.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>
#include <utils/general.h>
#include <utils/conexiones.h>
//#include <hilos.h>
#include <pthread.h>
#include <semaphore.h>

// ==========================================================================
// ====  Variables globales:  ===============================================
// ==========================================================================

extern int socket_cpu_dispatch;
extern int socket_cpu_interrupt;

extern t_list* cola_new; // Estado NEW. Es una lista de t_pcb* (Procesos)
extern t_list* cola_ready; // Estado READY. Es una lista de t_tcb* (Hilos)
extern t_pcb* proceso_exec; // Estado EXEC. Es un t_pcb* (Proceso)
extern t_tcb* hilo_exec; // Estado EXEC. Es un t_tcb* (Hilo)
extern t_list* cola_blocked; // Estado BLOCKED. Es una lista de t_tcb* (Hilos)
extern t_list* cola_exit; // Estado EXIT. Es una lista de t_tcb* (Hilos)

extern t_config *config;
extern int quantum_de_config;

extern t_log* log_kernel_oblig; // logger para los logs obligatorios
extern t_log* log_kernel_gral; // logger para los logs nuestros. Loguear con criterio de niveles.

// ==========================================================================
// ====  Funciones Comunicación:  ===========================================
// ==========================================================================

void enviar_orden_de_interrupcion(t_interrupt_code interrupt_code);

// ==========================================================================
// ====  Funciones Utils:  ==================================================
// ==========================================================================

void crear_hilo(int pid); // DESARROLLANDO
void iniciar_logs(bool testeo);
void terminar_programa();

#endif
