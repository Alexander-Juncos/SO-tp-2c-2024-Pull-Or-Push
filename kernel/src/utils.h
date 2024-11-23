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
#include <pthread.h>
#include <semaphore.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>
#include <utils/general.h>
#include <utils/conexiones.h>
#include <new.h>
#include <exit.h>
#include <io.h>
#include <planificador.h>

typedef void (*PtrFuncionIngresarReady)(t_tcb*);
typedef t_tcb* (*PtrFuncionEncontrarYRemoverTCBReady)(int, int);

// ==========================================================================
// ====  Variables globales:  ===============================================
// ==========================================================================

typedef enum
{
    FIFO,
    PRIORIDADES,
    CMN
} algoritmo_corto_code;

extern algoritmo_corto_code cod_algoritmo_planif_corto; // cod algoritmo planif. Obtenido del diccionario con la key del archivo config
//extern int grado_multiprogramacion; // Viene del archivo config
//extern int procesos_activos; // Cantidad de procesos en READY, BLOCKED, o EXEC
extern bool hay_algun_proceso_en_exec;

extern char* ip_memoria;
extern char* puerto_memoria;
extern int socket_cpu_dispatch;
extern int socket_cpu_interrupt;

extern bool new_puede_intentar_crear_proceso;

extern t_list* cola_new; // Estado NEW. Es una lista de t_pcb* (Procesos).
extern t_list* cola_ready_unica; // Estado READY (para FIFO y PRIORIDADES). Es una lista de t_tcb* (Hilos).
extern t_dictionary* diccionario_ready_multinivel; // Estado READY (para MULTINIVEL). Contiene elementos de t_cola_ready*. La key es la prioridad.
extern t_tcb* hilo_exec; // Estado EXEC. Es un t_tcb* (Hilo).
extern t_tcb* hilo_usando_io; // Estado BLOCKED (usando IO). Es un t_tcb* (Hilo).
extern t_list* cola_blocked_io; // Estado BLOCKED (esperando para usar IO). Es una lista de t_tcb* (Hilos).
extern t_list* cola_blocked_join; // Estado BLOCKED (por Join). Es una lista de t_tcb* (Hilos).
extern t_list* cola_blocked_memory_dump; // Estado BLOCKED (por esperar respuesta de Memory Dump). Es una lista de t_tcb* (Hilos).
extern t_list* cola_exit; // Estado EXIT. Es una lista de t_tcb* (Hilos).

// Los bloqueados por Mutex, tienen sus propias colas dentro de los mutex listados en el PCB.

//extern t_pcb* proceso_exec; // Estado EXEC. Es un t_pcb* (Proceso).
extern t_list* procesos_activos; // Es una lista de t_pcb* (Procesos). Son los que están en READY, EXEC, o BLOCKED.
extern t_list* procesos_exit; // Es una lista de t_pcb* (Procesos). Son los que están en EXIT.

extern t_config *config;
extern char* algoritmo_plani;
extern PtrFuncionIngresarReady ingresar_a_ready; // Variable que referencia a la función de ingresar_a_ready() específica del algoritmo a usar.
extern PtrFuncionEncontrarYRemoverTCBReady encontrar_y_remover_tcb_en_ready; // Variable que referencia a la función de encontrar_y_remover_tcb_en_ready() específica del algoritmo a usar.
extern int quantum_de_config;

extern t_log* log_kernel_oblig; // logger para los logs obligatorios
extern t_log* log_kernel_gral; // logger para los logs nuestros. Loguear con criterio de niveles.

// ==========================================================================
// ====  Semáforos globales:  ===============================================
// ==========================================================================

extern sem_t sem_cola_new; // Cantidad de procesos en estado NEW.
extern sem_t sem_cola_ready_unica; // Cantidad de procesos en estado READY (para FIFO y PRIORIDADES).
extern sem_t sem_cola_blocked_io;
extern sem_t sem_cola_exit;
extern sem_t sem_sincro_new_exit;
extern pthread_mutex_t mutex_cola_new;
extern pthread_mutex_t mutex_cola_ready_unica;
// extern pthread_mutex_t mutex_hilo_exec; // ESTE ESTÁ EN OBSERVACIÓN. POR AHORA NO VA.
extern pthread_mutex_t mutex_hilo_usando_io;
extern pthread_mutex_t mutex_cola_blocked_io;
extern pthread_mutex_t mutex_cola_blocked_memory_dump;
extern pthread_mutex_t mutex_cola_exit;
extern pthread_mutex_t mutex_procesos_activos;
extern pthread_mutex_t mutex_procesos_exit;
extern pthread_mutex_t mutex_sincro_new_exit;
/* Sacados del tp anterior
----------------------------------------------
extern pthread_mutex_t mutex_proceso_exec;
extern pthread_mutex_t mutex_grado_multiprogramacion;
----------------------------------------------
*/

// ==========================================================================
// ====  Funciones Comunicación:  ===========================================
// ==========================================================================
/**
* @brief Envía una orden de interrupción a CPU, con datos del PID y TID.
*/
void enviar_orden_de_interrupcion(void);

// ==========================================================================
// ====  Funciones Utils:  ==================================================
// ==========================================================================

