#ifndef HILO_QUANTUM_KERNEL_H_
#define HILO_QUANTUM_KERNEL_H_

#include "utils.h"
#include <commons/temporal.h>

/* TODO SACADO DEL TP ANTERIOR!! */

extern t_temporal* timer;
extern int ms_transcurridos;

extern pthread_mutex_t mutex_rutina_quantum;

////////////////////////////////////////

void* rutina_quantum(void *puntero_null);
t_list* esperar_cpu_rr(int* codigo_operacion);

#endif
