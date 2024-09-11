#ifndef MAIN_FS_H_
#define MAIN_FS_H_

// revisar sino sobran
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>

#include "utils.h"

// ==========================================================================
// ====  Variables:  ========================================================
// ==========================================================================

pthread_mutex_t mutex_socket_cliente_temp;
int socket_cliente_temp = 1;

// ==========================================================================
// ====  Funciones Servidor:  ===============================================
// ==========================================================================

void rutina_recepcion (void);
void* rutina_ejecucion (void*);

#endif /* MAIN_FS_H_ */