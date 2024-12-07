#include <utils.h>

// ==========================================================================
// ====  Variables globales:  ===============================================
// ==========================================================================

int socket_escucha_puerto_dispatch = 1;
int socket_escucha_puerto_interrupt = 1;
int socket_kernel_dispatch = 1;
int socket_kernel_interrupt = 1;
int socket_memoria = 1;

t_log* log_cpu_oblig; 
t_log* log_cpu_gral; 
t_config* config;

bool se_hizo_jnz = false;

t_contexto_exec contexto_exec;

bool hay_interrupcion;
pthread_mutex_t mutex_interrupcion;

bool desalojado;
bool segmentation_fault;

t_dictionary* diccionario_reg;

// ==========================================================================
// ====  Funciones Internas:  ===============================================
// ==========================================================================

t_list* decode (char* instruccion)
{
    char **arg = string_split(instruccion, " ");
    t_list* parametros = list_create();
    int *var_instruccion = malloc(sizeof(int));
    *var_instruccion = DESCONOCIDA;
    int num_arg = string_array_size(arg);

    // si el numero de parametros no coincide con lo esperado, desconoce la instruccion

    if (strcmp(arg[0], "SET") == 0 && num_arg == 3){
        *var_instruccion = SET;
    }
    if (strcmp(arg[0], "READ_MEM") == 0 && num_arg == 3){
        *var_instruccion = READ_MEM;
    }
    if (strcmp(arg[0], "WRITE_MEM") == 0 && num_arg == 3){
        *var_instruccion = WRITE_MEM;
    }
    if (strcmp(arg[0], "SUM") == 0 && num_arg == 3){
        *var_instruccion = SUM;
    }
    if (strcmp(arg[0], "SUB") == 0 && num_arg == 3){
        *var_instruccion = SUB;
    }
    if (strcmp(arg[0], "JNZ") == 0 && num_arg == 3){
        *var_instruccion = JNZ;
    }
    if (strcmp(arg[0], "LOG") == 0 && num_arg == 2){
        *var_instruccion = LOG;
    }
    if (strcmp(arg[0], "DUMP_MEMORY") == 0 && num_arg == 1){
        *var_instruccion = DUMP_MEMORY;
    }
    if (strcmp(arg[0], "IO") == 0 && num_arg == 2){
        *var_instruccion = IO;
    }
    if (strcmp(arg[0], "PROCESS_CREATE") == 0 && num_arg == 4){
        *var_instruccion = PROCESS_CREATE;
    }
    if (strcmp(arg[0], "THREAD_CREATE") == 0 && num_arg == 3){
        *var_instruccion = THREAD_CREATE;
    }
    if (strcmp(arg[0], "THREAD_JOIN") == 0 && num_arg == 2){
        *var_instruccion = THREAD_JOIN;
    }
    if (strcmp(arg[0], "THREAD_CANCEL") == 0 && num_arg == 2){
        *var_instruccion = THREAD_CANCEL;
    }
    if (strcmp(arg[0], "MUTEX_CREATE") == 0 && num_arg == 2){
        *var_instruccion = MUTEX_CREATE;
    }
    if (strcmp(arg[0], "MUTEX_LOCK") == 0 && num_arg == 2){
        *var_instruccion = MUTEX_LOCK;
    }
    if (strcmp(arg[0], "MUTEX_UNLOCK") == 0 && num_arg == 2){
        *var_instruccion = MUTEX_UNLOCK;
    }
    if (strcmp(arg[0], "THREAD_EXIT") == 0){
        *var_instruccion = THREAD_EXIT;
    }
    if (strcmp(arg[0], "PROCESS_EXIT") == 0){
        *var_instruccion = PROCESS_EXIT;
    }

    list_add(parametros, var_instruccion);
    log_debug(log_cpu_gral, "instruccion <%s> - codigo: %d - num param: %d", arg[0], *var_instruccion, num_arg);
    
    // si no se conoce la instruccion devuelvo
    if (*var_instruccion == DESCONOCIDA)
        return parametros;

    // paso los parametros
    for (int i = 1; i < num_arg; i++)
    {
        // if(arg[i]==NULL)  // solucionado estaba indice n de n (deberia ser n-1 de n)
        // {
        //     log_debug(log_cpu_gral, "indice: %d - de %d indices - es NULL.", i, num_arg);
        //     continue;
        // }
        list_add(parametros, string_duplicate(arg[i]));
    }

    //faltaba liberar arg
    string_array_destroy(arg);

    return parametros;    
}

uint32_t* mmu(uint32_t* dir_log)
{
    if (*dir_log > contexto_exec.Limite || (*dir_log + 4) > contexto_exec.Limite)
    {
        log_error(log_cpu_gral, "ERROR: direccion logica %d - limite: %d", dir_log, contexto_exec.Limite);
        segmentation_fault = true;
        return 0;
    }

    uint32_t* dir_fis = malloc(sizeof(uint32_t));
    *dir_fis = contexto_exec.Base + *dir_log;

    log_debug(log_cpu_gral, "MMU - Base: %d - Direccion: %d - Limite: %d", contexto_exec.Base, *dir_fis, contexto_exec.Limite);

    return dir_fis;
}

