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
bool enviar_nuevo_proceso_a_memoria(); // DESARROLLANDO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
/**
* @brief  Se conecta con memoria, le envía el pedido de creación de nuevo
*         hilo, recibe la respuesta, y se desconecta.
* @return Exito al inicializar el nuevo hilo.
*/
bool enviar_nuevo_hilo_a_memoria(); // DESARROLLANDO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// ==========================================================================
// ====  Funciones Internas:  ===============================================
// ==========================================================================



// ==========================================================================
// ====  Funciones Auxiliares:  =============================================
// ==========================================================================
/**
* @brief Envía el pedido de creación de nuevo proceso.
* @param socket : El socket de la conexión con memoria.
*/
void enviar_nuevo_proceso(int socket); // DESARROLLANDO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


#endif
