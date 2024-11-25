#include "utils.h"


// ==========================================================================
// ====  Variables globales:  ===============================================
// ==========================================================================
const int BYTES_ACCESO = 4;

int socket_cpu = 1;
int socket_escucha = 1;

t_config *config; 
t_log *log_memoria_oblig; 
t_log *log_memoria_gral; 

bool fin_programa = 0;

t_list* procesos_cargados; // sus elementos van a ser de tipo t_pcb_mem
pthread_mutex_t mutex_procesos_cargados;

t_memoria_particionada* memoria;
pthread_mutex_t mutex_memoria;

t_contexto_de_ejecucion_mem* contexto_ejecucion;
// pthread_mutex_t mutex_contexto_ejecucion; // lo comento porque solo el main va a acceder

// ==========================================================================
// ====  Funciones Internas:  ===============================================
// ==========================================================================

bool iniciar_memoria()
{
    char* str_auxiliar;

    memoria = malloc(sizeof(t_memoria_particionada));

    // distingue tipo de particiones
    str_auxiliar = config_get_string_value(config, "ESQUEMA");
    if (strcmp(str_auxiliar, "FIJAS") == 0){
        memoria->particiones_dinamicas = false;
    } else if (strcmp(str_auxiliar, "DINAMICAS")){
        memoria->particiones_dinamicas = true;
    } else {
        log_error(log_memoria_gral, "Error obtener el esquema de particiones");
        return false;
    }
    free(str_auxiliar);

    // distingue algoritmo
    str_auxiliar = config_get_string_value(config, "ALGORITMO_BUSQUEDA");
    if (strcmp(str_auxiliar, "FIRST") == 0){
        memoria->algorit_busq = FIRST_FIT;
    } else if (strcmp(str_auxiliar, "BEST")){
        memoria->algorit_busq = BEST_FIT;
    } else if (strcmp(str_auxiliar, "WORST")){
        memoria->algorit_busq = WORST_FIT; 
    } else {
        log_error(log_memoria_gral, "Error al obtener el algoritmo busqueda");
        return false;
    }
    free(str_auxiliar);

    memoria->tamano_memoria = config_get_int_value(config, "TAM_MEMORIA");
    
    // cargo particiones (considera fijas-dinamicas)
    memoria->lista_particiones = crear_lista_de_particiones();

    // si no tiene particiones aborta
    if (memoria->lista_particiones == NULL)
    {
        log_error(log_memoria_gral, "Memoria sin particiones, abortando");
        free(memoria);
        return false;
    }

    // reservo espacio usuario
    memoria->espacio_usuario = malloc(memoria->tamano_memoria);
    pthread_mutex_init(&mutex_memoria, NULL);

    // inicio lista procesos y su mutex
    procesos_cargados = list_create();
    pthread_mutex_init(&mutex_procesos_cargados, NULL);
    // inicio contexto_ejecucion
    contexto_ejecucion = malloc(sizeof(t_contexto_de_ejecucion_mem));
    
    log_debug(log_memoria_gral, "Memoria iniciada correctamente");
    return true;
}

t_tcb_mem* iniciar_tcb(int pid, int tid, char* ruta_script)
{
    t_tcb_mem* tcb_new = malloc(sizeof(t_tcb_mem));
    tcb_new->tid = tid;
    tcb_new->PC = 0;
    tcb_new->registros.AX = 0;
    tcb_new->registros.BX = 0;
    tcb_new->registros.CX = 0;
    tcb_new->registros.DX = 0;
    tcb_new->registros.EX = 0;
    tcb_new->registros.FX = 0;
    tcb_new->registros.GX = 0;
    tcb_new->registros.HX = 0;
    tcb_new->instrucciones = cargar_instrucciones(ruta_script, pid, tid);

    if (tcb_new->instrucciones == NULL)
    {
        free(tcb_new);
        log_error(log_memoria_gral, 
                    "proceso %d : thread %d: ERROR: instrucciones no cargadas. Abortando creacion de tcb",pid, tid);
        return NULL;
    }
    
    log_debug(log_memoria_gral, 
                    "Creado el thread %d del proceso %d", tid, pid);
    return tcb_new;
}

t_pcb_mem* iniciar_pcb(int pid, int tamanio, char* ruta_script_tid_0)
{
    t_pcb_mem* pcb_new = malloc(sizeof(t_pcb_mem));
    t_tcb_mem* tcb_0 = NULL;

    // busca si hay particion libre (din-fijas) - deja protegida la memoria mientras busca particiones (revisar si no es "Mucha" SC)
    pthread_mutex_lock(&mutex_memoria);
    pcb_new->particion = particion_libre(tamanio);
    pthread_mutex_unlock(&mutex_memoria);

    if (pcb_new->particion == NULL) // si no hay particion aborto
    {
        free(pcb_new);
        log_debug(log_memoria_gral, 
                    "No se le asigno una particion al proceso %d, abortando creacion del pcb",pid);
        return NULL;
    }
    // inicializo resto del pcb
    pcb_new->pid = pid;
    pcb_new->lista_tcb = list_create();
    pthread_mutex_init(&(pcb_new->sem_p_mutex), NULL);

    // inicializo el tcb-> tid=0
    tcb_0 = iniciar_tcb(pid, 0, ruta_script_tid_0);
    // si no se pudo iniciar aborto
    if (tcb_0 == NULL)
    {
        free(pcb_new->lista_tcb);
        pthread_mutex_destroy(&(pcb_new->sem_p_mutex));
        free(pcb_new->particion);
        free(pcb_new);
        log_error(log_memoria_gral, 
                    "ERROR: thread 0 del proceso %d no pudo ser iniciado. Abortando creacion de pcb",pid);
        return NULL;
    }

    list_add(pcb_new->lista_tcb, tcb_0);

    // este log luego deberia cambiarse por un log obligatorio
    log_debug(log_memoria_gral, 
                    "Creado el proceso %d, junto con su thread 0",pid);
    return pcb_new;
}

