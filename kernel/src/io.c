#include "io.h"

// variables globales para este hilo:
// ============================================
// ============================================

void* rutina_io(void* puntero_null) {
    t_blocked_io* blocked_io = NULL;
    t_tcb* tcb = NULL;
    log_debug(log_kernel_gral, "Hilo responsable de simular IO listo.");

    while(true) {

        sem_wait(&sem_cola_blocked_io);

        pthread_mutex_lock(&mutex_cola_blocked_io);
        if(!list_is_empty(cola_blocked_io)) {
            blocked_io = list_remove(cola_blocked_io, 0);
            tcb = blocked_io->tcb;
        }
        pthread_mutex_unlock(&mutex_cola_blocked_io);

        if(tcb != NULL) {
            pthread_mutex_lock(&mutex_hilo_usando_io);
            hilo_usando_io = tcb;
            pthread_mutex_unlock(&mutex_hilo_usando_io);

            log_debug(log_kernel_oblig, "Empiezo a dormir - PID: %d - TID: %d - voy a dormir: %d",tcb->pid_pertenencia,tcb->tid, blocked_io->tiempo_uso_io_en_microsegs/MILISEG_A_MICROSEG);
            usleep(blocked_io->tiempo_uso_io_en_microsegs);
            log_debug(log_kernel_oblig, "Me desperte - PID: %d - TID: %d",tcb->pid_pertenencia,tcb->tid);

            pthread_mutex_lock(&mutex_hilo_usando_io);
            if(hilo_usando_io != NULL) { // if (el hilo no finalizó mientras la IO laburaba)
                ingresar_a_ready(hilo_usando_io);
                hilo_usando_io = NULL;
            }
            pthread_mutex_unlock(&mutex_hilo_usando_io);
            
            tcb = NULL;
        }

        free(blocked_io);
    }


    return NULL;
}

// ==========================================================================

void usar_io(t_tcb* tcb, int tiempo_uso_io_en_ms) {
    t_blocked_io* hilo_bloqueado_por_io = malloc(sizeof(t_blocked_io));
    hilo_bloqueado_por_io->tcb = tcb;
    hilo_bloqueado_por_io->tiempo_uso_io_en_microsegs = tiempo_uso_io_en_ms*MILISEG_A_MICROSEG;
    pthread_mutex_lock(&mutex_cola_blocked_io);
    list_add(cola_blocked_io, hilo_bloqueado_por_io);
    pthread_mutex_unlock(&mutex_cola_blocked_io);
    sem_post(&sem_cola_blocked_io);
    log_info(log_kernel_oblig, "## (%d:%d) - Bloqueado por: IO", tcb->pid_pertenencia, tcb->tid);
    hilo_exec = NULL;
}
