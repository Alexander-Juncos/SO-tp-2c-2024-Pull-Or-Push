#include <utils.h>

// ==========================================================================
// ====  Variables globales:  ===============================================
// ==========================================================================

algoritmo_corto_code cod_algoritmo_planif_corto;
//int grado_multiprogramacion;
//int procesos_activos = 0;
bool hay_algun_proceso_en_exec = false;

char* ip_memoria = NULL;
char* puerto_memoria = NULL;
int socket_cpu_dispatch = 1;
int socket_cpu_interrupt = 1;

bool new_puede_intentar_crear_proceso = true;

t_list* cola_new = NULL;
t_list* cola_ready_unica = NULL;
t_dictionary* diccionario_ready_multinivel;
t_tcb* hilo_exec = NULL;
t_tcb* hilo_usando_io = NULL;
t_list* cola_blocked_io = NULL;
t_list* cola_blocked_join = NULL;
t_list* cola_blocked_memory_dump = NULL;
t_list* cola_exit = NULL;

// Los bloqueados por Mutex, tienen sus propias colas dentro de los mutex listados en el PCB.

//t_pcb* proceso_exec = NULL;
t_list* procesos_activos = NULL;
t_list* procesos_exit = NULL;

t_config *config = NULL;
char* algoritmo_plani = NULL;
PtrFuncionIngresarReady ingresar_a_ready;
PtrFuncionEncontrarYRemoverTCBReady encontrar_y_remover_tcb_en_ready;
int quantum_de_config;

t_log* log_kernel_oblig = NULL;
t_log* log_kernel_gral = NULL;

// ==========================================================================
// ====  Semáforos globales:  ===============================================
// ==========================================================================

sem_t sem_cola_new;
sem_t sem_cola_ready_unica;
sem_t sem_cola_blocked_io;
sem_t sem_cola_exit;
/* Sacados del tp anterior
----------------------------------------------
pthread_mutex_t mutex_proceso_exec;
pthread_mutex_t mutex_grado_multiprogramacion;
pthread_mutex_t mutex_cola_new;
pthread_mutex_t mutex_cola_ready;
----------------------------------------------
*/
sem_t sem_sincro_new_exit;
pthread_mutex_t mutex_cola_new;
pthread_mutex_t mutex_hilo_exec;
pthread_mutex_t mutex_hilo_usando_io;
pthread_mutex_t mutex_cola_blocked_memory_dump;
pthread_mutex_t mutex_cola_exit;
pthread_mutex_t mutex_procesos_activos;
pthread_mutex_t mutex_procesos_exit;
pthread_mutex_t mutex_sincro_new_exit;

// ==========================================================================
// ====  Funciones Comunicación:  ===========================================
// ==========================================================================

void enviar_orden_de_interrupcion(void) {
	t_paquete* paquete = crear_paquete(INTERRUPCION);
	agregar_a_paquete(paquete, (void*)&(hilo_exec->pid_pertenencia), sizeof(int));
	agregar_a_paquete(paquete, (void*)&(hilo_exec->tid), sizeof(int));
	enviar_paquete(paquete, socket_cpu_interrupt);
	eliminar_paquete(paquete);
}

// ==========================================================================
// ====  Funciones Utils:  ==================================================
// ==========================================================================

t_tcb* crear_tcb(int pid_pertenencia, int tid, int prioridad, char* path_instrucciones) {
	t_tcb* tcb = malloc(sizeof(t_tcb));
	tcb->tid = tid;
	tcb->pid_pertenencia = pid_pertenencia;
	tcb->prioridad = prioridad;
	tcb->path_relativo_archivo_instrucciones = path_instrucciones;
	return tcb;
}

void asociar_tid(t_pcb* pcb, t_tcb* tcb) {
	int* tid_a_asociar = malloc(sizeof(int));
	*tid_a_asociar = tcb->tid;
	list_add(pcb->tids_asociados, tid_a_asociar);
}

void desasociar_tid(t_pcb* pcb, t_tcb* tcb) {

	bool _es_el_tid_buscado(int* tid) {
		return (*tid) == tcb->tid;
	};

    list_remove_and_destroy_by_condition(pcb->tids_asociados, (void*)_es_el_tid_buscado, (void*)free);
}

t_pcb* buscar_pcb_por_pid(t_list* lista_de_pcb, int pid) {

	bool _es_el_pcb_buscado(t_pcb* pcb) {
		return pcb->pid == pid;
	};

	t_pcb* pcb_encontrado = NULL;
	pcb_encontrado = list_find(lista_de_pcb, (void*)_es_el_pcb_buscado);
	return pcb_encontrado;
};

