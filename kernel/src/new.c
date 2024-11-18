#include <new.h>

// variables globales para este pthread:
// -------------------------------------
// por ahora ninguna...
// -------------------------------------

void* rutina_new(void* puntero_null) {

    t_tcb* tcb_hilo_main = NULL;
    log_debug(log_kernel_gral, "Hilo responsable de cola NEW listo.");

    cola_new = list_create();

    while(true) {

        tcb_hilo_main = inicializacion_de_proceso();
        ingresar_a_ready(tcb_hilo_main);
    }

    return NULL;
}

// ==========================================================================
// ====  Funciones Externas:  ===============================================
// ==========================================================================

t_tcb* inicializacion_de_proceso(void) {
    t_pcb* pcb = NULL;
    bool exito_al_inicializar_proceso = true;
    
    // Espera hasta saber que hay algún proceso en la cola NEW.
    sem_wait(&sem_cola_new);
    pthread_mutex_lock(&mutex_cola_new);
    pcb = list_remove(cola_new, 0);
    pthread_mutex_unlock(&mutex_cola_new);

    exito_al_inicializar_proceso = enviar_nuevo_proceso_a_memoria(pcb);

    while(!exito_al_inicializar_proceso) {

        // Reintenta.
        exito_al_inicializar_proceso = reintentar_creacion_proceso(pcb);
    }

    asociar_tid(pcb, pcb->hilo_main);
    pthread_mutex_lock(&mutex_procesos_activos);
    list_add(procesos_activos, pcb);
    pthread_mutex_unlock(&mutex_procesos_activos);
    return pcb->hilo_main;
}

// ==========================================================================
// ====  Funciones Internas:  ===============================================
// ==========================================================================

bool enviar_nuevo_proceso_a_memoria(t_pcb* pcb) {
    bool exito_al_crear_proceso_en_memoria;
    int socket_memoria = crear_conexion_memoria(); // considera el handshake
    enviar_nuevo_proceso(pcb, socket_memoria);
    exito_al_crear_proceso_en_memoria = recibir_mensaje_de_rta(log_kernel_gral, "CREAR PROCESO", socket_memoria);
    liberar_conexion(log_kernel_gral, "Memoria (en Hilo NEW)", socket_memoria);
    return exito_al_crear_proceso_en_memoria;
}

bool reintentar_creacion_proceso(t_pcb* pcb) {
    bool exito_creacion_proceso;
    pthread_mutex_lock(&mutex_sincro_new_exit);
    new_puede_intentar_crear_proceso = false;
    pthread_mutex_unlock(&mutex_sincro_new_exit);
    // Espera que EXIT avise la finalización de algún proceso.
    sem_wait(&sem_sincro_new_exit);
    // Reintenta la creación del proceso.
    exito_creacion_proceso = enviar_nuevo_proceso_a_memoria(pcb);
    return exito_creacion_proceso;
}

// ==========================================================================
// ====  Funciones Auxiliares:  =============================================
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

