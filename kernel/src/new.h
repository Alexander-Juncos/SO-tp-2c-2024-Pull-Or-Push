#ifndef NEW_KERNEL_H_
#define NEW_KERNEL_H_

#include <utils/general.h>
#include <utils/conexiones.h>
#include <utils.h>

/**
* @fn    rutina_new
* @brief La función que ejecuta el hilo destinado a manejar la cola NEW.
*/
void* rutina_new(void* puntero_null);


#endif