t_tcb* buscar_tcb_por_tid(t_list* lista_de_tcb, int tid) {

	bool _es_el_tcb_buscado(t_tcb* tcb) {
		return (tcb->tid==tid);
	};

	t_tcb* tcb_encontrado = NULL;
	tcb_encontrado = list_find(lista_de_tcb, (void*)_es_el_tcb_buscado);
	return tcb_encontrado;
}

t_tcb* buscar_tcb_por_pid_y_tid(t_list* lista_de_tcb, int pid, int tid) {

	bool _es_el_tcb_que_se_busca(t_tcb* tcb) {
		return (tcb->tid == tid) && (tcb->pid_pertenencia == pid);
	};

	t_tcb* tcb_encontrado = NULL;
	tcb_encontrado = list_find(lista_de_tcb, (void*)_es_el_tcb_que_se_busca);
	return tcb_encontrado;
}

void ingresar_a_ready_fifo(t_tcb* tcb) {
    list_add(cola_ready_unica, tcb);
    sem_post(&sem_cola_ready_unica);
}

void ingresar_a_ready_prioridades(t_tcb* tcb) {
    bool _hilo_tiene_menor_prioridad(t_tcb* tcb1, t_tcb* tcb2) {
        return tcb1->prioridad < tcb2->prioridad;
    };

    list_add_sorted(cola_ready_unica, tcb, (void*)_hilo_tiene_menor_prioridad);
    sem_post(&sem_cola_ready_unica);
}

void ingresar_a_ready_multinivel(t_tcb* tcb) {

    t_cola_ready* estructura_ready_correspondiente = NULL;
    char* key = string_itoa(tcb->prioridad);
    estructura_ready_correspondiente = dictionary_get(diccionario_ready_multinivel, key);

    if(estructura_ready_correspondiente == NULL) { // if (no existe cola para esa prioridad)
        estructura_ready_correspondiente = crear_ready_multinivel();
        dictionary_put(diccionario_ready_multinivel, key, estructura_ready_correspondiente);
    }

    list_add(estructura_ready_correspondiente->cola_ready, tcb);
    sem_post(&(estructura_ready_correspondiente->sem_cola_ready));
}

t_cola_ready* crear_ready_multinivel(void) {
    t_cola_ready* nueva_estructura_cola_ready = malloc(sizeof(t_cola_ready));
    nueva_estructura_cola_ready->cola_ready = list_create();
    sem_init(&(nueva_estructura_cola_ready->sem_cola_ready), 0, 0);
    return nueva_estructura_cola_ready;
}

void agregar_ready_multinivel(int prioridad) {
    t_cola_ready* estruct_cola_ready = crear_ready_multinivel();
    dictionary_put(diccionario_ready_multinivel, string_itoa(prioridad), estruct_cola_ready);
}

t_tcb* encontrar_y_remover_tcb_en_ready_fifo_y_prioridades(int pid, int tid) {
    t_tcb* tcb = NULL;
    tcb = buscar_tcb_por_pid_y_tid(cola_ready_unica, pid, tid);
    if(tcb != NULL) {
        list_remove_element(cola_ready_unica, tcb);
    }
    return tcb;
}

t_tcb* encontrar_y_remover_tcb_en_ready_multinivel(int pid, int tid) {
    t_tcb* tcb = NULL;
    t_cola_ready* cola_multinivel = NULL;

    bool _es_la_key_que_busco(char* key) {
        cola_multinivel = dictionary_get(diccionario_ready_multinivel, key);
        tcb = buscar_tcb_por_pid_y_tid(cola_multinivel->cola_ready, pid, tid);
        return tcb != NULL;
    };

    t_list* lista_de_keys = dictionary_keys(diccionario_ready_multinivel);
    char* key_de_cola_ready = NULL;
    key_de_cola_ready = list_find(lista_de_keys, (void*)_es_la_key_que_busco);
    cola_multinivel = NULL;
    if(key_de_cola_ready != NULL) {
        cola_multinivel = dictionary_get(diccionario_ready_multinivel, key_de_cola_ready);
        list_remove_element(cola_multinivel->cola_ready, tcb);
    }
    list_destroy(lista_de_keys);
    return tcb;
}

