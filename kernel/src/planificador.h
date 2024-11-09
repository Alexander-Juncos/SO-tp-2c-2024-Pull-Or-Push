#ifndef HILO_PLANIFICADOR_KERNEL_H_
#define HILO_PLANIFICADOR_KERNEL_H_

#include <commons/config.h>

#include "utils.h"

extern int contador_pid; // Contador. Para asignar diferente pid a cada nuevo proceso.

/**
* @brief Inicia el Planificador de corto plazo.
*/
void iniciar_planificador(void);

////////////////////////////////////////////

/////// ALGORITMOS

// EN DESARROLLO
void planific_corto_fifo_y_prioridades(void);

// EN DESARROLLO
void planific_corto_multinivel(void);

// DEL TP VIEJO, COMO REFERENCIA
// void planific_corto_rr(void);


// ==========================================================================
// ====  Funciones Externas:  ===============================================
// ==========================================================================

/**
* @brief Manda a ejecutar en CPU el siguiente Hilo de una cola READY (ya planificada).
*        Su correspondiente TCB queda en estado EXEC.
* @param cola_ready : La cola Ready de la cual se quiera poner a ejecutar el Hilo.
* @note  Asume que en ese momento no hay Hilo en ejecución (el CPU está esperando).
*/
void ejecutar_siguiente_hilo(t_list* cola_ready);

/**
* @brief Espera hasta que CPU devuelva el control a Kernel. Se recibe la syscall
*        o interrupción, para luego atenderla.
* @param codigo_operacion : La variable donde se guarda el op_code recibido.
* @return : Una lista con los valores (elementos) del paquete recibido.
*/
t_list* recibir_de_cpu(int* codigo_operacion);
/**
* @brief Busca a un TCB en EXEC, BLOCKED Y READY, según su TID y su PID de pertenencia.
*        En caso de encontrarlo, lo remueve de ese estado.
* @param pid          : PID de pertenencia del TCB a buscar.
* @param tid          : TID del TCB a buscar.
* @return             : El TCB encontrado (y removido), o NULL en caso de no encontrarlo.
*/
t_tcb* encontrar_y_remover_tcb(int pid, int tid);
/**
* @brief Libera un Hilo y lo manda a EXIT. Si el Hilo a finalizar es Hilo main, primero
*        finaliza todos los otros Hilos del Proceso (igual a un FIN DE PROCESO).
* @param tcb : TCB del Hilo a finalizar.
*/

void finalizar_proceso(int pid_pertenencia);
void finalizar_hilo(t_tcb* tcb);
/**
* @brief Crea un Mutex (sin asignar), y lo asocia a un Proceso.
* @param pcb    : PCB del Proceso a asociarle el nuevo Mutex.
* @param nombre : Nombre del nuevo Mutex.
*/
void crear_mutex(t_pcb* pcb, char* nombre);
/**
* @brief Se fija si ya hay un Mutex con un determinado nombre asociado a un Proceso.
* @param pcb    : PCB del Proceso donde fijarse la existencia del Mutex.
* @param nombre : Nombre del Mutex a buscar.
* @return       : 'true' si existe, 'false' si no existe.
*/
bool ya_existe_mutex(t_pcb* pcb, char* nombre);
/**
* @brief  Se conecta con memoria, le envía el pedido de creación de nuevo
*         hilo, recibe la respuesta, y se desconecta.
* @param tcb : TCB del Hilo a crear en Memoria.
*/

t_mutex* encontrar_mutex(t_pcb* pcb, char* nombre);

void enviar_nuevo_hilo_a_memoria(t_tcb* tcb);

// ==========================================================================
// ====  Funciones Internas  ================================================
// ==========================================================================

/**
* @brief Crea un nuevo Proceso, y su Hilo main.
* @param tamanio                : El tamanio del Proceso a crear.
* @param prioridad_hilo_main    : La prioridad del Hilo main del Proceso a crear.
* @param path_instruc_hilo_main : El path al archivo de instrucciones del Hilo
*                                 main del Proceso a crear.
* @return : El PCB del Proceso creado.
* @note El Proceso y el Hilo creados no pertenecen a ningún estado. Se deben
*       activar luego, moviendo el Proceso a NEW.
*/
t_pcb* nuevo_proceso(int tamanio, int prioridad_hilo_main, char* path_instruc_hilo_main);
/**
* @brief Activa un Proceso recién creado (lo mueve a NEW).
* @param pcb : PCB del Proceso a activar.
*/
void ingresar_a_new(t_pcb* pcb);
/**
* @brief Crea un nuevo Hilo, y lo asocia con el Proceso que ordenó crearlo.
* @param pcb_creador        : El PCB del Proceso creador.
* @param prioridad          : La prioridad del Hilo a crear.
* @param path_instrucciones : El path al archivo de instrucciones del Hilo a crear.
* @return : El TCB del Hilo creado.
* @note El Hilo creado no pertenece a ningún estado. Se debe activar luego,
*       moviéndolo a READY.
*/
t_tcb* nuevo_hilo(t_pcb* pcb_creador, int prioridad, char* path_instrucciones);

void bloquear_mutex(t_tcb* tcb, t_mutex* mutex);

void liberar_mutex(t_mutex* mutex);

void liberar_joineado(t_tcb* tcb);

void hacer_join(t_tcb* tcb, int tid_a_joinear);

/**
* @brief Envia a CPU la orden para ejecutar instrucciones de un Hilo.
* @param tcb : El TCB del Hilo a correr.
*/
void enviar_orden_de_ejecucion_al_cpu(t_tcb* tcb);

void enviar_pedido_de_dump_a_memoria(t_tcb* tcb);

// ==========================================================================
// ====  Funciones Auxiliares:  =============================================
// ==========================================================================

t_pcb* crear_pcb(int pid, int tamanio);

void enviar_pedido_de_dump(int pid, int tid, int socket);

/* OBSOLETO. --------------
t_recurso* encontrar_recurso_del_sistema(char* nombre);
t_recurso_ocupado* encontrar_recurso_ocupado(t_list* lista_de_recursos_ocupados, char* nombre);
t_recurso_blocked* encontrar_recurso_blocked(char* nombre);
void asignar_recurso_ocupado(t_pcb* pcb, char* nombre_recurso);
// void* planificador_largo(t_parametros_planif_largo arg); //funcion para pasar a hilo, cuando se necesita al planificador de largo plazo se crea el hilo y se le da un opcode dependiendo del requisito.
---------------------------
*/

#endif
