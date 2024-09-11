#ifndef MAIN_MEMORIA_H_
#define MAIN_MEMORIA_H_

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
// ====  Funcion Hilo main (serv. cpu):  ====================================
// ==========================================================================

void atender_cpu(void);

// ==========================================================================
// ====  Funciones Servidor Multihilo:  =====================================
// ==========================================================================

void* rutina_recepcion (void*);
void* rutina_ejecucion (void*);

#endif /* MAIN_MEMORIA_H */