bool cargar_contexto_ejecucion(int pid, int tid)
{
    t_pcb_mem* pcb = NULL;
    t_tcb_mem* tcb = NULL;
    bool nuevo_pcb = false;

    // buscando pcb
    if (pid != contexto_ejecucion->pcb->pid)
    {
        pcb = obtener_pcb(pid);

        if (pcb == NULL){
            log_error(log_memoria_gral, 
                        "ERROR: pid %d no se encuentra en la lista de procesos cargados. No se pudo cargar el contexto de ejecución",
                        pid);
            return false;
        }

        nuevo_pcb = true;
    }

    // buscando tcb
    if (nuevo_pcb || tid != contexto_ejecucion->tcb->tid)
    {
        // verifico si cambio el pcb o no (si no cambio busco el tid desde el contexto ya cargado)
        if (nuevo_pcb)
            tcb = obtener_tcb(tid, pcb->lista_tcb);
        else
            tcb = obtener_tcb(tid, contexto_ejecucion->pcb->lista_tcb);

        if (tcb == NULL){
                    log_error(log_memoria_gral, 
                        "ERROR: tid %d del pcb %d no se encuentra en su lista de hilos cargados. No se pudo cargar el contexto de ejecución",
                        tid, pid);
            return false;
        }
    }
      
    // cargando a var global
    if (pcb != NULL)
        contexto_ejecucion->pcb = pcb;
    if (tcb != NULL)
        contexto_ejecucion->tcb = tcb;
    return true;
}

bool actualizar_contexto_ejecucion(t_list* nuevo_pedido_raw)
{
    void* data;

    // actualizo contexto
    data = list_get(nuevo_pedido_raw, 0);
    contexto_ejecucion->tcb->PC = *(uint32_t*)data;
    data = list_get(nuevo_pedido_raw, 1);
    contexto_ejecucion->tcb->registros.AX = *(uint32_t*)data;
    data = list_get(nuevo_pedido_raw, 2);
    contexto_ejecucion->tcb->registros.BX = *(uint32_t*)data;
    data = list_get(nuevo_pedido_raw, 3);
    contexto_ejecucion->tcb->registros.CX = *(uint32_t*)data;
    data = list_get(nuevo_pedido_raw, 4);
    contexto_ejecucion->tcb->registros.DX = *(uint32_t*)data;
    data = list_get(nuevo_pedido_raw, 5);
    contexto_ejecucion->tcb->registros.EX = *(uint32_t*)data;
    data = list_get(nuevo_pedido_raw, 6);
    contexto_ejecucion->tcb->registros.FX = *(uint32_t*)data;
    data = list_get(nuevo_pedido_raw, 7);
    contexto_ejecucion->tcb->registros.GX = *(uint32_t*)data;
    data = list_get(nuevo_pedido_raw, 8);
    contexto_ejecucion->tcb->registros.HX = *(uint32_t*)data;

    log_debug(log_memoria_gral, "Contexto Actualizado: PID: %d - TID: %d - PC: %d - AX: %d - BX: %d - CX: %d - DX: %d - EX: %d - FX: %d - GX: %d - HX: %d",
        contexto_ejecucion->pcb->pid,
        contexto_ejecucion->tcb->tid,
        contexto_ejecucion->tcb->PC,
        contexto_ejecucion->tcb->registros.AX,
        contexto_ejecucion->tcb->registros.BX,
        contexto_ejecucion->tcb->registros.CX,
        contexto_ejecucion->tcb->registros.DX,
        contexto_ejecucion->tcb->registros.EX,
        contexto_ejecucion->tcb->registros.FX,
        contexto_ejecucion->tcb->registros.GX,
        contexto_ejecucion->tcb->registros.HX);

    // LOG OBLIGATORIO
    log_info(log_memoria_oblig, "## Contexto Actualizado - (PID:TID) - (%d:%d)",
    contexto_ejecucion->pcb->pid, contexto_ejecucion->tcb->tid);
    
    retardo_operacion();
    return true;
}

char* obtener_instruccion(uint32_t num_instruccion)
{
    // como instruccion va de 0 en adelante la instruccion 4 (5 instruccion) 
    // se obtiene con list_get(... , 4) [4 posicion de la lista q es el 5to elemento]
    char* instruccion = NULL;
    instruccion = (char*) list_get(contexto_ejecucion->tcb->instrucciones,
                                    num_instruccion);
    // actualizo el PC del tcb
    contexto_ejecucion->tcb->PC = num_instruccion;

    // emito para testear la instruccion                                
    log_debug(log_memoria_gral,"Intruccion %d: %s", num_instruccion, instruccion);
    return instruccion;
}

