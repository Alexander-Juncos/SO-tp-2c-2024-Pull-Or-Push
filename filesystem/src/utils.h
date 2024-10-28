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
#include <commons/bitarray.h>
// posiblemente estaria bueno incluir libreria para gestionar directorios
// Intento de hacer q el FS cree su propio directorio para archivos
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>

// ==========================================================================
// ====  Variables globales:  ===============================================
// ==========================================================================
extern t_log* log_fs_oblig; // logger para los logs obligatorios
extern t_log* log_fs_gral; // logger para los logs nuestros. Loguear con criterio de niveles.
extern t_config* config;

extern int socket_escucha;

extern bool fin_programa;

typedef struct {
    FILE* f_bloques;
    uint tam_bloques; 
    uint cant_bloques;
} t_file_system;

typedef struct {
    FILE* f;
    t_bitarray* bitarray;
    void *espacio_bitmap; // para funcionamiento interno bitmap
} t_bitmap;

extern t_file_system* fs;
extern pthread_mutex_t mutex_fs;
extern t_bitmap* bitmap;

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

/// @brief Abre - Crea el archivo bitmap, mapea el espacio_bitmap con el archivo y inicia el bitarray
/// @param ruta 
void iniciar_bitmap(char* ruta);

/// @brief Sincroniza todo el mapeo de bitmap.dat, podria encontrarse forma de
/// solo sincronizar el byte modificado, pero me parece mucho problema
/// Solo llamarla al realizar todos los cambios ?
void actualizar_f_bitmap();

/// @brief imprime el bitmap en lineas q contienen 20 bits del bitarray c/u 
void imprimir_bitmap();

void iniciar_logs(bool testeo);
void terminar_programa();
void retardo_acceso(); // retardo para cada acceso a bloques de memoria

#endif /* UTILS_FS_H */