void instruccion_set (t_list* param)
{

    char* str_r = (char*)list_get(param, 0);
    char* valor = (char*)list_get(param, 1);
    
    // para revisar si coincide hubo algun error al cambiar contexto
    log_debug(log_cpu_gral, "PID: %d - TID: %d - Ejecutando: %s - %s %s", contexto_exec.pid, contexto_exec.tid, "SET", str_r, valor);

    // LOG OBLIGATORIO
    log_info(log_cpu_oblig, "## TID: %d - Ejecutando: %s - %s %s", contexto_exec.tid, "SET", str_r, valor);


	void* registro = dictionary_get(diccionario_reg, str_r);
	*(uint32_t*)registro = (uint32_t)atoi(valor);
	
    log_debug(log_cpu_gral, "Se hizo SET de %s en %s", str_r, valor); // temporal. Sacar luego
    
}

void instruccion_sum (t_list* param)
{
    char* str_r_dstn = (char*)list_get(param, 0);
    char* str_r_orig = (char*)list_get(param, 1);
    
    // para revisar si coincide hubo algun error al cambiar contexto
    log_debug(log_cpu_gral, "PID: %d - TID: %d - Ejecutando: %s - %s %s", contexto_exec.pid, contexto_exec.tid, "SUM", str_r_dstn, str_r_orig);

    // LOG OBLIGATORIO
    log_info(log_cpu_oblig, "## TID: %d - Ejecutando: %s - %s %s", contexto_exec.tid, "SUM", str_r_dstn, str_r_orig);

	void* reg_dstn = dictionary_get(diccionario_reg, str_r_dstn);
    void* reg_orig = dictionary_get(diccionario_reg, str_r_orig);

	*(uint32_t*)reg_dstn = *(uint32_t*)reg_dstn + *(uint32_t*)reg_orig;

	log_debug(log_cpu_gral, "Se hizo SUM de %s y %s, nuevo valor de %s: %d", str_r_dstn, str_r_orig, str_r_dstn, *(uint32_t*)reg_dstn); // temporal. Sacar luego
    
}

void instruccion_sub (t_list* param)
{
    char* str_r_dstn = (char*)list_get(param, 0);
    char* str_r_orig = (char*)list_get(param, 1);
    
    // para revisar si coincide hubo algun error al cambiar contexto
    log_debug(log_cpu_gral, "PID: %d - TID: %d - Ejecutando: %s - %s %s", contexto_exec.pid, contexto_exec.tid, "SUB", str_r_dstn, str_r_orig);

    // LOG OBLIGATORIO
    log_info(log_cpu_oblig, "## TID: %d - Ejecutando: %s - %s %s", contexto_exec.tid, "SUB", str_r_dstn, str_r_orig);

	void* reg_dstn = dictionary_get(diccionario_reg, str_r_dstn);
    void* reg_orig = dictionary_get(diccionario_reg, str_r_orig);

	*(uint32_t*)reg_dstn = *(uint32_t*)reg_dstn - *(uint32_t*)reg_orig;
    
	log_debug(log_cpu_gral, "Se hizo SUB de %s y %s", str_r_dstn, str_r_orig); // temporal. Sacar luego
    
}

void instruccion_jnz (t_list* param)
{
    char* str_r = (char*)list_get(param, 0);
    char* data_como_string = list_get(param, 1);
    uint32_t num_inst = atoi(data_como_string);
    
    // para revisar si coincide hubo algun error al cambiar contexto
    log_debug(log_cpu_gral, "PID: %d - TID: %d - Ejecutando: %s - %s %d", contexto_exec.pid, contexto_exec.tid, "JNZ", str_r, num_inst);

    // LOG OBLIGATORIO
    log_info(log_cpu_oblig, "## TID: %d - Ejecutando: %s - %s %d", contexto_exec.tid, "JNZ", str_r, num_inst);

	void* reg = dictionary_get(diccionario_reg, str_r);

    if(*(uint32_t*)reg == 0){
        log_debug(log_cpu_gral, "El registro %s tiene valor 0, no se realiza JNZ", str_r);
        return;
    }

    // como no es 0 reemplazo el PC
    contexto_exec.PC = num_inst;
    se_hizo_jnz = true;
    
	log_debug(log_cpu_gral, "Se hizo JNZ a instruccion %d", num_inst); // temporal. Sacar luego
    
}

void instruccion_log (t_list* param)
{
    char* str_r = (char*)list_get(param, 0);
    
    // para revisar si coincide hubo algun error al cambiar contexto
    log_debug(log_cpu_gral, "PID: %d - TID: %d - Ejecutando: %s - %s", contexto_exec.pid, contexto_exec.tid, "LOG", str_r);

    // LOG OBLIGATORIO
    log_info(log_cpu_oblig, "## TID: %d - Ejecutando: %s - %s", contexto_exec.tid, "LOG", str_r);

	void* reg = dictionary_get(diccionario_reg, str_r);
    
    // no sabria si asi es como quieren el q se realiza el log ya q no nos dieron formato...
    log_info(log_cpu_oblig, "Registro %s : %d", str_r, *(uint32_t*)reg);
    
	log_debug(log_cpu_gral, "Se hizo LOG del registro (%s)", str_r); // temporal. Sacar luego
    
}