t_tcb* crear_tcb(int pid_creador, int tid, int prioridad, char* path_instrucciones);

/**
* @brief Crea y agrega un TID (identificador) a la lista de TID's asociados
*        a algún PCB.
* @param pcb : El PCB al cual asociar.
* @param tcb : El TCB cuyo TID se quiere asociar.
*/
void asociar_tid(t_pcb* pcb, t_tcb* tcb);
/**
* @brief Remueve (y destruye) un TID (identificador) de la lista de TID's asociados
*        a algún PCB.
* @param pcb : El PCB del cual desasociar.
* @param tcb : El TCB cuyo TID se quiere desasociar.
*/
void desasociar_tid(t_pcb* pcb, t_tcb* tcb);
/**
* @brief Busca un PCB según su PID.
* @param lista_de_pcb : Lista en la que buscar al PCB.
* @param pid          : PID del PCB a buscar.
* @return             : El PCB encontrado, o NULL en caso de no encontrarlo.
*/
t_pcb* buscar_pcb_por_pid(t_list* lista_de_pcb, int pid);
/**
* @brief Busca un TCB según su TID.
* @param lista_de_tcb : Lista en la que buscar al TCB.
* @param tid          : TID del TCB a buscar.
* @return             : El TCB encontrado, o NULL en caso de no encontrarlo.
*/
t_tcb* buscar_tcb_por_tid(t_list* lista_de_tcb, int tid);
/**
* @brief Busca un TCB según su TID y su PID de pertenencia.
* @param lista_de_tcb : Lista en la que buscar al TCB.
* @param pid          : PID de pertenencia del TCB a buscar.
* @param tid          : TID del TCB a buscar.
* @return             : El TCB encontrado, o NULL en caso de no encontrarlo.
*/
t_tcb* buscar_tcb_por_pid_y_tid(t_list* lista_de_tcb, int pid, int tid);
/**
* @brief NO INVOCAR DIRECTAMENTE. USAR "ingresar_a_ready(t_tcb*)" INDEPENDIENTEMENTE
*        DEL ALGORITMO DE PLANIFICACIÓN.
*
*        Pone un Hilo en READY, replanificando según algoritmo FIFO.
* @param tcb : El TCB del Hilo a poner en READY.
* @note "replanificar" conceptualmente, pues en realidad ya lo ingresa
*       ordenado en la cola.
*/
void ingresar_a_ready_fifo(t_tcb* tcb);
/**
* @brief NO INVOCAR DIRECTAMENTE. USAR "ingresar_a_ready(t_tcb*)" INDEPENDIENTEMENTE
*        DEL ALGORITMO DE PLANIFICACIÓN.
*
*        Pone un Hilo en READY, replanificando según algoritmo PRIORIDADES.
* @param tcb : El TCB del Hilo a poner en READY.
* @note "replanificar" conceptualmente, pues en realidad ya lo ingresa
*       ordenado en la cola.
*/
void ingresar_a_ready_prioridades(t_tcb* tcb);
/**
* @brief NO INVOCAR DIRECTAMENTE. USAR "ingresar_a_ready(t_tcb*)" INDEPENDIENTEMENTE
*        DEL ALGORITMO DE PLANIFICACIÓN.
*
*        Pone un Hilo en READY, replanificando según algoritmo CMN. En caso
*        de no existir la cola de READY correspondiente a la prioridad del Hilo
*        a ingresar, esta es creada.
* @param tcb : El TCB del Hilo a poner en READY.
* @note "replanificar" conceptualmente, pues en realidad lo ingresa
*       ordenado en la cola que corresponda.
*/
void ingresar_a_ready_multinivel(t_tcb* tcb);

t_cola_ready* crear_ready_multinivel(void);

void agregar_ready_multinivel(int prioridad);

t_tcb* encontrar_y_remover_tcb_en_ready_fifo_y_prioridades(int pid, int tid);

t_tcb* encontrar_y_remover_tcb_en_ready_multinivel(int pid, int tid);

void finalizar_hilos_no_main_de_proceso(t_pcb* pcb);
/**
* @brief Libera los mutexes asignados a un Hilo, y desbloquea a los Hilos que
*        tiene joineados. Además, si NO es Hilo main, desasocia su TID del Proceso
*        de pertenencia.
* @param pcb : PCB del Proceso creador.
* @param tcb : TCB del Hilo a liberar.
* @note  Lo deja listo para ser mandado a EXIT.
*/
void liberar_hilo(t_pcb* pcb, t_tcb* tcb);

void liberar_hilos_joineados(t_tcb* tcb);

void liberar_mutexes_asignados(t_pcb* pcb, t_tcb* tcb);

/**
* @brief Mueve a EXIT un Hilo, el cual ya se debe encontrar liberado.
* @param tcb : TCB del Hilo.
*/
void mandar_a_exit(t_tcb* tcb);

void iniciar_logs(bool testeo);
void terminar_programa();

// ==========================================================================
// ====  Funciones Auxiliares:  =============================================
// ==========================================================================

void enviar_nuevo_hilo(t_tcb* tcb, int socket);

/// @brief Crea la conexion con memoria y realiza el handshake (utiliza ip_memoria, puerto_memoria)
/// @return retorna el socket
int crear_conexion_memoria();

#endif