char* mem_lectura (unsigned int desplazamiento)
{
    char* data = malloc (BYTES_ACCESO);// bytes de lectura

    // apunto al espacio de usuario
    void* aux_direccion = memoria->espacio_usuario; 
    // me muevo a la particion del proceso en ejecucion
    aux_direccion += contexto_ejecucion->pcb->particion->base;

    // Lo siguiente es solo a motivo de debug 
    void* base_part = aux_direccion;
    void* limite_part = base_part + contexto_ejecucion->pcb->particion->limite;

    // me desplazo al byte solicitado
    aux_direccion += desplazamiento;

    log_debug(log_memoria_gral, "ACCESO_LECTURA - PID: %d - TID: %d - Base: %d - Limite: %d - Desplazamiento: %d",
                                contexto_ejecucion->pcb->pid, contexto_ejecucion->tcb->tid,
                                contexto_ejecucion->pcb->particion->base,contexto_ejecucion->pcb->particion->limite,
                                desplazamiento);
    log_debug(log_memoria_gral, "DIR Espacio Usuario (REAL) INI: %d - FIN: %d - Base (REAL): %d - Limite (REAL): %d - DIR LECTURA (real): %d",
                                memoria->espacio_usuario, (memoria->espacio_usuario + memoria->tamano_memoria -1),
                                base_part, limite_part, aux_direccion);
    
    
    memcpy(data, aux_direccion, BYTES_ACCESO);


    log_debug(log_memoria_gral, "Resultado ACCESO_LECTURA: %s", data);
    
    // LOG OBLIGATORIO
    log_info(log_memoria_oblig, "## Lectura - (PID:TID) - (%d:%d) - Dir. Física: %d - Tamaño: %d",
                                contexto_ejecucion->pcb->pid, contexto_ejecucion->tcb->tid,
                                (contexto_ejecucion->pcb->particion->base + desplazamiento),
                                BYTES_ACCESO);

    retardo_operacion();
    return data;
}

bool mem_escritura (unsigned int desplazamiento, void* data)
{
    // apunto al espacio de usuario
    void* aux_direccion = memoria->espacio_usuario; 
    // me muevo a la particion del proceso en ejecucion
    aux_direccion += contexto_ejecucion->pcb->particion->base;

    // Lo siguiente es solo a motivo de debug 
    void* base_part = aux_direccion;
    void* limite_part = base_part + contexto_ejecucion->pcb->particion->limite;

    // me desplazo al byte solicitado
    aux_direccion += desplazamiento;

    log_debug(log_memoria_gral, "ACCESO_ESCRITURA - PID: %d - TID: %d - Base: %d - Limite: %d - Desplazamiento: %d",
                                contexto_ejecucion->pcb->pid, contexto_ejecucion->tcb->tid,
                                contexto_ejecucion->pcb->particion->base,contexto_ejecucion->pcb->particion->limite,
                                desplazamiento);
    log_debug(log_memoria_gral, "DIR Espacio Usuario (REAL) INI: %d - FIN: %d - Base (REAL): %d - Limite (REAL): %d - DIR LECTURA (real): %d",
                                memoria->espacio_usuario, (memoria->espacio_usuario + memoria->tamano_memoria -1),
                                base_part, limite_part, aux_direccion);


    memcpy(aux_direccion, data, BYTES_ACCESO);

    // LOG OBLIGATORIO
    log_info(log_memoria_oblig, "## Escritura - (PID:TID) - (%d:%d) - Dir. Física: %d - Tamaño: %d",
                                contexto_ejecucion->pcb->pid, contexto_ejecucion->tcb->tid,
                                (contexto_ejecucion->pcb->particion->base + desplazamiento),
                                BYTES_ACCESO);

    retardo_operacion();
    return true;
}

void consolidar_particion (int indice) // Ya protegida x memoria
{
    t_particion* particion_izquierda = NULL;
    t_particion* particion_derecha = NULL;

    // chequeo la particion anterior al indice
    particion_izquierda = list_get(memoria->lista_particiones, indice -1);
    if (particion_izquierda->ocupada == false)
    {
        particion_derecha = list_remove(memoria->lista_particiones, indice);
        
        // log para debug
        log_debug(log_memoria_gral, "Consolidando Memoria - Particiones [num](Base:Limite) - [%d](%d:%d) >> [%d](%d:%d)",
                                     indice-1, particion_izquierda->base, particion_izquierda->limite,
                                     indice, particion_derecha->base, particion_derecha->limite);

        // paso el limite de la particion actual a la particion anterior (extendiendola hacia adelante)
        particion_izquierda->limite = particion_derecha->limite;
        free(particion_derecha);

        // ajusto indice para el siguiente chequeo ya q la particion a la q apuntaba fue consolidada con la anterior
        indice--;

        // logueo
        log_debug(log_memoria_gral, "Particion %d consolidada - Base: %d - Limite(new): %d :", indice, 
                                    particion_izquierda->base, particion_izquierda->limite);
    }

    // chequeo la particion siguiente al indice
    particion_derecha = list_get(memoria->lista_particiones, indice +1);
    if (particion_derecha->ocupada == false)
    {
        particion_izquierda = list_remove(memoria->lista_particiones, indice);

        // log para debug
        log_debug(log_memoria_gral, "Consolidando Memoria - Particiones [num](Base:Limite) - [%d](%d:%d) >> [%d](%d:%d)",
                                     indice, particion_izquierda->base, particion_izquierda->limite,
                                     indice+1, particion_derecha->base, particion_derecha->limite);

        // paso la base de la particion actual a la particion siguiente (extiendola hacia atras)
        particion_derecha->base = particion_izquierda->base;
        free(particion_izquierda);

        // logueo
        log_debug(log_memoria_gral, "Particion %d consolidada - Base: %d - Limite(new): %d :", indice, 
                                    particion_derecha->base, particion_derecha->limite);
    }

    /* RESUMEN VISUAL:
        P_Izq->libre y P_Der->Ocupada    ==>   P_Izq->base (=)                ||  P_Izq->limite = P_Indice->limite
        P_Izq->Ocupada y P_Der->libre    ==>   P_Der->base = P_indice->base   ||  P_Der->limite (=)  
        P_Izq->libre y P_Der->libre      ==>   P_Izq->base (=)                ||  P_Izq->limite = P_Der->limite 
        P_Izq->Ocupada y P_Der->Ocupada  ==>   NADA  
    */
}