// ==========================================================================
// ====  Funciones Externas:  ===============================================
// ==========================================================================

char* fetch (void)
{
    t_paquete* paq = crear_paquete(OBTENER_INSTRUCCION);
    // agregar_a_paquete(paq, &(contexto_exec.pid), sizeof(int)); // no se requieren ya que la instruccion se toma del contexto
    // agregar_a_paquete(paq, &(contexto_exec.tid), sizeof(int));
    agregar_a_paquete(paq, &(contexto_exec.PC), sizeof(uint32_t));
    enviar_paquete(paq, socket_memoria);
    eliminar_paquete(paq);
    // logs grales y obligatorios
    log_info(log_cpu_gral, "## PID: %d - TID: %d - FETCH - Program Counter: %d", contexto_exec.pid, contexto_exec.tid, (int)contexto_exec.PC);
    log_info(log_cpu_oblig, "## TID: %d - FETCH - Program Counter: %d", contexto_exec.tid, (int)contexto_exec.PC);
    
    if(recibir_codigo(socket_memoria) != MENSAJE){
        log_error(log_cpu_gral,"Error en respuesta de siguiente instruccion");
    }
    char* instruccion = recibir_mensaje(socket_memoria);

    // log gral
    log_debug(log_cpu_gral, "Instruccion recibida: %s", instruccion);

    return instruccion;
}

void recibir_pedido_ejecucion(void)
{
    desalojado = false; // como ya estoy seguro q tengo contexto

    t_list* pedido;
    int codigo;
    void* data;
    int pid;
    int tid;
    bool resultado_contexto = true;
    log_debug(log_cpu_gral, "Esperando pedido ejecución");

    codigo = recibir_codigo(socket_kernel_dispatch);

    switch (codigo)
    {
    case EJECUCION:
        log_debug(log_cpu_gral, "Pedido ejecucion Kernel");
        break;
    case -1:
        log_error(log_cpu_gral, "Kernel se desconecto, finalizando modulo cpu");
        exit(3);
        break;
    
    default:
        log_error(log_cpu_gral, "Error al recibir contexto de ejecucion");
        exit(3);
        break;
    }

    pedido = recibir_paquete(socket_kernel_dispatch);
    data = list_get(pedido, 0);
    pid = *(int*)data;
    data = list_get(pedido, 1);
    tid = *(int*)data;

    // Si habia interrupcion y desalojo a mismo pid-tid entonces interrumpo
    if (hay_interrupcion && contexto_exec.pid == pid && contexto_exec.tid == tid)
    {
        interrupcion(INTERRUPCION);
        contexto_exec.pid = -1;
        list_destroy_and_destroy_elements(pedido, free);
        return;
    }

    resultado_contexto = obtener_contexto_ejecucion(pid, tid);

    if (!resultado_contexto)
    {
        log_error(log_cpu_gral, "No hay contexto cargado abortando ejecución cpu");
        exit(3);
    }

    list_destroy_and_destroy_elements(pedido, free);
}

