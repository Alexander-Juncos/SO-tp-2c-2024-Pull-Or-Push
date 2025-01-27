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

// para el uso de usleep(), que usa MICROSEG en vez de MILISEG.
#define MILISEG_A_MICROSEG 1000

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
    int tid;
    int pid_pertenencia; // Agrego este campo para facilidad de implementación
    int prioridad;
    char* path_relativo_archivo_instrucciones;
    int tid_joined; // Para saber a qué TID hizo join. Se evalúa solo en la "cola_blocked_join".
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

typedef struct
{
    char* nombre;
    bool asignado;
    int tid_asignado;
    t_list* bloqueados_esperando; // lista de TCB's
} t_mutex;

typedef struct
{
    t_tcb* tcb;
    int tiempo_uso_io_en_microsegs; // para el usleep()
} t_blocked_io;

/* ==========  OBSOLETO  ====================================================
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
*/ // =======================================================================

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
