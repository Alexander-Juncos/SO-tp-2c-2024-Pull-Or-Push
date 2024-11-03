#include <exit.h>

// variables globales para este hilo:
// ----------------------------------
// por ahora ninguna...
// ----------------------------------

void* rutina_exit(void* puntero_null) {

    t_tcb* tcb = NULL;
    int pid_de_pcb_a_eliminar;
    log_debug(log_kernel_gral, "Hilo responsable de cola EXIT listo.");

    while(true) {

        sem_wait(&sem_cola_exit);
        pthread_mutex_lock(&mutex_cola_exit);
        tcb = list_remove(cola_exit, 0);
        pthread_mutex_unlock(&mutex_cola_exit);

        if(tcb->tid == 0) { // (if es Hilo main)

            enviar_fin_hilo_a_memoria(tcb);
            enviar_fin_proceso_a_memoria(tcb->pid_pertenencia);

            pid_de_pcb_a_eliminar = tcb->pid_pertenencia;
            destruir_tcb(tcb);
            destruir_pcb(pid_de_pcb_a_eliminar);
            
            // Si NEW se encuentra esperando, le avisa
            // que reintente la creación de proceso.
            pthread_mutex_lock(&mutex_sincro_new_exit);
            if(!new_puede_intentar_crear_proceso) {
                new_puede_intentar_crear_proceso = true;
                sem_post(&sem_sincro_new_exit);
            }
            pthread_mutex_unlock(&mutex_sincro_new_exit);

        }
        else {
            enviar_fin_hilo_a_memoria(tcb);
            destruir_tcb(tcb);
        }
        
        tcb = NULL;

    }

    return NULL;
}


// ==========================================================================
// ====  Funciones Externas:  ===============================================
// ==========================================================================

void enviar_fin_hilo_a_memoria(t_tcb* tcb) {
    int socket_memoria = crear_conexion(ip_memoria, puerto_memoria);
    enviar_fin_hilo(tcb, socket_memoria);
    recibir_mensaje_de_rta(log_kernel_gral, "FINALIZAR HILO", socket_memoria);
    liberar_conexion(log_kernel_gral, "Memoria (en Hilo EXIT)", socket_memoria);
}

void enviar_fin_proceso_a_memoria(int pid) {
    int socket_memoria = crear_conexion(ip_memoria, puerto_memoria);
    enviar_fin_proceso(pid, socket_memoria);
    recibir_mensaje_de_rta(log_kernel_gral, "FINALIZAR PROCESO", socket_memoria);
    liberar_conexion(log_kernel_gral, "Memoria (en Hilo EXIT)", socket_memoria);
}

void destruir_tcb(t_tcb* tcb) {
    free(tcb->path_relativo_archivo_instrucciones);
    free(tcb);
}

void destruir_pcb(int pid) {
    t_pcb* pcb = buscar_pcb_por_pid(procesos_exit, pid);

    if(pcb == NULL) { // solo para asegurarnos que anda bien
        log_error(log_kernel_gral, "Error en la lógica de EXIT. No se encuentra el PCB a destruir.");
    }
    else {
        list_remove_element(procesos_exit, pcb);
        list_destroy_and_destroy_elements(pcb->tids_asociados, (void*)free);
        list_destroy_and_destroy_elements(pcb->mutex_creados, ¡¡¡¡COMPLETAR!!!!);
        free(pcb);
    }
}

// ==========================================================================
// ====  Funciones Internas:  ===============================================
// ==========================================================================

void enviar_fin_hilo(t_tcb* tcb, int socket) {
    t_paquete* paquete = crear_paquete(FINALIZAR_HILO);
    agregar_a_paquete(paquete, (void*)&(tcb->pid_pertenencia), sizeof(int));
    agregar_a_paquete(paquete, (void*)&(tcb->tid), sizeof(int));
    enviar_paquete(paquete, socket);
    eliminar_paquete(paquete);
}

void enviar_fin_proceso(int pid, int socket) {
    t_paquete* paquete = crear_paquete(FINALIZAR_PROCESO);
    agregar_a_paquete(paquete, (void*)&pid, sizeof(int));
    enviar_paquete(paquete, socket);
    eliminar_paquete(paquete);
}

// ==========================================================================
// ====  Funciones Auxiliares:  =============================================
// ==========================================================================

