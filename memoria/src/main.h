#ifndef MAIN_MEMORIA_H_
#define MAIN_MEMORIA_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>

#include "utils.h"

/* PLACE HOLDER - a revisar

/// @brief recibe solo conexiones temporales (IO y KERNEL)
/// @param nada // no deberia recibir nada, simplemente es * por pthread
/// @return // no deberia retornar, solo utilizar pthread_exit()
void* rutina_recepcion(void *nada);

/// @brief Actua sobre un cliente (ya verificado), recibiendo su operacion y ejecutando lo que sea necesario
/// @param nada // no deberia recibir nada, simplemente es * por pthread
/// @return // no deberia retornar, solo utilizar pthread_exit()
void* rutina_ejecucion(void *nada);

/// @brief Atiende al CPU en bucle hasta q este se desconecte
/// @param socket comunicacion con CPU, para recepcion y envio de paquetes
void atender_cpu(int socket);
*/

void terminar_programa(); // revisar y modificar-quizas podria liberar la memoria tambien

#endif /* MAIN_MEMORIA_H */