bool obtener_contexto_ejecucion(int pid, int tid)
{
    t_paquete* paquete;
    int codigo;
    t_list* recibido;
    void* data;

    // // si no cambio pid y tid entonces el contexto ya esta en CPU
    // if (contexto_exec.pid == pid && contexto_exec.tid == tid)
    // {
    //     log_debug(log_cpu_gral, "Contexto PID: %d - TID: %d - Ya cargado en cpu", pid, tid);
    //     return true;
    // }
    log_debug(log_cpu_gral, "Solicito contexto Ejcución <PID:TID> %d:%d",pid,tid);
    log_info(log_cpu_oblig, "## TID: %d - Solicito Contexto Ejecución", tid);

    // pido contexto
    paquete = crear_paquete(CONTEXTO_EJECUCION);
    agregar_a_paquete(paquete, &pid, sizeof(int));
    agregar_a_paquete(paquete, &tid, sizeof(int));
    enviar_paquete(paquete, socket_memoria);
    eliminar_paquete(paquete);

    codigo = recibir_codigo(socket_memoria);
    if(codigo != CONTEXTO_EJECUCION)
    {
        recibido = recibir_paquete(socket_memoria);
        list_destroy_and_destroy_elements(recibido, free);
        return false;
    }
    
    // cargo contexto
    recibido = recibir_paquete(socket_memoria);
    contexto_exec.pid = pid;
    contexto_exec.tid = tid;
    data = list_get(recibido, 0);
    contexto_exec.PC = *(uint32_t*)data;
    data = list_get(recibido, 1);
    contexto_exec.registros.AX = *(uint32_t*)data;
    data = list_get(recibido, 2);
    contexto_exec.registros.BX = *(uint32_t*)data;
    data = list_get(recibido, 3);
    contexto_exec.registros.CX = *(uint32_t*)data;
    data = list_get(recibido, 4);
    contexto_exec.registros.DX = *(uint32_t*)data;
    data = list_get(recibido, 5);
    contexto_exec.registros.EX = *(uint32_t*)data;
    data = list_get(recibido, 6);
    contexto_exec.registros.FX = *(uint32_t*)data;
    data = list_get(recibido, 7);
    contexto_exec.registros.GX = *(uint32_t*)data;
    data = list_get(recibido, 8);
    contexto_exec.registros.HX = *(uint32_t*)data;
    data = list_get(recibido, 9);
    contexto_exec.Base= *(uint32_t*)data;
    data = list_get(recibido, 10);
    contexto_exec.Limite = *(uint32_t*)data;

    list_destroy_and_destroy_elements(recibido,free);

    log_debug(log_cpu_gral, "Contexto Cargado: PID: %d - TID: %d - PC: %d - AX: %d - BX: %d - CX: %d - DX: %d - EX: %d - FX: %d - GX: %d - HX: %d - BASE: %d - LIMITE: %d",
        pid, tid, contexto_exec.PC,
        contexto_exec.registros.AX,
        contexto_exec.registros.BX,
        contexto_exec.registros.CX,
        contexto_exec.registros.DX,
        contexto_exec.registros.EX,
        contexto_exec.registros.FX,
        contexto_exec.registros.GX,
        contexto_exec.registros.HX,
        contexto_exec.Base,
        contexto_exec.Limite);

    return true;
}

void actualizar_contexto_ejecucion(void)
{
    t_paquete* paquete = empaquetar_contexto();
    enviar_paquete(paquete, socket_memoria);
    eliminar_paquete(paquete);
    log_info(log_cpu_oblig, "## TID: %d - Actualizo Contexto Ejecución", contexto_exec.tid);
    recibir_mensaje_de_rta(log_cpu_gral, "Actualizar contexto a Memoria", socket_memoria);
}

// instrucciones lecto-escritura memoria

void instruccion_read_mem (t_list* param)
{
    // actualmente esta armado para ser legible, pero podria optimizarse a usar solo 3 void* para manejar lista-registros (creo)
    char* str_r_dat = (char*)list_get(param, 0);
    char* str_r_dir = (char*)list_get(param, 1);
    t_paquete* paquete;
    t_list* respuesta;
    void* valor;
    int codigo;
    
    // para revisar si coincide hubo algun error al cambiar contexto
    log_debug(log_cpu_gral, "PID: %d - TID: %d - Ejecutando: %s - %s %s", contexto_exec.pid, contexto_exec.tid, "READ_MEM", str_r_dat, str_r_dir);
    
    // LOG OBLIGATORIO
    log_info(log_cpu_oblig, "## TID: %d - Ejecutando: %s - %s %s", contexto_exec.tid, "READ_MEM", str_r_dat, str_r_dir);

	void* registro_dat = dictionary_get(diccionario_reg, str_r_dat);
    void* registro_dir = dictionary_get(diccionario_reg, str_r_dir);

    // MMU
    valor = mmu((uint32_t*)registro_dir);
    if (segmentation_fault)
    {
        free(valor);
        syscall_process_exit(false);
        return;
    }
	
    // envio pedido lectura a memoria (mismo protocolo q antes sin pid-tid q se toman de contexto exec)
    paquete = crear_paquete(ACCESO_LECTURA);
    agregar_a_paquete(paquete, (uint32_t*)valor, sizeof(uint32_t));
    enviar_paquete(paquete, socket_memoria);
    eliminar_paquete(paquete);
    free(valor);

    // recibo respuesta mem
    codigo = recibir_codigo(socket_memoria);
    respuesta = recibir_paquete(socket_memoria);

    if (codigo != ACCESO_LECTURA){ // si hubo error logueo y salgo
        log_error(log_cpu_gral, "ERROR: Respuesta memoria diferente a lo esperado");
        list_destroy_and_destroy_elements(respuesta, free);
        return;
    }
    
    valor = list_get(respuesta, 0);
    *(uint32_t*)registro_dat = *(uint32_t*)valor;

    log_info(log_cpu_oblig, "## TID: %d - Acción: LEER - Dirección Física: %d", contexto_exec.tid, *(uint32_t*)registro_dir);
    log_debug(log_cpu_gral, "Nuevo valor registro %s: %d", str_r_dat, *(uint32_t*)registro_dat);

    list_destroy_and_destroy_elements(respuesta, free);
}

