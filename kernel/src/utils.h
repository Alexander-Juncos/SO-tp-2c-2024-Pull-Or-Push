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

extern char* ip_memoria;
extern char* puerto_memoria;
extern int socket_cpu_dispatch;
extern int socket_cpu_interrupt;

extern bool new_puede_intentar_crear_proceso;

extern t_list* cola_new; // Estado NEW. Es una lista de t_pcb* (Procesos).
extern t_list* cola_ready; // Estado READY. Es una lista de t_tcb* (Hilos).
extern t_tcb* hilo_exec; // Estado EXEC. Es un t_tcb* (Hilo).
extern t_list* cola_blocked; // Estado BLOCKED. Es una lista de t_tcb* (Hilos).
extern t_list* cola_exit; // Estado EXIT. Es una lista de t_tcb* (Hilos).

extern t_pcb* proceso_exec; // Estado EXEC. Es un t_pcb* (Proceso).
extern t_list* procesos_activos; // Es una lista de t_pcb* (Procesos). Son los que están en READY, EXEC, o BLOCKED.
extern t_list* procesos_exit; // Es una lista de t_pcb* (Procesos). Son los que están en EXIT.

extern t_config *config;
extern int quantum_de_config;

extern t_log* log_kernel_oblig; // logger para los logs obligatorios
extern t_log* log_kernel_gral; // logger para los logs nuestros. Loguear con criterio de niveles.

// ==========================================================================
// ====  Semáforos globales:  ===============================================
// ==========================================================================

extern sem_t sem_cola_new;
extern sem_t sem_cola_exit;

extern sem_t sem_sincro_new_exit;
extern pthread_mutex_t mutex_sincro_new_exit;

// ==========================================================================
// ====  Funciones Comunicación:  ===========================================
// ==========================================================================
/**
* @brief Envía una orden de interrupción a CPU.
* @param interrupt_code : El código de la interrupción.
*/
void enviar_orden_de_interrupcion(t_interrupt_code interrupt_code);
/**
* @brief  Se conecta con memoria, le envía el pedido de creación de nuevo
*         hilo, recibe la respuesta, y se desconecta.
* @return Exito al inicializar el nuevo hilo.
*/
bool enviar_nuevo_hilo_a_memoria(); // DESARROLLANDO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// ==========================================================================
// ====  Funciones Utils:  ==================================================
// ==========================================================================

// DESARROLLANDO
void crear_hilo(int pid); // DESARROLLANDO

/**
* @brief Crea y agrega un TID (identificador) a la lista de TID's asociados
*        a algún PCB.
* @param pcb : El PCB al cual asociar.
* @param tcb : El TCB cuyo TID se quiere asociar.
*/
void asociar_tid(t_pcb* pcb, t_tcb* tcb);

/**
* @brief Busca un PCB según su PID.
* @param lista_de_pcb : Lista en la que buscar al PCB.
* @param pid          : PID del PCB a buscar.
* @return             : El PCB encontrado, o NULL en caso de no encontrarlo.
*/
t_pcb* buscar_pcb_por_pid(t_list* lista_de_pcb, int pid);

void iniciar_logs(bool testeo);
void terminar_programa();

#endif
