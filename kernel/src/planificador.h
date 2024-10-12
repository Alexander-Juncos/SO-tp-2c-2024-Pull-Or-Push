#ifndef HILO_PLANIFICADOR_KERNEL_H_
#define HILO_PLANIFICADOR_KERNEL_H_

#include <commons/config.h>

#include "utils.h"

/* TODO SACADO DEL TP ANTERIOR!! */

/**
* @brief Inicia el Planificador de corto plazo.
*/
void iniciar_planificador(void);

////////////////////////////////////////////

/////// ALGORITMOS

// EN DESARROLLO
void planific_corto_fifo(void);

// EN DESARROLLO
void planific_corto_prioridades(void);

// EN DESARROLLO
void planific_corto_multinivel(void);

// DEL TP VIEJO, COMO REFERENCIA
void planific_corto_rr(void);


// ==========================================================================
// ====  Funciones Externas:  ===============================================
// ==========================================================================

// Pone el siguiente hilo a ejecutar. Asume que no hay proceso en ejecucion.
void ejecutar_siguiente_hilo(t_list* cola_ready);

// A MODIFICAR
void recibir_y_verificar_codigo(int socket, op_code cod, char* traduccion_de_cod);

// ==========================================================================
// ====  Funciones Internas:  ===============================================
// ==========================================================================

void ingresar_a_ready_fifo(t_tcb* tcb);

void ingresar_a_ready_prioridades(t_tcb* tcb);

void ingresar_a_ready_multinivel(t_tcb* tcb);

void enviar_orden_de_ejecucion_al_cpu(t_tcb* tcb, int socket);

// ==========================================================================
// ====  Funciones Auxiliares:  =============================================
// ==========================================================================

t_cola_ready* crear_ready_multinivel();

/* OBSOLETO. --------------
t_recurso* encontrar_recurso_del_sistema(char* nombre);
t_recurso_ocupado* encontrar_recurso_ocupado(t_list* lista_de_recursos_ocupados, char* nombre);
t_recurso_blocked* encontrar_recurso_blocked(char* nombre);
void asignar_recurso_ocupado(t_pcb* pcb, char* nombre_recurso);
// void* planificador_largo(t_parametros_planif_largo arg); //funcion para pasar a hilo, cuando se necesita al planificador de largo plazo se crea el hilo y se le da un opcode dependiendo del requisito.
---------------------------
*/

#endif