void finalizar_hilos_no_main_de_proceso(t_pcb* pcb) {
    int* ptr_tid;
    t_tcb* tcb = NULL;
    while(!list_is_empty(pcb->tids_asociados)) {
        ptr_tid = list_get(pcb->tids_asociados, 0);
        tcb = encontrar_y_remover_tcb(pcb->pid, *ptr_tid);
        if(tcb == NULL) {
            log_error(log_kernel_gral, "## (%d:%d) no encontrado. Revisar codigo.", pcb->pid, *ptr_tid);
            return;
        }
        else {
            liberar_hilo(pcb, tcb);
            log_info(log_kernel_oblig, "## (%d:%d) Finaliza el hilo", tcb->pid_pertenencia, tcb->tid);
            pthread_mutex_lock(&mutex_cola_exit);
            mandar_a_exit(tcb);
            pthread_mutex_unlock(&mutex_cola_exit);
        }
    }
}

void liberar_hilo(t_pcb* pcb, t_tcb* tcb) {
    liberar_mutexes_asignados(pcb, tcb);
    liberar_hilos_joineados(tcb);
    desasociar_tid(pcb, tcb);
}

void liberar_hilos_joineados(t_tcb* tcb) {

	bool _esta_joineado_a_este_hilo(t_tcb* tcb_bloqueado) {
		return (tcb_bloqueado->tid_joined == tcb->tid) && (tcb_bloqueado->pid_pertenencia == tcb->pid_pertenencia);
	};

    t_list* lista_de_hilos_joineados_a_este_hilo = list_filter(cola_blocked_join, (void*)_esta_joineado_a_este_hilo);
    int cant_a_desjoinear = list_size(lista_de_hilos_joineados_a_este_hilo);
    bool removido_exitosamente = true;
    for(int hilo_a_desjoinear = 1; hilo_a_desjoinear <= cant_a_desjoinear; hilo_a_desjoinear++) {
        removido_exitosamente = list_remove_element(cola_blocked_join, list_get(lista_de_hilos_joineados_a_este_hilo, hilo_a_desjoinear-1));
        if(!removido_exitosamente) {
            log_error(log_kernel_gral, "Algo anda mal en el codigo de liberar_joineados()");
        }
    }
    list_iterate(lista_de_hilos_joineados_a_este_hilo, (void*)liberar_joineado);
    list_destroy(lista_de_hilos_joineados_a_este_hilo);
}

void liberar_mutexes_asignados(t_pcb* pcb, t_tcb* tcb) {

	bool _es_mutex_asignado(t_mutex* mutex) {
		return (mutex->tid_asignado == tcb->tid) && (mutex->asignado == true);
	};

    t_list* lista_de_mutexes_asignados = list_filter(pcb->mutex_creados, (void*)_es_mutex_asignado);
    list_iterate(lista_de_mutexes_asignados, (void*)liberar_mutex);
    list_destroy(lista_de_mutexes_asignados);
}

void mandar_a_exit(t_tcb* tcb) {
    list_add(cola_exit, tcb);
    sem_post(&sem_cola_exit);
}

// ====================================================
// ====================================================

void iniciar_logs(bool testeo)
{
    log_kernel_gral = log_create("kernel_general.log", "Kernel", testeo, LOG_LEVEL_DEBUG);
    
    // Log obligatorio
    char * nivel;
    nivel = config_get_string_value(config, "LOG_LEVEL");
    log_kernel_oblig = log_create("kernel_obligatorio.log", "Kernel", true, log_level_from_string(nivel));

    /*
        Ver luego si se quiere manejar caso de que el config este mal () y como cerrar el programa.
    */

    free(nivel);		
}

void terminar_programa()
{
	// Y por ultimo, hay que liberar lo que utilizamos (conexiones, log y config)
	 // con las funciones de las commons y del TP mencionadas en el enunciado /
	liberar_conexion(log_kernel_gral, "CPU Dispatch", socket_cpu_dispatch);
	liberar_conexion(log_kernel_gral, "CPU Interrupt", socket_cpu_interrupt);
	log_destroy(log_kernel_oblig);
	log_destroy(log_kernel_gral);
	config_destroy(config);
}

// ==========================================================================
// ====  Funciones Auxiliares:  =============================================
// ==========================================================================

void enviar_nuevo_hilo(t_tcb* tcb, int socket) {
    t_paquete* paquete = crear_paquete(CREAR_HILO);
    agregar_a_paquete(paquete, (void*)&(tcb->tid), sizeof(int));
    char* path = tcb->path_relativo_archivo_instrucciones;
    agregar_a_paquete(paquete, (void*)path, strlen(path) + 1);
    enviar_paquete(paquete, socket);
    eliminar_paquete(paquete);
}