// ==========================================================================
// ====  Funciones Externas:  ===============================================
// ==========================================================================

// Kernel - Memoria

void rutina_crear_proceso(t_list* param, int socket_cliente)
{
    void* aux; // para recibir parametros
    int pid;
    int tamanio;
    char* ruta;

    // descargo parametros
    aux = list_get(param, 0);
    pid = *(int*) aux;
    aux = list_get(param, 1);
    tamanio = *(int*) aux;
    aux = list_get(param, 2);
    ruta = aux;

    t_pcb_mem* pcb_new = iniciar_pcb(pid, tamanio, ruta);

    if (pcb_new != NULL) {
        // agrego pcb a la lista
        pthread_mutex_lock(&mutex_procesos_cargados);
        list_add(procesos_cargados, pcb_new);
        pthread_mutex_unlock(&mutex_procesos_cargados);

        enviar_mensaje("OK", socket_cliente);
        
        // LOG OBLIGATORIO
        log_info(log_memoria_oblig, "## Proceso Creado-  PID: %d - Tamaño: %d", pid, tamanio);
    } else {
        enviar_mensaje("INSUFICIENTE/ERROR", socket_cliente);
    }
}

void rutina_finalizar_proceso(int socket_cliente)
{
    // trabaja sobre el proceso que se encuentre actualemnte en contexto de ejecucion
    t_tcb_mem* tcb;
    t_particion* particion_liberada = NULL;
    int pid;


    // Limpiando cada TCB perteneciente al proceso y sus estrucutras
    pthread_mutex_lock(&(contexto_ejecucion->pcb->sem_p_mutex));
    
    log_debug(log_memoria_gral, "PID: %d - Se van a liberar %d TCB asociados",
                                contexto_ejecucion->pcb->pid,
                                list_size(contexto_ejecucion->pcb->lista_tcb));
    pid = contexto_ejecucion->pcb->pid; // para poder loguear luego de la destrucción

    for (int i=0; i< list_size(contexto_ejecucion->pcb->lista_tcb); i++)
    {
        tcb = list_remove(contexto_ejecucion->pcb->lista_tcb, 0);
        list_destroy_and_destroy_elements(tcb->instrucciones, free);
        free(tcb);
    }
    list_destroy_and_destroy_elements(contexto_ejecucion->pcb->lista_tcb, free); // x las dudas
    pthread_mutex_unlock(&(contexto_ejecucion->pcb->sem_p_mutex));  
    pthread_mutex_destroy(&(contexto_ejecucion->pcb->sem_p_mutex));
    contexto_ejecucion->tcb = NULL;

    // Libero la particion y la retengo en var auxiliar
    contexto_ejecucion->pcb->particion->ocupada = false;
    particion_liberada = contexto_ejecucion->pcb->particion;
    contexto_ejecucion->pcb->particion = NULL;

    // Saco el PCB de la lista y lo libero
    pthread_mutex_lock(&mutex_procesos_cargados); 
    eliminar_pcb(procesos_cargados, contexto_ejecucion->pcb->pid);
    pthread_mutex_unlock(&mutex_procesos_cargados);
    contexto_ejecucion->pcb = NULL;

    enviar_mensaje("OK", socket_cliente);

    // LOG OBLIGATORIO
    log_info(log_memoria_oblig, "## Proceso Destruido -  PID: %d - Tamaño: %d",
                                pid, (particion_liberada->limite - particion_liberada->base + 1));

    if (memoria->particiones_dinamicas == false)
        return; // si hay particiones fijas ya terminamos x lo q salimos

    /****************************** PARTICIONES DINAMICAS ***********************************************/

    // Listado de particiones actuales
    listar_particiones();

    // envio pedido de consolidacion (solo se hace de ser valido) - memoria protegida
    pthread_mutex_lock(&mutex_memoria);
    consolidar_particion(obtener_indice_particion(particion_liberada->base));
    pthread_mutex_unlock(&mutex_memoria);
}

void rutina_crear_hilo(t_list* param, int socket_cliente)
{
    // trabaja sobre el proceso que se encuentre actualemnte en contexto de ejecucion
    void* aux; // para recibir parametros
    int tid;
    char* ruta;

    // descargo parametros
    // aux = list_get(param, 0);
    // int pid = *(int*) aux;
    aux = list_get(param, 0);
    tid = *(int*) aux;
    aux = list_get(param, 1);
    ruta = aux;

    t_pcb_mem* tcb_new = iniciar_tcb(contexto_ejecucion->pcb->pid, tid, ruta);

    if (tcb_new == NULL) {
        enviar_mensaje("ERROR", socket_cliente);
    } else {
        // agrego tcb a la lista del proceso
        pthread_mutex_lock(&(contexto_ejecucion->pcb->sem_p_mutex));
        list_add(contexto_ejecucion->pcb->lista_tcb, tcb_new);
        pthread_mutex_unlock(&(contexto_ejecucion->pcb->sem_p_mutex));

        enviar_mensaje("OK", socket_cliente);

        // LOG OBLIGATORIO
        log_info(log_memoria_oblig, "## Hilo Creado - (PID:TID) - (%d:%d)", contexto_ejecucion->pcb->pid, tid);
    }
}

