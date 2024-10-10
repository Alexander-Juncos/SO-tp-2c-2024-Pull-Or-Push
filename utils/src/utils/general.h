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

// typedef struct
// {
//     int pid;
//     int tid;
//     uint32_t PC;
//     t_reg_cpu reg_cpu;
// } t_contexto_de_ejecucion;
// lo comento, porque cpu y memoria tendran sus propias estructuras

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
