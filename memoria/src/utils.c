#include "utils.h"

// ==========================================================================
// ====  Variables globales:  ===============================================
// ==========================================================================

int socket_cpu = 1;
int socket_escucha = 1;

t_config *config; 
t_log *log_memoria_oblig; 
t_log *log_memoria_gral; 

bool fin_programa = 0;

t_list* procesos_cargados; // sus elementos van a ser de tipo t_pcb_mem
pthread_mutex_t mutex_procesos_cargados;

t_memoria_particionada* memoria;

t_contexto_de_ejecucion* contexto_ejecucion;
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

    // inicio lista procesos y su mutex
    procesos_cargados = list_create();
    pthread_mutex_init(&mutex_procesos_cargados, NULL);
    // inicio contexto_ejecucion
    pthread_mutex_init(&mutex_contexto_ejecucion, NULL);
    contexto_ejecucion = malloc(sizeof(t_contexto_de_ejecucion));
    
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
                    "Creado el thread %d del proceso %d", pid, tid);
    return tcb_new;
}

t_pcb_mem* iniciar_pcb(int pid, int tamanio, char* ruta_script_tid_0)
{
    t_pcb_mem* pcb_new = NULL;
    t_tcb_mem* tcb_0 = NULL;

    // busca si hay particion libre (din-fijas) - x ahora STUB afirmativo
    pcb_new->particion = particion_libre(tamanio);

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

    // inicializo el tcb-> tid=0
    tcb_0 = iniciar_tcb(pid, 0, ruta_script_tid_0);
    // si no se pudo iniciar aborto
    if (tcb_0 == NULL)
    {
        free(pcb_new->lista_tcb);
        free(pcb_new->particion);
        free(pcb_new);
        log_error(log_memoria_gral, 
                    "ERROR: thread 0 del proceso %d no pudo ser iniciado. Abortando creacion de pcb",pid);
        return NULL;
    }

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
    // como actualizar un contexto de ejecucion implica q previamente lo pidio no se necesita chequear nada
    // por lo tanto simplemente descargamos el pedido y lo cargamos en el contexto de ejecucion (es decir en el pcb y tcb segun corresponda)
}

char* obtener_instruccion(int num_instruccion)
{
    // como implica q antes se obtuvo el contexto de ejecucion, no se necesita cambiar el contexto
    // se puede acceder desde el contexto al tcb y hacer list_get(num_instruccion)
    // y lo q tiene devolverlo
}

// ==========================================================================
// ====  Funciones Externas:  ===============================================
// ==========================================================================

bool memory_dump_fs (t_list* pedido)
{
    char* ip;
    char* puerto;
    int socket_fs;
    t_paquete* paquete;

    ip = config_get_string_value(config, "IP_FILESYSTEM");
    puerto = config_get_string_value(config, "PUERTO_FILESYSTEM");
    socket_fs = crear_conexion(ip, puerto);
    enviar_handshake(MEMORIA, socket_fs);
    recibir_y_manejar_rta_handshake(log_memoria_gral, "Memoria", socket_fs);


    paquete = crear_paquete(MEMORY_DUMP);
    // agregar el pid, tid, y el tiempo actual al iniciar el memory_dump?
    // hay q agregar al paquete TODO lo contenido en la particion del proceso
    // puede ser en un void*
    enviar_paquete(paquete, socket_fs);
    recibir_mensaje(socket_fs);
    // tendria q modificarse para sacar información
    // if (correcta) return true else return false
    liberar_conexion(log_memoria_gral, "memoria >> FS", socket_fs);
    return true;
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

t_particion* particion_libre (int tamanio) // PENDIENTE HASTA DESARROLLO ESPACIO USUARIO
{
    t_particion* particion = malloc(t_particion);

    // no verifica nada
    particion->base = 0;
    particion->limite = tamanio - 1;

    return particion;
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
    size_t tamano_ruta = strlen(base_dir) + strlen(directorio) + 1;
    char *dir_completa = malloc(tamano_ruta);
    if (dir_completa == NULL) {
        return NULL;
    }
    strcpy(dir_completa, base_dir);
    strcat(dir_completa, directorio);
    
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
    bool coincidencia = true;
    t_tcb_mem* tcb;

    i = 0;
    coincidencia = false;
    while (!coincidencia && i < list_size(lista_tcb) )
    {
        tcb = (t_tcb_mem*) list_get(lista_tcbs, i);
        i++; 
        if (pcb->pid == tid)
            coincidencia = true;
    }
    if (!coincidencia)
        tcb = NULL;
    return tcb;
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