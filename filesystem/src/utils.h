#ifndef UTILS_FS_H_
#define UTILS_FS_H_

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <commons/log.h>
#include <utils/general.h>
#include <utils/conexiones.h>
// posiblemente estaria bueno incluir libreria para gestionar directorios

// ==========================================================================
// ====  Variables globales:  ===============================================
// ==========================================================================
extern t_log* log_fs_oblig; // logger para los logs obligatorios
extern t_log* log_fs_gral; // logger para los logs nuestros. Loguear con criterio de niveles.
extern t_config* config;

extern int socket_escucha;

extern bool fin_programa;

typedef struct {
    /*
        pendiente
    */
} t_file_system;
extern t_file_system* fs;

// ==========================================================================
// ====  Funciones Internas:  ===============================================
// ==========================================================================

bool iniciar_fs(void);
/*
    iniciar el fs, crear archivos, cerrar el fs
*/

// ==========================================================================
// ====  Funciones Auxiliares:  =============================================
// ==========================================================================

/*
    obtener bloques libres, actualizar bitmap, obtener path absoluto, quizas algo q emita logs
*/
void iniciar_logs(bool testeo);
void terminar_programa();

#endif /* UTILS_FS_H */