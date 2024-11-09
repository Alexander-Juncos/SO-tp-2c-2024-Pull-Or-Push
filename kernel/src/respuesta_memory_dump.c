#include "respuesta_memory_dump.h"

// variables globales para este hilo:
// ============================================
// por ahora ninguna...
// ============================================

void* rutina_respuesta_memory_dump(void* info_para_recepcion) {
    t_recepcion_respuesta_memory_dump* recepcion = info_para_recepcion;
    int pid_pertenencia = (recepcion->tcb)->pid_pertenencia;
    int tid = (recepcion->tcb)->tid;

    bool dump_exitoso = recibir_mensaje_de_rta(log_kernel_gral, "MEMORY DUMP", recepcion->socket_de_la_conexion);
    bool hilo_habia_finalizado = false;
    pthread_mutex_lock(&mutex_cola_blocked_memory_dump);
    // Acá agrego esta verificación extra, por si el
    // planificador lo mató mientras se esperaba la respuesta.
    if(buscar_tcb_por_pid_y_tid(cola_blocked_memory_dump, pid_pertenencia, tid) != NULL) {
        list_remove_element(cola_blocked_memory_dump, recepcion->tcb);
    }
    else {
        hilo_habia_finalizado = true;
        log_debug(log_kernel_gral, "Respuesta de Memory Dump descartada por finalizacion de hilo");
    }
    pthread_mutex_unlock(&mutex_cola_blocked_memory_dump);

    if(hilo_habia_finalizado) {
    liberar_conexion(log_kernel_gral, "Memoria (por Memory Dump)", recepcion->socket_de_la_conexion);
    free(recepcion);
    return NULL; 
    }

    if(dump_exitoso) {
        ingresar_a_ready(recepcion->tcb);
    }
    else {
        finalizar_hilo(recepcion->tcb);
    }
    
    liberar_conexion(log_kernel_gral, "Memoria (por Memory Dump)", recepcion->socket_de_la_conexion);
    free(recepcion);
    return NULL;
}