void instruccion_write_mem (t_list* param)
{
    // actualmente esta armado para ser legible, pero podria optimizarse a usar solo 3 void* para manejar lista-registros (creo)
    char* str_r_dir = (char*)list_get(param, 0);
    char* str_r_dat = (char*)list_get(param, 1);
    t_paquete* paquete;
    bool resultado;
    uint32_t* dir_fis;
    
    // para revisar si coincide hubo algun error al cambiar contexto
    log_debug(log_cpu_gral, "PID: %d - TID: %d - Ejecutando: %s - %s %s", contexto_exec.pid, contexto_exec.tid, "WRITE_MEM", str_r_dir, str_r_dat);

    // LOG OBLIGATORIO
    log_info(log_cpu_oblig, "## TID: %d - Ejecutando: %s - %s %s", contexto_exec.tid, "WRITE_MEM", str_r_dir, str_r_dat);

	void* registro_dat = dictionary_get(diccionario_reg, str_r_dat);
    void* registro_dir = dictionary_get(diccionario_reg, str_r_dir);

    log_debug(log_cpu_gral, "Valor a escribir registro %s: %d", str_r_dat, *(uint32_t*)registro_dat);

    // MMU
    dir_fis = mmu((uint32_t*)registro_dir);
    
    if (segmentation_fault)
    {
        free(dir_fis);
        syscall_process_exit(false);
        return;
    }
	
    // envio pedido lectura a memoria (mismo protocolo q antes sin pid-tid q se toman de contexto exec)
    paquete = crear_paquete(ACCESO_ESCRITURA);
    agregar_a_paquete(paquete, dir_fis, sizeof(uint32_t));
    agregar_a_paquete(paquete, (uint32_t*)registro_dat, sizeof(uint32_t));
    enviar_paquete(paquete, socket_memoria);
    eliminar_paquete(paquete);
    free(dir_fis);

    // recibo respuesta mem
    resultado = recibir_mensaje_de_rta(log_cpu_gral, "WRITE_MEM", socket_memoria);

    if (resultado)
        log_info(log_cpu_oblig, "## TID: %d - Acción: ESCRIBIR - Dirección Física: %d", contexto_exec.tid, *(uint32_t*)registro_dir);
}

// syscalls para facilitar implementacion solo pasarles directamente lo decodificado (sin el op_code)

void syscall_dump_memory (void)
{
    bool respuesta_kernel;

    // para revisar si coincide hubo algun error al cambiar contexto
    log_debug(log_cpu_gral, "PID: %d - TID: %d - Ejecutando: %s", contexto_exec.pid, contexto_exec.tid, "DUMP_MEMORY");
    
    // LOG OBLIGATORIO
    log_info(log_cpu_oblig, "## TID: %d - Ejecutando: %s", contexto_exec.tid, "DUMP_MEMORY");

    // actualizo el contexto de ejecucion en memoria
    actualizar_contexto_ejecucion();

    // devuelvo control a kernel junto con parametros q requiera
    t_paquete* paquete = crear_paquete(SYSCALL_MEMORY_DUMP);
    enviar_paquete(paquete, socket_kernel_dispatch);
    eliminar_paquete(paquete);

    desalojado = true;
}

void syscall_io (t_list* param)
{
    // variables parametros
    void* var_aux;
    char* tiempo_como_string;
    int tiempo;

    // actualizo el contexto de ejecucion en memoria
    actualizar_contexto_ejecucion();

    // devuelvo control a kernel junto con parametros q requiera
    t_paquete* paquete = crear_paquete(SYSCALL_IO);

        // descargo parametros
    var_aux = list_get(param, 0);
    tiempo_como_string = (char*)var_aux;
    tiempo = atoi(tiempo_como_string);
    agregar_a_paquete(paquete, &tiempo, sizeof(int));

    // para revisar si coincide hubo algun error al cambiar contexto (para agilizar no pongo los param (par no repetir))
    log_debug(log_cpu_gral, "PID: %d - TID: %d - Ejecutando: %s", contexto_exec.pid, contexto_exec.tid, "IO");

    // LOG OBLIGATORIO
    log_info(log_cpu_oblig, "## TID: %d - Ejecutando: %s - %d", contexto_exec.tid, "IO", tiempo);

    enviar_paquete(paquete, socket_kernel_dispatch);
    eliminar_paquete(paquete);
    
    desalojado = true;
}

void syscall_process_create (t_list* param)
{
    // variables parametros
    void* var_aux;
    char* ruta;
    char* tamanio_como_string;
    int tamanio;
    char* prioridad_como_string;
    int prioridad;

    // actualizo el contexto de ejecucion en memoria
    actualizar_contexto_ejecucion();

    // devuelvo control a kernel junto con parametros q requiera
    t_paquete* paquete = crear_paquete(SYSCALL_CREAR_PROCESO);

        // descargo parametros
    var_aux = list_get(param, 0);
    ruta = (char*)var_aux;
    agregar_a_paquete(paquete, ruta, strlen(ruta) + 1);

    var_aux = list_get(param, 1);
    tamanio_como_string = (char*)var_aux;
    tamanio = atoi(tamanio_como_string);
    agregar_a_paquete(paquete, &tamanio, sizeof(int));

    var_aux = list_get(param, 2);
    prioridad_como_string = (char*)var_aux;
    prioridad = atoi(prioridad_como_string);
    agregar_a_paquete(paquete, &prioridad, sizeof(int));

        // para revisar si coincide hubo algun error al cambiar contexto
    log_debug(log_cpu_gral, "PID: %d - TID: %d - Ejecutando: %s", contexto_exec.pid, contexto_exec.tid, "PROCESS_CREATE");

    // LOG OBLIGATORIO
    log_info(log_cpu_oblig, "## TID: %d - Ejecutando: %s - %s %d %d", contexto_exec.tid, "PROCESS_CREATE", ruta, tamanio, prioridad); 

    enviar_paquete(paquete, socket_kernel_dispatch);
    eliminar_paquete(paquete);

    desalojado = true;
}

