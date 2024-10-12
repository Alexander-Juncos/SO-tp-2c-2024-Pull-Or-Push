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

typedef enum {
    FIRST_FIT,
    BEST_FIT,
    WORST_FIT
} algoritmo_busqueda_part_dinam;

typedef struct {
    unsigned int base;
    unsigned int limite;
} t_particion;

typedef struct {
    int tid;
    uint32_t PC;
    t_reg_cpu registros;
    t_list* instrucciones; 
} t_tcb_mem;

typedef struct {
    int pid;
    t_particion* particion;
    t_list* lista_tcb;
} t_pcb_mem;

typedef struct {
    t_pcb_mem* pcb;
    t_tcb_mem* tcb;
} t_contexto_de_ejecucion;

extern t_list* procesos_cargados; // sus elementos van a ser de tipo t_pcb_mem
extern pthread_mutex_t mutex_procesos_cargados;
extern t_contexto_de_ejecucion* contexto_ejecucion;
// extern pthread_mutex_t mutex_contexto_ejecucion; // lo comento porque solo el main va a acceder

typedef struct {
    void* espacio_usuario;
    bool particiones_dinamicas;
    uint8_t algorit_busq; // solo va a ser usado si particiones_dinamicas = TRUE
    int tamano_memoria;
    t_list* lista_particiones; // cada elemento puede apuntar a las particiones de los t_pcb?
} t_memoria_particionada; // temporal hasta q plantee bien el tema de particiones
/* 
    hay q generar metodos para q la lista_particiones se mantenga ordenada
*/
extern t_memoria_particionada* memoria;

// ==========================================================================
// ====  Funciones Internas:  ===============================================
// ==========================================================================

bool iniciar_memoria();
t_tcb_mem* iniciar_tcb(int pid, int tid, char* ruta_script);
t_pcb_mem* iniciar_pcb(int pid, int tamanio, char* ruta_script_tid_0);
bool cargar_contexto_ejecucion(int pid, int tid); 
bool actualizar_contexto_ejecucion(t_list* nuevo_pedido_raw); 
char* obtener_instruccion(uint32_t num_instruccion);

// ==========================================================================
// ====  Funciones Externas:  ===============================================
// ==========================================================================

bool memory_dump_fs (t_list* pedido);
void rutina_contexto_ejecucion(t_list* param);

// ==========================================================================
// ====  Funciones Auxiliares:  =============================================
// ==========================================================================

void retardo_operacion(void);
t_list* crear_lista_de_particiones(void);
t_particion* particion_libre (int tamanio); // PENDIENTE HASTA DESARROLLO ESPACIO USUARIO
t_list *cargar_instrucciones(char *directorio, int pid, int tid);
t_pcb_mem* obtener_pcb (int pid);
t_tcb_mem* obtener_tcb (int tid, t_list* lista_tcb);
t_paquete* empaquetar_contexto (void);
void iniciar_logs(bool testeo);
void terminar_programa(void); // revisar y modificar-quizas podria liberar la memoria tambien

#endif /* UTILS_H_ */