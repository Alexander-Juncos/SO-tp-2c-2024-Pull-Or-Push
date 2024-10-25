#include "respuesta_memory_dump.h"

// variables globales para este hilo:
// ============================================
// por ahora ninguna...
// ============================================

void* rutina_respuesta_memory_dump(void* info_para_recepcion) {
    t_recepcion_respuesta_memory_dump* recepcion = info_para_recepcion;

    bool dump_exitoso = recibir_mensaje_de_rta(log_kernel_gral, "MEMORY DUMP", recepcion->socket_de_la_conexion);
    pthread_mutex_lock(&mutex_cola_blocked_memory_dump);
    list_remove_element(cola_blocked_memory_dump, recepcion->tcb);
    pthread_mutex_unlock(&mutex_cola_blocked_memory_dump);
    // DESARROLLANDO
    if(dump_exitoso) {
        ingresar_a_ready(recepcion->tcb);
    }
    else {
        // finalizar_hilo()
        pthread_mutex_lock(&mutex_cola_exit);
        list_add(cola_exit, recepcion->tcb);
        pthread_mutex_unlock(&mutex_cola_exit);
        sem_post(&sem_cola_exit);
    }
    
    liberar_conexion(log_kernel_gral, "Memoria (en )", socket_memoria);
    free(recepcion);
    return NULL;

}

/*
void* rutina_respuesta_memory_dump_fifo(void* info_para_recepcion) {
    t_recepcion_respuesta_memory_dump* recepcion = info_para_recepcion;
    // DESARROLLANDO
    bool dump_exitoso = recibir_mensaje_de_rta(log_kernel_gral, "MEMORY DUMP", recepcion->socket_de_la_conexion);
    pthread_mutex_lock(&mutex_cola_blocked_memory_dump);
    list_remove_element(cola_blocked_memory_dump, recepcion->tcb);
    pthread_mutex_unlock(&mutex_cola_blocked_memory_dump);
    if(dump_exitoso) {
        ingresar_a_ready_fifo(recepcion->tcb);
    }
    else {

    }
    
    liberar_conexion(log_kernel_gral, "Memoria (en )", socket_memoria);
    return NULL;

}

void* rutina_respuesta_memory_dump_prioridades(void* info_para_recepcion) {
    t_recepcion_respuesta_memory_dump* recepcion = info_para_recepcion;
    // DESARROLLANDO
    bool dump_exitoso = recibir_mensaje_de_rta(log_kernel_gral, "MEMORY DUMP", recepcion->socket_de_la_conexion);
    if(dump_exitoso) {
        ingresar_a_ready_prioridades(recepcion->tcb);
    }
    else {
        
    }
    
    liberar_conexion(log_kernel_gral, "Memoria (en )", socket_memoria);
    return NULL;

}

void* rutina_respuesta_memory_dump_multinivel(void* info_para_recepcion) {
    t_recepcion_respuesta_memory_dump* recepcion = info_para_recepcion;
    // DESARROLLANDO
    bool dump_exitoso = recibir_mensaje_de_rta(log_kernel_gral, "MEMORY DUMP", recepcion->socket_de_la_conexion);
    if(dump_exitoso) {
        ingresar_a_ready_multinivel(recepcion->tcb);
    }
    else {
        
    }
    
    liberar_conexion(log_kernel_gral, "Memoria (en )", socket_memoria);
    return NULL;

}
*/