void rutina_finalizar_hilo(t_list* param, int socket_cliente)
{
    // trabaja sobre el proceso que se encuentre actualemnte en contexto de ejecucion
    void* aux; // para recibir parametros
    int tid;

    // descargo parametros
    // aux = list_get(param, 0);
    // int pid = *(int*) aux;
    aux = list_get(param, 0);
    tid = *(int*)aux;

    pthread_mutex_lock(&(contexto_ejecucion->pcb->sem_p_mutex));
    eliminar_tcb(contexto_ejecucion->pcb->lista_tcb, tid);
    pthread_mutex_unlock(&(contexto_ejecucion->pcb->sem_p_mutex));

    // LOG OBLIGATORIO
    log_info(log_memoria_oblig, "## Hilo Destruido - (PID:TID) - (%d:%d)", contexto_ejecucion->pcb->pid, tid);

    enviar_mensaje("OK", socket_cliente);
}

void memory_dump_fs (t_list* pedido, int socket_cliente)
{
    char* ip;
    char* puerto;
    int socket_fs;
    t_paquete* paquete;
    void* data_proceso; // va a contener todo el espacio usuario del proceso
    void* aux_mem;
    int tamanio_proceso;
    char* timestamp;

    // preparo la conexion
    ip = config_get_string_value(config, "IP_FILESYSTEM");
    puerto = config_get_string_value(config, "PUERTO_FILESYSTEM");
    socket_fs = crear_conexion(ip, puerto);
    enviar_handshake(MEMORIA, socket_fs);
    recibir_y_manejar_rta_handshake(log_memoria_gral, "Memoria", socket_fs);

    // preparo lo que voy a enviar
    tamanio_proceso = contexto_ejecucion->pcb->particion->limite - contexto_ejecucion->pcb->particion->base + 1;
    data_proceso = malloc(tamanio_proceso);
    aux_mem = memoria->espacio_usuario + contexto_ejecucion->pcb->particion->base;
    // cargo data
    pthread_mutex_lock(&mutex_memoria);
    memcpy(data_proceso, aux_mem, tamanio_proceso);
    timestamp = temporal_get_string_time("%H:%M:%S:%MS"); // revisar en libreria si hay q cambiar el formato
    pthread_mutex_unlock(&mutex_memoria);

    // creo el paquete y lo envio
    paquete = crear_paquete(MEMORY_DUMP);
    agregar_a_paquete(paquete, &(contexto_ejecucion->pcb->pid), sizeof(int));
    agregar_a_paquete(paquete, &(contexto_ejecucion->tcb->tid), sizeof(int));
    agregar_a_paquete(paquete, timestamp, string_length(timestamp) + 1);
    agregar_a_paquete(paquete, &tamanio_proceso, sizeof(int));
    agregar_a_paquete(paquete, data_proceso, tamanio_proceso);       
    enviar_paquete(paquete, socket_fs);
    eliminar_paquete(paquete);

    // LOG OBLIGATORIO
    log_info(log_memoria_oblig, "## Memory Dump solicitado - (PID:TID) - (%d:%d)",
                                contexto_ejecucion->pcb->pid,contexto_ejecucion->tcb->tid);

    // recibo respuesta y la reenvio a kernel
    if (recibir_mensaje_de_rta(log_memoria_gral, "MEMORY_DUMP", socket_fs)){
        enviar_mensaje("OK", socket_cliente);
    } else {
        enviar_mensaje("ERROR FS al realizar DUMP_MEMORY", socket_cliente);
    }

    liberar_conexion(log_memoria_gral, "memoria >> FS", socket_fs);
    free(timestamp);
    free(data_proceso);
}

// CPU - Memoria

void rutina_contexto_ejecucion(t_list* param)
{
    void* data; // para recibir parametros
    int pid;
    int tid;
    bool resultado;
    t_paquete* paquete;

    data = list_get(param, 0);
    pid = *(int*)data;
    data = list_get(param, 1);
    tid = *(int*)data;

    resultado = cargar_contexto_ejecucion(pid, tid);
    if(!resultado)
    {
        paquete = crear_paquete(MENSAJE_ERROR);
        enviar_paquete(paquete, socket_cpu);
        eliminar_paquete(paquete);
        return;
    }

    // LOG OBLIGATORIO
    log_info(log_memoria_oblig, "## Contexto Solicitado - (PID:TID) - (%d:%d)",
                                contexto_ejecucion->pcb->pid, contexto_ejecucion->tcb->tid);

    retardo_operacion();
    paquete = empaquetar_contexto();
    enviar_paquete(paquete, socket_cpu);
    eliminar_paquete(paquete);
}

void rutina_acceso_lectura(t_list* param)
{
    unsigned int direccion;
    void* data; // para recibir parametros
    t_paquete* paquete;

    data = list_get(param, 0);
    direccion = *(unsigned int*) data;

    data = mem_lectura(direccion);

    paquete = crear_paquete(ACCESO_LECTURA);
    agregar_a_paquete(paquete, data, BYTES_ACCESO);
    enviar_paquete(paquete, socket_cpu);
    eliminar_paquete(paquete);
    free(data);
}