void syscall_thread_create (t_list* param)
{
    // variables parametros
    void* var_aux;
    char* ruta;
    char* prioridad_como_string;
    int prioridad;

    // actualizo el contexto de ejecucion en memoria
    actualizar_contexto_ejecucion();

    // devuelvo control a kernel junto con parametros q requiera
    t_paquete* paquete = crear_paquete(SYSCALL_CREAR_HILO);

        // descargo parametros
    var_aux = list_get(param, 0);
    ruta = (char*)var_aux;
    agregar_a_paquete(paquete, ruta, strlen(ruta) + 1);

    var_aux = list_get(param, 1);
    prioridad_como_string = (char*)var_aux;
    prioridad = atoi(prioridad_como_string);
    agregar_a_paquete(paquete, &prioridad, sizeof(int));

    // para revisar si coincide hubo algun error al cambiar contexto
    log_debug(log_cpu_gral, "PID: %d - TID: %d - Ejecutando: %s", contexto_exec.pid, contexto_exec.tid, "THREAD_CREATE");

    // LOG OBLIGATORIO
    log_info(log_cpu_oblig, "## TID: %d - Ejecutando: %s - %s %d", contexto_exec.tid, "THREAD_CREATE", ruta, prioridad);

    enviar_paquete(paquete, socket_kernel_dispatch);
    eliminar_paquete(paquete);

    desalojado = true;
}

void syscall_thread_join (t_list* param)
{
    // variables parametros
    void* var_aux;
    char* tid_como_string;
    int tid;

    // actualizo el contexto de ejecucion en memoria
    actualizar_contexto_ejecucion();

    // devuelvo control a kernel junto con parametros q requiera
    t_paquete* paquete = crear_paquete(SYSCALL_JOIN_HILO);

        // descargo parametros
    var_aux = list_get(param, 0);
    tid_como_string = (char*)var_aux;
    tid = atoi(tid_como_string);
    agregar_a_paquete(paquete, &tid, sizeof(int));

    // para revisar si coincide hubo algun error al cambiar contexto
    log_debug(log_cpu_gral, "PID: %d - TID: %d - Ejecutando: %s", contexto_exec.pid, contexto_exec.tid, "THREAD_JOIN");

    // LOG OBLIGATORIO
    log_info(log_cpu_oblig, "## TID: %d - Ejecutando: %s - %d", contexto_exec.tid, "THREAD_JOIN", tid);

    enviar_paquete(paquete, socket_kernel_dispatch);
    eliminar_paquete(paquete);

    desalojado = true;
}

void syscall_thread_cancel (t_list* param)
{
    // variables parametros
    void* var_aux;
    char* tid_como_string;
    int tid;

    // actualizo el contexto de ejecucion en memoria
    actualizar_contexto_ejecucion();

    // devuelvo control a kernel junto con parametros q requiera
    t_paquete* paquete = crear_paquete(SYSCALL_FINALIZAR_ALGUN_HILO);

        // descargo parametros
    var_aux = list_get(param, 0);
    tid_como_string = (char*)var_aux;
    tid = atoi(tid_como_string);
    agregar_a_paquete(paquete, &tid, sizeof(int));

    // para revisar si coincide hubo algun error al cambiar contexto
    log_debug(log_cpu_gral, "PID: %d - TID: %d - Ejecutando: %s", contexto_exec.pid, contexto_exec.tid, "THREAD_CANCEL");

    // LOG OBLIGATORIO
    log_info(log_cpu_oblig, "## TID: %d - Ejecutando: %s - %d", contexto_exec.tid, "THREAD_CANCEL", tid);

    enviar_paquete(paquete, socket_kernel_dispatch);
    eliminar_paquete(paquete);

    desalojado = true;
}

void syscall_mutex_create (t_list* param)
{
    // variables parametros
    void* var_aux;
    char* recurso;

    // actualizo el contexto de ejecucion en memoria
    actualizar_contexto_ejecucion();

    // devuelvo control a kernel junto con parametros q requiera
    t_paquete* paquete = crear_paquete(SYSCALL_CREAR_MUTEX);

        // descargo parametros
    var_aux = list_get(param, 0);
    recurso = (char*)var_aux;
    agregar_a_paquete(paquete, recurso, strlen(recurso) + 1);

    // para revisar si coincide hubo algun error al cambiar contexto
    log_debug(log_cpu_gral, "PID: %d - TID: %d - Ejecutando: %s", contexto_exec.pid, contexto_exec.tid, "MUTEX_CREATE");

    // LOG OBLIGATORIO
    log_info(log_cpu_oblig, "## TID: %d - Ejecutando: %s - %s", contexto_exec.tid, "MUTEX_CREATE", recurso);

    enviar_paquete(paquete, socket_kernel_dispatch);
    eliminar_paquete(paquete);

    desalojado = true;
}

