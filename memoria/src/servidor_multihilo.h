#ifndef SERVIDOR_MULTIHILO_MEMORIA_H_
#define SERVIDOR_MULTIHILO_MEMORIA_H_
#include <utils.h>

// ==========================================================================
// ====  Variables:  ========================================================
// ==========================================================================

// pthread_mutex_t mutex_socket_cliente_temp;
// int socket_cliente_temp = 1;
// por alguna razon me detecta como doble declaracion si esta aca
// lo deje en el .c

// ==========================================================================
// ====  Funciones Servidor Multihilo:  =====================================
// ==========================================================================

void* rutina_recepcion (void*);
void* rutina_ejecucion (void*);

#endif /* SERVIDOR_MULTIHILO_MEMORIA_H_ */