void rutina_acceso_escritura(t_list* param)
{
    unsigned int direccion;
    void* data; // para recibir parametros
    t_paquete* paquete;

    data = list_get(param, 0);
    direccion = *(unsigned int*) data;

    data = list_get(param, 1);

    mem_escritura(direccion, data);

    // podria ponerse un checkeo aca ya q mem_escritura podria devolver bool (si se hiciera captacion de errores)
    enviar_mensaje("OK", socket_cpu); 
}

// ==========================================================================
// ====  Funciones Auxiliares:  =============================================
// ==========================================================================

void retardo_operacion()
{
    unsigned int tiempo_en_microsegs = config_get_int_value(config, "RETARDO_RESPUESTA")*MILISEG_A_MICROSEG;
    usleep(tiempo_en_microsegs);
}

t_list* crear_lista_de_particiones() 
{
    char** array_particiones;
	t_list* lista_particiones = list_create();
	int puntero = 0;
    int i=0;
    t_particion* particion;

    if (memoria->particiones_dinamicas == false)
    {
        array_particiones = config_get_array_value(config, "PARTICIONES");
	    while (array_particiones[i] != NULL) 
        {
            particion = malloc(sizeof(t_particion));
            particion->base = puntero;
            puntero += atoi(array_particiones[i]); //
            particion->limite = puntero - 1; // el -1 para que cumpla el rango ej: 0->256-1 = 256 de tam_particion
            list_add(lista_particiones, particion);
            i++;
	    }
        free(array_particiones);
    }
    else
    { // crea una partición q ocupa toda la memoria
        particion = malloc(sizeof(t_particion));
        particion->base = puntero;
        puntero = memoria->tamano_memoria;
        particion->limite = puntero - 1;
        list_add(lista_particiones, particion);
    }

    // La condicion de paso ayuda a comprobar q todas las particiones sumadas formen toda la memoria... (capaz no es correcto?)
    if (puntero != memoria->tamano_memoria){
        log_debug(log_memoria_gral, "particiones no completan el tamano de memoria, particiones no creadas");
        list_destroy_and_destroy_elements(lista_particiones, free); //ya libera el puntero
        return NULL;
    }
	return lista_particiones;
}

t_particion* particion_libre (int tamanio)
{
    t_particion* particion;
    char* algoritmo;

    // Primero buscamos una particion que cumpla lo requerido
    switch (memoria->algorit_busq)
    {
    case FIRST_FIT:
        algoritmo = string_from_format("FIRST_FIT");
        particion = alg_first_fit(tamanio);
        break;
    case BEST_FIT:
        algoritmo = string_from_format("BEST_FIT");
        particion = alg_best_fit(tamanio);
        break;
    case WORST_FIT:
        algoritmo = string_from_format("WORST_FIT");
        particion = alg_worst_fit(tamanio);
        break;
    }

    // Si no encontro particion retorno NULL
    if (particion == NULL)
    {
        log_debug(log_memoria_gral, "No hay particion de tamaño <%d bytes> disponible.", tamanio);
        free(algoritmo);
        return particion;
    }

    // Si hay particiones fijas devolvemos la particion hallada
    if (memoria->particiones_dinamicas == false)
    {
        log_debug(log_memoria_gral, "Particion Fija hallada [%s] >> Base: %d - Limite: %d", algoritmo, particion->base, particion->limite);
        particion->ocupada = true;
        free(algoritmo);
        return particion;
    }

    // Si las particiones son dinamicas recortamos solo lo necesario
    log_debug(log_memoria_gral, "Particion Dinamica hallada [%s] >> Base: %d - Limite: %d", algoritmo, particion->base, (particion->base + tamanio - 1));
    free(algoritmo);
    return recortar_particion(particion, tamanio);
}

t_particion* alg_first_fit(int tamanio) // devuelve directamente la referencia a una particion de la lista
{
    t_particion* particion = NULL;
    t_particion* aux; 
    unsigned int tam_aux;
    
    for (int i=0; i < list_size(memoria->lista_particiones); i++)
    {
        aux = list_get(memoria->lista_particiones, i);
        tam_aux = aux->limite - aux->base +1;
        // si no esta ocupada y su tamaño es suficiente o sobra (la primera q encuentre)
        if (aux->ocupada == false && tam_aux >= tamanio)
        {
            particion = aux;
            return particion;
        }
        log_debug(log_memoria_gral, "Particion ocupada [FIRST_FIT] >> Base: %d - Limite: %d", aux->base,aux->limite);
    }

    return particion; // si no encontro va a retornar NULL
}

t_particion* alg_best_fit(int tamanio) // devuelve directamente la referencia a una particion de la lista
{
    t_particion* particion = NULL;
    t_particion* aux;
    unsigned int tam_aux;
    unsigned int tam_part_elegida = memoria->tamano_memoria + 1; 
    // un valor q asegura q sea remplazado aunque solo haya una particion dinamica q ocupe toda la memoria
    
    for (int i=0; i < list_size(memoria->lista_particiones); i++)
    {
        aux = list_get(memoria->lista_particiones, i);
        tam_aux = aux->limite - aux->base +1;
        // si no esta ocupada, su tamaño es suficiente o sobra y ademas "su tamaño" es menor a la ultima encontrada
        if (aux->ocupada == false && tam_aux >= tamanio && tam_aux < tam_part_elegida)
        {
            tam_part_elegida = tam_aux;
            particion = aux;
            log_debug(log_memoria_gral, "Particion posible [BEST_FIT] >> Base: %d - Limite: %d", particion->base, particion->limite);
        }
        log_debug(log_memoria_gral, "Particion ocupada [BEST_FIT] >> Base: %d - Limite: %d", aux->base,aux->limite);
    }

    return particion; // si no encontro va a retornar NULL
}