void syscall_mutex_lock (t_list* param)
{
    // variables parametros
    void* var_aux;
    char* recurso;

    // actualizo el contexto de ejecucion en memoria
    actualizar_contexto_ejecucion();

    // devuelvo control a kernel junto con parametros q requiera
    t_paquete* paquete = crear_paquete(SYSCALL_BLOQUEAR_MUTEX);

        // descargo parametros
    var_aux = list_get(param, 0);
    recurso = (char*)var_aux;
    agregar_a_paquete(paquete, recurso, strlen(recurso) + 1);

        // para revisar si coincide hubo algun error al cambiar contexto
    log_debug(log_cpu_gral, "PID: %d - TID: %d - Ejecutando: %s", contexto_exec.pid, contexto_exec.tid, "MUTEX_LOCK");

    // LOG OBLIGATORIO
    log_info(log_cpu_oblig, "## TID: %d - Ejecutando: %s - %s", contexto_exec.tid, "MUTEX_LOCK", recurso);

    enviar_paquete(paquete, socket_kernel_dispatch);
    eliminar_paquete(paquete);
    desalojado = true;
}

void syscall_mutex_unlock (t_list* param)
{
    // variables parametros
    void* var_aux;
    char* recurso;

    // actualizo el contexto de ejecucion en memoria
    actualizar_contexto_ejecucion();

    // devuelvo control a kernel junto con parametros q requiera
    t_paquete* paquete = crear_paquete(SYSCALL_DESBLOQUEAR_MUTEX);

        // descargo parametros
    var_aux = list_get(param, 0);
    recurso = (char*)var_aux;
    agregar_a_paquete(paquete, recurso, strlen(recurso) + 1);

    // para revisar si coincide hubo algun error al cambiar contexto
    log_debug(log_cpu_gral, "PID: %d - TID: %d - Ejecutando: %s", contexto_exec.pid, contexto_exec.tid, "MUTEX_UNLOCK");

    // LOG OBLIGATORIO
    log_info(log_cpu_oblig, "## TID: %d - Ejecutando: %s - %s", contexto_exec.tid, "MUTEX_UNLOCK", recurso);

    enviar_paquete(paquete, socket_kernel_dispatch);
    eliminar_paquete(paquete);
    desalojado = true;
}

void syscall_thread_exit (void)
{
    // para revisar si coincide hubo algun error al cambiar contexto (para agilizar no pongo los param (par no repetir))
    log_debug(log_cpu_gral, "PID: %d - TID: %d - Ejecutando: %s", contexto_exec.pid, contexto_exec.tid, "THREAD_EXIT");

    // LOG OBLIGATORIO
    log_info(log_cpu_oblig, "## TID: %d - Ejecutando: %s", contexto_exec.tid, "THREAD_EXIT");

    // actualizo el contexto de ejecucion en memoria
    actualizar_contexto_ejecucion();

    // devuelvo control a kernel junto con parametros q requiera
    t_paquete* paquete = crear_paquete(SYSCALL_FINALIZAR_ESTE_HILO);
    enviar_paquete(paquete, socket_kernel_dispatch);
    eliminar_paquete(paquete);

    desalojado = true;
}

void syscall_process_exit (bool exitoso)
{
    // para revisar si coincide hubo algun error al cambiar contexto (para agilizar no pongo los param (par no repetir))
    log_debug(log_cpu_gral, "PID: %d - TID: %d - Ejecutando: %s", contexto_exec.pid, contexto_exec.tid, "PROCESS_EXIT");

    if(exitoso) {
        log_info(log_cpu_oblig, "## TID: %d - Ejecutando: %s", contexto_exec.tid, "PROCESS_EXIT");
    }
    else {
        log_info(log_cpu_gral, "## TID: %d - SEGMENTATION FAULT", contexto_exec.tid);
    }

    // actualizo el contexto de ejecucion en memoria
    actualizar_contexto_ejecucion();

    // devuelvo control a kernel junto con parametros q requiera
    t_paquete* paquete = crear_paquete(SYSCALL_FINALIZAR_PROCESO);
    enviar_paquete(paquete, socket_kernel_dispatch);
    eliminar_paquete(paquete);

    desalojado = true;
    segmentation_fault = false; // limpio segfault x las dudas
}

