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

// DESARROLLANDO
void* rutina_respuesta_memory_dump(void* info_para_recepcion);

/*
void* rutina_respuesta_memory_dump_fifo(void* info_para_recepcion);

void* rutina_respuesta_memory_dump_prioridades(void* info_para_recepcion);

void* rutina_respuesta_memory_dump_multinivel(void* info_para_recepcion);
*/

///////////////////////////////////////



#endif
