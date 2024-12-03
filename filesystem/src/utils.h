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
    unsigned int bloques_libres_tot;
} t_bitmap;

extern t_file_system* fs;
extern pthread_mutex_t mutex_fs;
extern pthread_mutex_t mutex_bitmap;
extern t_bitmap* bitmap;

// typedef struct {
//     unsigned int bloque;
//     unsigned int cant_bloques;
// } t_bloques_libres;
// lo cambio para tomar simplificar el codigo (vector de unsigned int)

// ==========================================================================
// ====  Funciones Internas:  ===============================================
// ==========================================================================

bool iniciar_fs(void);

bool memory_dump(char* ruta, int size, void* data);

void escribir_bloques(char* nombre, unsigned int bloque_indice, void* data, unsigned int cant_bloques);

// ==========================================================================
// ====  Funciones Externas:  ===============================================
// ==========================================================================

bool rutina_memory_dump(t_list* param);

// ==========================================================================
// ====  Funciones Auxiliares:  =============================================
// ==========================================================================

/// @brief Abre - Crea el archivo bitmap, mapea el espacio_bitmap con el archivo y inicia el bitarray
/// @param ruta 
void iniciar_bitmap();

/// @brief Para que se haga un conteo de los bloques libres
void contar_bloques_libres_totales();

/// @brief Sincroniza todo el mapeo de bitmap.dat, podria encontrarse forma de
/// solo sincronizar el byte modificado, pero me parece mucho problema
/// Solo llamarla al realizar todos los cambios ?
void actualizar_f_bitmap();

/// @brief Revisa el bitmap buscando bloques libres y los va agregando una lista (de t_bloques_libres). Usando una busqueda
/// secuencial (usando ultimo_bloque_revisado)
/// @param cantidad  cantidad de bloques libres q tiene q encontrar (incluye bloque indices)
/// @return          Si no encontro suficiente retorna NULL, sino retorna un vector con los indices
unsigned int* bloques_libres (unsigned int cantidad);

void marcar_bloques_libres(unsigned int* v_bloques, unsigned int cant_bloques, char* archivo);

/// @brief imprime el bitmap en lineas q contienen 20 bits del bitarray c/u 
void imprimir_bitmap();

void iniciar_logs(bool testeo);
void terminar_programa();
void retardo_acceso(); // retardo para cada acceso a bloques de memoria
char *obtener_path_absoluto(char *ruta);
char *obtener_path_absoluto_metadata(char *ruta);

#endif /* UTILS_FS_H */