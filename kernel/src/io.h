#ifndef IO_KERNEL_H_
#define IO_KERNEL_H_

#include <utils/general.h>
#include <utils/conexiones.h>
#include <utils.h>

/**
* @brief La función que ejecuta el pthread destinado a la IO.
*/
void* rutina_io(void* puntero_null);

// ==========================================================================

void usar_io(t_tcb* tcb, int tiempo_uso_io_en_ms);

#endif
