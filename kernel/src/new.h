#ifndef NEW_KERNEL_H_
#define NEW_KERNEL_H_

#include <utils/general.h>
#include <utils/conexiones.h>
#include <commons/string.h>
#include <utils.h>

/**
* @brief La función que ejecuta el hilo destinado a manejar la cola NEW.
*/
void* rutina_new(void* puntero_null); // DESARROLLANDO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// ==========================================================================
// ====  Funciones Externas:  ===============================================
// ==========================================================================

/**
* @brief  Se conecta con memoria, le envía el pedido de creación de nuevo
*         proceso, recibe la respuesta, y se desconecta.
* @return Exito al inicializar el nuevo proceso.
*/
bool enviar_nuevo_proceso_a_memoria(t_pcb* pcb);

// ==========================================================================
// ====  Funciones Internas:  ===============================================
// ==========================================================================
/**
* @brief Envía el pedido de creación de nuevo proceso.
* @param socket : El socket de la conexión con memoria.
*/
void enviar_nuevo_proceso(t_pcb* pcb, int socket);

// ==========================================================================
// ====  Funciones Auxiliares:  =============================================
// ==========================================================================



#endif
