#ifndef UTILS_MEMORIA_H_
#define UTILS_MEMORIA_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <assert.h>
#include <commons/log.h>
#include <commons/process.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <utils/general.h>
#include <pthread.h>
#include <commons/bitarray.h>
#include <utils/conexiones.h>
#include <utils/general.h>

// ==========================================================================
// ====  Variables globales:  ===============================================
// ==========================================================================
extern int socket_escucha; // socket servidor
extern int socket_cpu;

extern t_config *config; // donde se levanta lo del archivo .config del módulo memoria
extern t_log *log_memoria_oblig; // logger para los logs obligatorios
extern t_log *log_memoria_gral; // logger para los logs nuestros. Loguear con criterio de niveles.

extern void *espacio_bitmap_no_tocar; // solo se usa al crear/destruir el bitmap (a implementar)
extern bool fin_programa;

typedef enum {
    ERROR,
    CORRECTA,
    INSUFICIENTE
} resultado_operacion; // para funciones internas de memoria

typedef struct {
    int pid;
    /*
        pendiente
    */
} t_proceso; // inicia sin paginas asignadas

// ==========================================================================
// ====  Funciones Internas:  ===============================================
// ==========================================================================



// ==========================================================================
// ====  Funciones Auxiliares:  =============================================
// ==========================================================================

void retardo_operacion();
void iniciar_logs(bool testeo);
void terminar_programa(); // revisar y modificar-quizas podria liberar la memoria tambien

#endif /* UTILS_H_ */