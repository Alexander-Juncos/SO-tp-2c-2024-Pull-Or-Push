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
PtrFuncionIngresoReady ingresar_a_ready;
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
pthread_mutex_t mutex_hilo_exec;
pthread_mutex_t mutex_cola_blocked_memory_dump;
pthread_mutex_t mutex_cola_exit;
pthread_mutex_t mutex_procesos_activos;
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

bool enviar_nuevo_hilo_a_memoria() {
    //
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

t_pcb* buscar_pcb_por_pid(t_list* lista_de_pcb, int pid) {

	bool _es_el_pcb_buscado(t_pcb* pcb) {
		return pcb->pid == pid;
	}

	t_pcb* pcb_encontrado = NULL;
	pcb_encontrado = list_find(lista_de_pcb, (void*)_es_el_pcb_buscado);
	return pcb_encontrado;
}

void ingresar_a_ready_fifo(t_tcb* tcb) {
    list_add(cola_ready_unica, tcb);
    sem_post(&sem_cola_ready_unica);
}

void ingresar_a_ready_prioridades(t_tcb* tcb) {
    bool _hilo_tiene_menor_prioridad(t_tcb* tcb1, t_tcb* tcb2) {
        return tcb1->prioridad < tcb2->prioridad;
    }

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

// ====================================================
// =======  DESARROLLANDO  ============================
// ====================================================

void finalizar_hilo(t_tcb* tcb) {
	if(tcb->tid == 0) { // (if es Hilo main)

	} else {
		
	}
}

void liberar_joineados(t_tcb* tcb) {

}

void liberar_mutexes(t_tcb* tcb) {

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
