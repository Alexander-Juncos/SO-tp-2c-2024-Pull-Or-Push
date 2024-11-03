#ifndef RESPUESTA_MEMORY_DUMP_KERNEL_H_
#define RESPUESTA_MEMORY_DUMP_KERNEL_H_

#include <utils/general.h>
#include <utils/conexiones.h>
#include <utils.h>

typedef struct
{
    t_tcb* tcb;
    int socket_de_la_conexion;
} t_recepcion_respuesta_memory_dump;

// ==========================================================================

/**
* @brief La función que ejecutan los pthread destinados a recibir
*        y manejar la respuesta al pedido de Memory Dump.
* @param info_para_recepcion : es del tipo t_recepcion_respuesta_memory_dump*
*/
void* rutina_respuesta_memory_dump(void* info_para_recepcion);

// ==========================================================================

#endif
