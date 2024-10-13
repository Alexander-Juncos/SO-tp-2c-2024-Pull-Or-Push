#ifndef UTILS_GENERAL_H_
#define UTILS_GENERAL_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/collections/list.h>

// Cambié el uso de sleep(), por el uso de usleep(), que usa MICROSEG en vez de SEG.
// Esto nos permite ser más exactos en los tiempos de espera, y no tener que usar "div_t".
// Cambié el uso de sleep(), por el uso de usleep(), que usa MICROSEG en vez de SEG.
// Esto nos permite ser más exactos en los tiempos de espera, y no tener que usar "div_t".
#define MILISEG_A_MICROSEG 1000

typedef enum
{
/* -------------------------------------------------------------------------------------------- */
/* ------ Motivos que siempre indican FIN DE PROCESO (mueven el proceso al estado EXIT). ------ */
/* -------------------------------------------------------------------------------------------- */
    // Se leyó la "instrucción EXIT".
	SUCCESS,
    // Memoria no pudo asignar más tamanio al proceso (falló la "instrucción RESIZE").
    OUT_OF_MEMORY,
/* ------------------------------------------------------------------------------------------------------------------------------- */
/* ------ Motivos que siempre indican que el PROCESO ESTÁ LISTO PARA SEGUIR EJECUTANDO (mueven el proceso al estado READY). ------ */
/* ------------------------------------------------------------------------------------------------------------------------------- */
    // En RR o VRR, se consumió todo el quantum.
    INTERRUPTED_BY_QUANTUM,

/* -------------------------------------------------------------------------------------------------------------------------- */
/* ------ Motivos que indican un BLOQUEO DE PROCESO (mueven el proceso al estado BLOCKED), siempre que la interfaz ---------- */
/* ------ solicitada existe y acepta la operación. Caso contrario, se realiza un FIN DE PROCESO por INVALID_INTERFACE. ------ */
/* -------------------------------------------------------------------------------------------------------------------------- */
    // Se leyó la "instrucción IO_GEN_SLEEP".
    IO,
    // Se leyó la "instrucción IO_FS_CREATE".
    FS_CREATE,
    // Se leyó la "instrucción IO_FS_DELETE".
    FS_DELETE,
    // Se leyó la "instrucción IO_FS_TRUNCATE".
    FS_TRUNCATE,
    // Se leyó la "instrucción IO_FS_WRITE".
    FS_WRITE,
    // Se leyó la "instrucción IO_FS_READ".
    FS_READ,

/* ------------------------------------------------------------------------------------------------------------------------- */
/* ------ Motivos que indican que el PROCESO SIGUE EJECUTANDO (permanece en el estado EXEC), siempre que el recurso -------- */
/* ------ solicitado existe y dispone de instancias disponibles. ----------------------------------------------------------- */
/* ------ Si no existe se realiza un FIN DE PROCESO por INVALID_RESOURCE. -------------------------------------------------- */
/* ------ Por su parte, si no hay instancias disponibles (solo en el caso del WAIT) se realiza un BLOQUEO DE PROCESO. ------ */
/* ------------------------------------------------------------------------------------------------------------------------- */
    // Se leyó la "instrucción WAIT", que solicita retener una instancia de un recurso.
    WAIT,
    // Se leyó la "instrucción SIGNAL", que solicita liberar una instancia de un recurso.
    SIGNAL

} motivo_desalojo_code;

typedef struct
{
    uint32_t AX;
    uint32_t BX;
    uint32_t CX;
    uint32_t DX;
    uint32_t EX;
    uint32_t FX;
    uint32_t GX;
    uint32_t HX;
} t_reg_cpu;

typedef struct
{
    int pid;
    uint32_t PC;
    t_reg_cpu_uso_general reg_cpu_uso_general;
} t_contexto_de_ejecucion;

typedef struct {
    t_contexto_de_ejecucion contexto;
    motivo_desalojo_code motiv;
} t_desalojo;

typedef struct 
{
    int tid;
    int pid_pertenencia; // agrego este campo para facilidad de implementación
    int prioridad;
    char* path_relativo_archivo_instrucciones;
} t_tcb;

typedef struct
{
    int pid;
    int tamanio;
    t_list* tids_asociados;
    t_list* mutex_creados;
    int sig_tid_a_asignar;
    t_tcb* hilo_main;
} t_pcb;

// Estructura para usar con el diccionario de colas Ready, en algoritmo MULTINIVEL.
typedef struct
{
    int cantidad_de_hilos_activos;
    t_list* cola_ready;
} t_cola_ready;

typedef struct
{
    bool asignado;
    int tid_asignado;
    t_tcb* bloqueados_esperando;
} t_mutex;

typedef struct
{
    char* nombre;
    int instancias_disponibles;
} t_recurso;

typedef struct
{
    char* nombre;
    int instancias;
} t_recurso_ocupado;

typedef struct
{
    char* nombre;
    int socket;
    t_list* cola_blocked; // Es una lista de t_pcb*
} t_io_blocked;

typedef struct
{
    char* nombre;
    int instancias_disponibles;
    t_list* cola_blocked; // Es una lista de t_pcb*
} t_recurso_blocked;

typedef enum // son los posibles mensajes q puede recibir por interrupción CPU
{
    NADA,
    FINALIZAR, // interrumpido de forma "manual". NO SÉ SI ES NECESARIO TENER ESTE COD. EN ESTE TP
    DESALOJAR // por fin de quantum
} t_interrupt_code; // revisar si no requiere modificación, por la consigna de este TP

/**
* @brief Imprime un saludo por consola
* @param quien Módulo desde donde se llama a la función
* @return No devuelve nada
*/
void saludar(char* quien);

t_config* iniciar_config(char* tipo_config);
char* obtener_ruta_archivo_config(char* tipo_config);

///////////////////////////////////////////////////////////////
// Estas dos funciones las agregamos para tener formas
// más faciles de imprimir. Además de que cuando invocamos
// el printf() directamente, a veces el texto no aparece en
// consola al momento. En cambio asi metido dentro de otra
// función si aparece.
///////////////////////////////////////////////////////////////
/**
* @fn    imprimir_mensaje
* @brief Imprime por consola el mensaje pasado por parametro
*        agregandole un newline al final.
*/
void imprimir_mensaje(char* mensaje);
/**
* @fn    imprimir_entero
* @brief Imprime por consola el numero pasado por parametro
*        agregandole un newline al final.
*/
void imprimir_entero(int num);
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

#endif /* UTILS_GENERAL_H_ */
