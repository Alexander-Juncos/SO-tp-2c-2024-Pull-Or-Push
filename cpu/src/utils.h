#ifndef UTILS_CPU_H_
#define UTILS_CPU_H_

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <string.h>
#include <assert.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <utils/general.h>
#include <utils/conexiones.h>
#include <pthread.h>

// ==========================================================================
// ====  Variables globales:  ===============================================
// ==========================================================================

extern int socket_escucha_puerto_dispatch;
extern int socket_escucha_puerto_interrupt;
extern int socket_kernel_dispatch;
extern int socket_kernel_interrupt;
extern int socket_memoria;

extern t_log* log_cpu_oblig; // logger para los logs obligatorios
extern t_log* log_cpu_gral; // logger para los logs nuestros. Loguear con criterio de niveles.

extern t_config* config;

typedef enum {
    DESCONOCIDA,

    // instrucciones (solo cpu)
    SET, // (registro, valor)
    READ_MEM, // (registro dat, registro dir)
    WRITE_MEM, // (registro dir, registro dat)
    SUM, // (registro, registro)
    SUB, // (registro, registro)
    JNZ, // (registro, instruccion)
    LOG, // registro

    // syscalls (guardar contexto y dar control a kernel)
    DUMP_MEMORY, 
    IO, // (tiempo)
    PROCESS_CREATE, // (archivo, tamaño, prioridad tid 0)
    THREAD_CREATE, // (archivo, prioridad)
    THREAD_JOIN, // (tid)
    THREAD_CANCEL, // (tid)
    MUTEX_CREATE, // (recurso)
    MUTEX_LOCK, // (recurso)
    MUTEX_UNLOCK, // (recurso)
    THREAD_EXIT,
    PROCESS_EXIT
} execute_op_code;

typedef struct {
    int pid;
    int tid;
    uint32_t PC;
    t_reg_cpu registros;
    unsigned int base;
    unsigned int limite;
} t_contexto_exec;

typedef enum {
    NINGUNA,
    DESALOJO,
    SYSCALL,
    SEG_FAULT
} t_interrupcion;

extern t_contexto_exec contexto_exec;
extern execute_op_code codigo_instruccion;
extern t_interrupcion tipo_interrupcion;
extern pthread_mutex_t mutex_interrupcion;


// ==========================================================================
// ====  Funciones utils:  ==================================================
// ==========================================================================

void iniciar_logs(bool testeo);
void terminar_programa(); // revisar tema socket_kernel...

#endif /* UTILS_CPU_H_ */