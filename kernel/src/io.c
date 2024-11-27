#include "io.h"

// variables globales para este hilo:
// ============================================
unsigned int tiempo_uso_io_en_microsegs = 0;
// ============================================

void* rutina_io(void* puntero_null) {
    t_tcb* tcb = NULL;
    log_debug(log_kernel_gral, "Hilo responsable de simular IO listo.");

    while(true) {

        sem_wait(&sem_cola_blocked_io);

        pthread_mutex_lock(&mutex_cola_blocked_io);
        if(!list_is_empty(cola_blocked_io)) {
            tcb = list_remove(cola_blocked_io, 0);
        }
        pthread_mutex_unlock(&mutex_cola_blocked_io);

        if(tcb != NULL) {
            pthread_mutex_lock(&mutex_hilo_usando_io);
            hilo_usando_io = tcb;
            pthread_mutex_unlock(&mutex_hilo_usando_io);

            usleep(tiempo_uso_io_en_microsegs);

            pthread_mutex_lock(&mutex_hilo_usando_io);
            if(hilo_usando_io != NULL) { // if (el hilo no finalizó mientras la IO laburaba)
                ingresar_a_ready(hilo_usando_io);
                hilo_usando_io = NULL;
            }
            pthread_mutex_unlock(&mutex_hilo_usando_io);
            
            tcb = NULL;
        }
    }


    return NULL;
}

// ==========================================================================

void usar_io(t_tcb* tcb, int tiempo_uso_io_en_ms) {
    tiempo_uso_io_en_microsegs = tiempo_uso_io_en_ms*MILISEG_A_MICROSEG;
    pthread_mutex_lock(&mutex_cola_blocked_io);
    list_add(cola_blocked_io, tcb);
    pthread_mutex_unlock(&mutex_cola_blocked_io);
    sem_post(&sem_cola_blocked_io);
    log_info(log_kernel_oblig, "## (%d:%d) - Bloqueado por: IO", tcb->pid_pertenencia, tcb->tid);
    //hay_que_chequear_colas_cmn = true; // Sirve solo para CMN
    hilo_exec = NULL;
}