t_particion* alg_worst_fit(int tamanio) // devuelve directamente la referencia a una particion de la lista
{
    t_particion* particion = NULL;
    t_particion* aux;
    unsigned int tam_aux;
    unsigned int tam_part_elegida = 0; // asegura q la primera valida entre
    
    for (int i=0; i < list_size(memoria->lista_particiones); i++)
    {
        aux = list_get(memoria->lista_particiones, i);
        tam_aux = aux->limite - aux->base +1;
        // si no esta ocupada, su tamaño es suficiente o sobra y ademas "su tamaño" es menor a la ultima encontrada
        if (aux->ocupada == false && tam_aux >= tamanio && tam_aux > tam_part_elegida)
        {
            tam_part_elegida = tam_aux;
            particion = aux;
            log_debug(log_memoria_gral, "Particion posible [WORST_FIT] >> Base: %d - Limite: %d", particion->base, particion->limite);
        }
        log_debug(log_memoria_gral, "Particion ocupada [WORST_FIT] >> Base: %d - Limite: %d", aux->base,aux->limite);
    }

    return particion; // si no encontro va a retornar NULL
}

t_particion* recortar_particion(t_particion* p, int tamanio)
{
    // creo nuevo elemento de la lista (nueva particion)
    t_particion* p_new = malloc(sizeof(t_particion));
    p_new->base = p->base;
    p_new->limite = tamanio -1;
    p_new->ocupada = true;

    // como la referencia ya apunta a un elemento existente lo modifico
    p->base = p_new->limite + 1;
    /*
        la particion recibida sigue estando libre, solo q su base se adelanto al final
        de la nueva particion creada...
    */

    // agrego el nuevo elemento en indice adecuado
    list_add_in_index(memoria->lista_particiones, obtener_indice_particion(p_new->base), p_new);
    /*
        como la particion recibida fue actualizada el indice obtenido va a ser el q le correspondia...
        x lo q su lugar en la lista sera tomado x p_new y pasando al siguiente indice.
    */
    
    return p_new;
}

t_list *cargar_instrucciones(char *directorio, int pid, int tid)
{
    FILE *archivo;
    size_t lineaSize = 0; // esto es necesario para que getline() funcione bien
    char *lineaInstruccion = NULL; // esto es necesario para que getline() funcione bien
    int cant_instrucciones_cargadas = 0;
    char *base_dir = config_get_string_value(config, "PATH_INSTRUCCIONES");

    log_debug(log_memoria_gral, "Cargando instrucciones del hilo %d, del proceso %d....", tid, pid);
    
    // Crear una nueva cadena para la ruta completa
    size_t tamano_ruta = strlen(base_dir) + strlen(directorio) + 1 + 1; // +1 x el "/" y +1 x el fin string
    char *dir_completa = malloc(tamano_ruta);
    if (dir_completa == NULL) {
        return NULL;
    }
    strcpy(dir_completa, base_dir);
    strcat(dir_completa, "/");
    strcat(dir_completa, directorio);

    log_debug(log_memoria_gral, "Ruta al archivo: %s", dir_completa);
    archivo = fopen(dir_completa, "r");
    free(dir_completa); // ahora podemos liberar la memoria de dir_completa

    if (archivo == NULL) {
        log_error(log_memoria_gral, "No se pudo abrir el archivo de instrucciones");
        return NULL;
    }

    t_list *lista = list_create();
    if (lista == NULL) {
        log_error(log_memoria_gral, "No se pudo crear la lista para cargar las instrucciones leidas");
        fclose(archivo);
        return NULL;
    }

    while (getline(&lineaInstruccion, &lineaSize, archivo) != -1) {
        char *instruccion_copia = strdup(lineaInstruccion); // Copiar la línea leída
        if (instruccion_copia != NULL) {
            string_trim_right(&instruccion_copia);
            list_add(lista, instruccion_copia);
            cant_instrucciones_cargadas++;
        }
    }

    log_debug(log_memoria_gral, "Se cargaron correctamente %d instrucciones para el hilo %d, del proceso %d", cant_instrucciones_cargadas, tid, pid);

    fclose(archivo);
    free(lineaInstruccion);
    return lista;
}

t_pcb_mem* obtener_pcb (int pid)
{
    int i;
    bool coincidencia = true;
    t_pcb_mem* pcb;

    i = 0;
    coincidencia = false;
    while (!coincidencia && i < list_size(procesos_cargados) )
    {
        pcb = (t_pcb_mem*) list_get(procesos_cargados, i);
        i++; 
        if (pcb->pid == pid)
            coincidencia = true;
    }
    if (!coincidencia)
        pcb = NULL;
    return pcb;
}

t_tcb_mem* obtener_tcb (int tid, t_list* lista_tcb)
{
    int i;
    bool coincidencia = false;
    t_tcb_mem* tcb;

    i = 0;
    while (!coincidencia && i < list_size(lista_tcb) )
    {
        tcb = (t_tcb_mem*) list_get(lista_tcb, i);
        i++; 
        if (tcb->tid == tid)
            coincidencia = true;
    }
    if (!coincidencia)
        tcb = NULL;
    return tcb;
}

