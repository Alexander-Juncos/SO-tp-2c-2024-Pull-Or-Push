#ifndef SERVIDOR_MULTIHILO_FILESYSTEM_H_
#define SERVIDOR_MULTIHILO_FILESYSTEM_H_
#include <utils.h>

// ==========================================================================
// ====  Variables:  ========================================================
// ==========================================================================

pthread_mutex_t mutex_socket_cliente_temp;
int socket_cliente_temp = 1;

// ==========================================================================
// ====  Funciones Servidor Multihilo:  =====================================
// ==========================================================================

void rutina_recepcion (void);
void* rutina_ejecucion (void*);

#endif /* SERVIDOR_MULTIHILO_FILESYSTEM_H_ */