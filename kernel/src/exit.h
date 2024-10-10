#ifndef EXIT_KERNEL_H_
#define EXIT_KERNEL_H_

#include <utils/general.h>
#include <utils/conexiones.h>
#include <utils.h>

/**
* @brief La función que ejecuta el hilo destinado a manejar la cola EXIT.
*/
void* rutina_exit(void* puntero_null); // DESARROLLANDO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// ==========================================================================
// ====  Funciones Externas:  ===============================================
// ==========================================================================

/**
* @brief Se conecta con memoria, le envía el pedido de finalización de Hilo,
*        recibe la respuesta, y se desconecta.
* @param tcb : TCB del Hilo a finalizar.
*/
void enviar_fin_hilo_a_memoria(t_tcb* tcb);
/**
* @brief Se conecta con memoria, le envía el pedido de finalización de Proceso,
*        recibe la respuesta, y se desconecta.
* @param pid : PID (identificador) del Proceso a finalizar.
*/
void enviar_fin_proceso_a_memoria(int pid);
/**
* @brief Destruye el TCB de un Hilo. Se usa tras el OK de Memoria.
* @param tcb : TCB a destruir.
*/
void destruir_tcb(t_tcb* tcb);
/**
* @brief Destruye el PCB de un Proceso. Se usa tras la destrucción de los
*        TCB de todos sus Hilos, y tras el OK de Memoria.
*
*        FALTA COMPLETAR!!!
* @param pid : PID (identificador) del PCB a destruir.
*/
void destruir_pcb(int pid); // FALTA COMPLETAR!!!

// ==========================================================================
// ====  Funciones Internas:  ===============================================
// ==========================================================================

/**
* @brief Envía el pedido de finalización de Hilo.
* @param tcb    : TCB del Hilo a finalizar.
* @param socket : El socket de la conexión.
*/
void enviar_fin_hilo(t_tcb* tcb, int socket);
/**
* @brief Envía el pedido de finalización de Proceso.
* @param pid    : PID (identificador) del Proceso a finalizar.
* @param socket : El socket de la conexión.
*/
void enviar_fin_proceso(int pid, int socket);

// ==========================================================================
// ====  Funciones Auxiliares:  =============================================
// ==========================================================================


#endif