void interrupcion (op_code tipo_interrupcion)
{
    // gestion caso hay interrupcion, pero el cpu ya habia desalojado = no tengo q interrumpir
    if (desalojado){
        log_debug(log_cpu_gral, "PID: %d - TID: %d - interrupcion recibida pero cpu ya esta desalojado. Omitiendo interrupcion.", 
                                contexto_exec.pid, contexto_exec.tid);
        return;
    }

    char* str_interrupcion = string_new();
    switch (tipo_interrupcion)
    {
    case INTERRUPCION:
        string_append(&str_interrupcion, "INTERRUPCION");
        break;
    // case SEGMENTATION_FAULT:
    //     string_append(&str_interrupcion, "SEGMENTATION_FAULT");
    //     break;
    default:
        log_debug(log_cpu_gral, "PID: %d - TID: %d - tipo interrupcion invalida, continuando ejecucion.", 
                                contexto_exec.pid, contexto_exec.tid);
        return;
        break;
    }

    log_debug(log_cpu_gral, "PID: %d - TID: %d - tipo interrupcion %s, continuando ejecucion.",
                            contexto_exec.pid, contexto_exec.tid, str_interrupcion);

    // actualizo el contexto de ejecucion en memoria (resto 1 en caso de instruccion no syscall...)
    contexto_exec.PC--;
    actualizar_contexto_ejecucion();

    // devuelvo control a kernel junto con parametros q requiera
    t_paquete* paquete = crear_paquete(tipo_interrupcion);
    // paquete = crear_paquete(INTERRUPCION);
    enviar_paquete(paquete, socket_kernel_dispatch);
    eliminar_paquete(paquete);

    free(str_interrupcion);
    desalojado = true;
    hay_interrupcion = false; // cuando se llama a la funcion ya esta protegida x mutex
    // la comento xq la interrupcion se resetea forzadamente al inicio de cada ciclo
    // Alexis 1: Lo descomento, y quito el reseteo forzado al inicio de cada ciclo
}

// ==========================================================================
// ====  Funciones Auxiliares:  =============================================
// ==========================================================================

void iniciar_logs(bool testeo)
{
    log_cpu_gral = log_create("cpu_general.log", "CPU", testeo, LOG_LEVEL_DEBUG);
    
    // Log obligatorio
    char * nivel;
    nivel = config_get_string_value(config, "LOG_LEVEL");
    log_cpu_oblig = log_create("cpu_obligatorio.log", "CPU", true, log_level_from_string(nivel));

    /*
        Ver luego si se quiere manejar caso de que el config este mal () y como cerrar el programa.
    */

    free(nivel);		
}

t_dictionary* crear_diccionario_reg(t_contexto_exec* r)
{
   t_dictionary* dicc = dictionary_create();
   dictionary_put(dicc, "PC", &(r->PC));
   dictionary_put(dicc, "AX", &(r->registros.AX));
   dictionary_put(dicc, "BX", &(r->registros.BX));
   dictionary_put(dicc, "CX", &(r->registros.CX));
   dictionary_put(dicc, "DX", &(r->registros.DX));
   dictionary_put(dicc, "EX", &(r->registros.EX));
   dictionary_put(dicc, "FX", &(r->registros.FX));
   dictionary_put(dicc, "GX", &(r->registros.GX));
   dictionary_put(dicc, "HX", &(r->registros.HX));
//    dictionary_put(dicc, "", &(r->Base));
//    dictionary_put(dicc, "", &(r->Limite));
// por ahora los dejo comentados, ya q dudo q se requira en el diccionario
   return dicc;
}

t_paquete* empaquetar_contexto()
{
    // para que al cargar contexto refleje la syscall como hecha
    uint32_t siguiente_PC = contexto_exec.PC + 1;

    t_paquete* p = crear_paquete(ACTUALIZAR_CONTEXTO_EJECUCION);
    agregar_a_paquete(p, &(siguiente_PC), sizeof(uint32_t));
    agregar_a_paquete(p, &(contexto_exec.registros.AX), sizeof(uint32_t));
    agregar_a_paquete(p, &(contexto_exec.registros.BX), sizeof(uint32_t));
    agregar_a_paquete(p, &(contexto_exec.registros.CX), sizeof(uint32_t));
    agregar_a_paquete(p, &(contexto_exec.registros.DX), sizeof(uint32_t));
    agregar_a_paquete(p, &(contexto_exec.registros.EX), sizeof(uint32_t));
    agregar_a_paquete(p, &(contexto_exec.registros.FX), sizeof(uint32_t));
    agregar_a_paquete(p, &(contexto_exec.registros.GX), sizeof(uint32_t));
    agregar_a_paquete(p, &(contexto_exec.registros.HX), sizeof(uint32_t));
    // no agrego base ni limite ya q no se alteran en ejecucion

    return p;
}

void terminar_programa() // revisar
{
	liberar_conexion(log_cpu_gral, "Memoria", socket_memoria);
	liberar_conexion(log_cpu_gral, "Kernel del puerto Dispatch", socket_kernel_dispatch);
	liberar_conexion(log_cpu_gral, "Kernel del puerto Interrupt", socket_kernel_interrupt);
	log_destroy(log_cpu_oblig);
	log_destroy(log_cpu_gral);
	config_destroy(config);
}