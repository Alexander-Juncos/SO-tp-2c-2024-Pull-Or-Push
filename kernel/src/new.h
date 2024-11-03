#ifndef NEW_KERNEL_H_
#define NEW_KERNEL_H_

#include <utils/general.h>
#include <utils/conexiones.h>
#include <commons/string.h>
#include <utils.h>

/**
* @brief La función que ejecuta el pthread destinado a manejar la cola NEW.
*/
void* rutina_new(void* puntero_null);

// ==========================================================================
// ====  Funciones Externas:  ===============================================
// ==========================================================================

/**
* @brief  Activa al Proceso que se encuentre esperando hace más tiempo en NEW.
* @return El TCB del Hilo main del Proceso activado.
*/
t_tcb* inicializacion_de_proceso(void);

// ==========================================================================
// ====  Funciones Internas:  ===============================================
// ==========================================================================

/**
* @brief  Se conecta con Memoria, le envía el pedido de creación de nuevo
*         Proceso, recibe la respuesta, y se desconecta.
* @param pcb : El PCB del Proceso a crear.
* @return Memoria tuvo éxito al crear el nuevo proceso.
*/
bool enviar_nuevo_proceso_a_memoria(t_pcb* pcb);
/**
* @brief  Espera que se cumplan las condiciones necesarias, y reintenta la
*         creación del Proceso, reenviando el pedido a Memoria.
* @param pcb : El PCB del Proceso a crear.
* @return Memoria tuvo éxito al crear el nuevo proceso.
*/
bool reintentar_creacion_proceso(t_pcb* pcb);

// ==========================================================================
// ====  Funciones Auxiliares:  =============================================
// ==========================================================================

/**
* @brief Envía el pedido de creación de nuevo Proceso.
* @param pcb    : PCB del nuevo Proceso.
* @param socket : El socket de la conexión.
*/
void enviar_nuevo_proceso(t_pcb* pcb, int socket);

#endif