int obtener_indice_particion(int base_objetivo)
{
    t_particion* p_aux;
    int indice;
    bool encontrada = false;

    indice = 0;

    // si la particion objetivo va a ser la primera no necesitamos chequear, su indice ES 0.
    if (base_objetivo == 0)
        encontrada = true;

    // en cuanto lo encuentre indice va a apuntar al indice que deberia tener la base_objetivo
    while (!encontrada && indice < list_size(memoria->lista_particiones))
    {
        p_aux = list_get(memoria->lista_particiones, indice);
        // Si actual->limite + 1 == nuevo->base => que actual es el indice previo al q buscamos
        if((p_aux->limite + 1) == base_objetivo)
            encontrada = true;   
        indice++;
    }
    return indice;

    /* PRUEBA ESCRITORIO
    - base_objetivo = 0   y limite anterior [ind = X](q tiene q haber sido modificada) = XX > ==> indice = 0
    - base_objetivo = 234 y limite anterior [ind = 4]("...") = 233 ==> indice = 5
    - base_objetivo = 455 y limite anterior [ind = 11] ("...") = 454 ==> indice = 12
    */
}

t_paquete* empaquetar_contexto (void)
{
    t_paquete* p = crear_paquete(CONTEXTO_EJECUCION);
    agregar_a_paquete(p, &(contexto_ejecucion->tcb->PC), sizeof(uint32_t));
    agregar_a_paquete(p, &(contexto_ejecucion->tcb->registros.AX), sizeof(uint32_t));
    agregar_a_paquete(p, &(contexto_ejecucion->tcb->registros.BX), sizeof(uint32_t));
    agregar_a_paquete(p, &(contexto_ejecucion->tcb->registros.CX), sizeof(uint32_t));
    agregar_a_paquete(p, &(contexto_ejecucion->tcb->registros.DX), sizeof(uint32_t));
    agregar_a_paquete(p, &(contexto_ejecucion->tcb->registros.EX), sizeof(uint32_t));
    agregar_a_paquete(p, &(contexto_ejecucion->tcb->registros.FX), sizeof(uint32_t));
    agregar_a_paquete(p, &(contexto_ejecucion->tcb->registros.GX), sizeof(uint32_t));
    agregar_a_paquete(p, &(contexto_ejecucion->tcb->registros.HX), sizeof(uint32_t));
    agregar_a_paquete(p, &(contexto_ejecucion->pcb->particion->base), sizeof(uint32_t));
    agregar_a_paquete(p, &(contexto_ejecucion->pcb->particion->limite), sizeof(uint32_t));
    
    return p;
}

void eliminar_tcb( t_list* lista, int tid)
{
    int i;
    bool coincidencia = false;
    t_tcb_mem* tcb;

    i = 0;
    while (!coincidencia && i < list_size(lista) )
    {
        tcb = (t_tcb_mem*) list_get(lista, i);
        i++; 
        if (tcb->tid == tid)
            coincidencia = true;
    }
    if (!coincidencia){
        tcb = NULL;
        log_error(log_memoria_gral, "ERROR - TID: %d - No se encuentra para PID: %d - Imposible finalizarlo",
                                    tid, contexto_ejecucion->pcb->pid);
        return;
    }
    i--; // lo vuelvo al anterior

    tcb = list_remove(lista, i);
    list_clean_and_destroy_elements(tcb->instrucciones, free);
    free(tcb);
}

void eliminar_pcb( t_list* lista, int pid)
{
    // ya se hicieron los free necesarios antes de esta funcion
    int i;
    bool coincidencia = false;
    t_pcb_mem* pcb;

    i = 0;
    while (!coincidencia && i < list_size(lista) )
    {
        pcb = (t_pcb_mem*) list_get(lista, i);
        i++; 
        if (pcb->pid == pid)
            coincidencia = true;
    }
    if (!coincidencia){
        pcb = NULL;
        log_error(log_memoria_gral, "ERROR - PID: %d - No se encuentra en memoria - Imposible finalizarlo", pid);
        return;
    }
    i--; // lo vuelvo al anterior

    pcb = list_remove(lista, i);
    free(pcb);
}

void iniciar_logs(bool testeo)
{
    log_memoria_gral = log_create("memoria_general.log", "Memoria", testeo, LOG_LEVEL_DEBUG);
    
    // Log obligatorio
    char * nivel;
    nivel = config_get_string_value(config, "LOG_LEVEL");
    log_memoria_oblig = log_create("memoria_obligatorio.log", "Memoria", true, log_level_from_string(nivel));

    /*
        Ver luego si se quiere manejar caso de que el config este mal () y como cerrar el programa.
    */

    free(nivel);		
}

void listar_particiones()
{
    t_particion* p;

    for (int i=0; i < list_size(memoria->lista_particiones); i++)
    {
        p = list_get(memoria->lista_particiones, i);
        if (p->ocupada)
            log_debug(log_memoria_gral, "Particion [%d] - Base: %d - Limite: %d - Estado: Ocupada", i, p->base, p->limite);
        else
            log_debug(log_memoria_gral, "Particion [%d] - Base: %d - Limite: %d - Estado: Libre", i, p->base, p->limite);
    }
}

void terminar_programa()
{
	// Y por ultimo, hay que liberar lo que utilizamos (conexion, log y config) 
	 // con las funciones de las commons y del TP mencionadas en el enunciado /
	liberar_conexion(log_memoria_gral, "CPU", socket_cpu);
	liberar_conexion(log_memoria_gral, "Mi propio Servidor Escucha", socket_escucha);
	log_destroy(log_memoria_oblig);
	log_destroy(log_memoria_gral);
	config_destroy(config);
}