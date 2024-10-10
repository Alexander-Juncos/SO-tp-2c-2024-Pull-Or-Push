#include <new.h>

// variables globales para este hilo:
// ----------------------------------
// por ahora ninguna...
// ----------------------------------

void* rutina_new(void* puntero_null) {

    t_pcb* pcb = NULL;
    bool exito_al_inicializar_proceso = true;
    log_debug(log_kernel_gral, "Hilo responsable de cola NEW listo.");

    while(true) {

        sem_wait(&sem_cola_new);
        pcb = list_remove(cola_new, 0);
        exito_al_inicializar_proceso = enviar_nuevo_proceso_a_memoria(pcb);

        while(!exito_al_inicializar_proceso) {

            pthread_mutex_lock(&mutex_sincro_new_exit);
            new_puede_intentar_crear_proceso = false;
            pthread_mutex_unlock(&mutex_sincro_new_exit);
            // Espera que EXIT avise la finalización de algún proceso.
            sem_wait(&sem_sincro_new_exit);

            // Reintento de nuevo proceso.
            exito_al_inicializar_proceso = enviar_nuevo_proceso_a_memoria(pcb);
        }

        asociar_tid(pcb, pcb->hilo_main);
        // ingresar_a_ready(pcb->hilo_main); // CONOCE EL ALGORITMO DE PLANIF.

        pcb = NULL;

    }


    return NULL;
}

// ==========================================================================
// ====  Funciones Externas:  ===============================================
// ==========================================================================

bool enviar_nuevo_proceso_a_memoria(t_pcb* pcb) {
    bool exito_al_crear_proceso_en_memoria;
    int socket_memoria = crear_conexion(ip_memoria, puerto_memoria);
    enviar_nuevo_proceso(pcb, socket_memoria);
    exito_al_crear_proceso_en_memoria = recibir_mensaje_de_rta(log_kernel_gral, "CREAR PROCESO", socket_memoria);
    liberar_conexion(log_kernel_gral, "Memoria (en Hilo NEW)", socket_memoria);

    return exito_al_crear_proceso_en_memoria;
}

// ==========================================================================
// ====  Funciones Internas:  ===============================================
// ==========================================================================

void enviar_nuevo_proceso(t_pcb* pcb, int socket) {
    t_paquete* paquete = crear_paquete(CREAR_PROCESO);
    agregar_a_paquete(paquete, (void*)&(pcb->pid), sizeof(int));
    agregar_a_paquete(paquete, (void*)&(pcb->tamanio), sizeof(int));
    char* path = pcb->hilo_main->path_relativo_archivo_instrucciones;
    agregar_a_paquete(paquete, (void*)path, strlen(path) + 1);
    enviar_paquete(paquete, socket);
    eliminar_paquete(paquete);
}

// ==========================================================================
// ====  Funciones Auxiliares:  =============================================
// ==========================================================================


