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

t_contexto_exec contexto_exec;
t_interrupcion tipo_interrupcion = NINGUNA;
pthread_mutex_t mutex_interrupcion;

t_dictionary* diccionario_reg;

// ==========================================================================
// ====  Funciones Internas:  ===============================================
// ==========================================================================

t_list* decode (char* instruccion)
{
    char **arg = string_split(instruccion, " ");
    t_list* parametros = list_create();
    void* var_instruccion = malloc(sizeof(int));
    *var_instruccion = DESCONOCIDA;
    int num_arg = string_array_size(arg);

    // si el numero de parametros no coincide con lo esperado, desconoce la instruccion

    if (strcmp(arg[0], "SET") == 0 && num_arg == 2){
        *var_instruccion = SET;
    }
    if (strcmp(arg[0], "READ_MEM") == 0 && num_arg == 2){
        *var_instruccion = READ_MEM;
    }
    if (strcmp(arg[0], "WRITE_MEM") == 0 && num_arg == 2){
        *var_instruccion = WRITE_MEM;
    }
    if (strcmp(arg[0], "SUM") == 0 && num_arg == 2){
        *var_instruccion = SUM;
    }
    if (strcmp(arg[0], "SUB") == 0 && num_arg == 2){
        *var_instruccion = SUB;
    }
    if (strcmp(arg[0], "JNZ") == 0 && num_arg == 2){
        *var_instruccion = JNZ;
    }
    if (strcmp(arg[0], "LOG") == 0 && num_arg == 1){
        *var_instruccion = LOG;
    }
    if (strcmp(arg[0], "DUMP_MEMORY") == 0 && num_arg == 0){
        *var_instruccion = DUMP_MEMORY;
    }
    if (strcmp(arg[0], "IO") == 0 && num_arg == 1){
        *var_instruccion = IO;
    }
    if (strcmp(arg[0], "PROCESS_CREATE") == 0 && num_arg == 3){
        *var_instruccion = PROCESS_CREATE;
    }
    if (strcmp(arg[0], "THREAD_CREATE") == 0 && num_arg == 2){
        *var_instruccion = THREAD_CREATE;
    }
    if (strcmp(arg[0], "THREAD_JOIN") == 0 && num_arg == 1){
        *var_instruccion = THREAD_JOIN;
    }
    if (strcmp(arg[0], "THREAD_CANCEL") == 0 && num_arg == 1){
        *var_instruccion = THREAD_CANCEL;
    }
    if (strcmp(arg[0], "MUTEX_CREATE") == 0 && num_arg == 1){
        *var_instruccion = MUTEX_CREATE;
    }
    if (strcmp(arg[0], "MUTEX_LOCK") == 0 && num_arg == 1){
        *var_instruccion = MUTEX_LOCK;
    }
    if (strcmp(arg[0], "MUTEX_UNLOCK") == 0 && num_arg == 1){
        *var_instruccion = MUTEX_UNLOCK;
    }
    if (strcmp(arg[0], "THREAD_EXIT") == 0){
        *var_instruccion = THREAD_EXIT;
    }
    if (strcmp(arg[0], "PROCESS_EXIT") == 0){
        *var_instruccion = PROCESS_EXIT;
    }

    list_add(parametros, *(int*)var_instruccion);
    
    // si no se conoce la instruccion devuelvo
    if (*(int*)var_instruccion == DESCONOCIDA)
        return parametros;

    // paso los parametros
    for (int i = 1; i <= num_arg; i++)
    {
        list_add(parametros, arg[i]);
    }
    return parametros;    
}

void instruccion_set (t_list* param)
{
    char* str_r = (char*)list_get(param, 0);
    char* valor = (char*)list_get(param, 1);
    
    // para revisar si coincide hubo algun error al cambiar contexto
    log_debug(log_cpu_gral, "PID: %d - TID: %d - Ejecutando: %s - %s %s", contexto_exec.pid, contexto_exec.tid, "SET", str_r, valor);

    log_info(log_cpu_oblig, "## TID: %d - Ejecutando: %s - %s %s", contexto_exec.tid, "SET", str_r, valor);

	void* registro = dictionary_get(diccionario_reg, str_r);
	*(uint32_t*)registro = (uint32_t*)atoi(valor);
	log_debug(log_cpu_gral, "Se hizo SET de %s en %s", str_r, valor); // temporal. Sacar luego
}

void instruccion_sum (t_list* param)
{
    char* str_r_dstn = (char*)list_get(param, 0);
    char* str_r_orig = (char*)list_get(param, 1);
    
    // para revisar si coincide hubo algun error al cambiar contexto
    log_debug(log_cpu_gral, "PID: %d - TID: %d - Ejecutando: %s - %s %s", contexto_exec.pid, contexto_exec.tid, "SUM", str_r_dstn, str_r_orig);

    log_info(log_cpu_oblig, "## TID: %d - Ejecutando: %s - %s %s", contexto_exec.tid, "SUM", str_r_dstn, str_r_orig);

	void* reg_dstn = dictionary_get(diccionario_reg, str_r_dstn);
    void* reg_orig = dictionary_get(diccionario_reg, str_r_orig);

	*(uint32_t*)reg_dstn = *(uint32_t*)reg_dstn + *(uint32_t*)reg_orig;

	log_debug(log_cpu_gral, "Se hizo SUM de %s y %s", str_r_dstn, str_r_orig); // temporal. Sacar luego
}

void instruccion_sub (t_list* param)
{
    char* str_r_dstn = (char*)list_get(param, 0);
    char* str_r_orig = (char*)list_get(param, 1);
    
    // para revisar si coincide hubo algun error al cambiar contexto
    log_debug(log_cpu_gral, "PID: %d - TID: %d - Ejecutando: %s - %s %s", contexto_exec.pid, contexto_exec.tid, "SUB", str_r_dstn, str_r_orig);

    log_info(log_cpu_oblig, "## TID: %d - Ejecutando: %s - %s %s", contexto_exec.tid, "SUB", str_r_dstn, str_r_orig);

	void* reg_dstn = dictionary_get(diccionario_reg, str_r_dstn);
    void* reg_orig = dictionary_get(diccionario_reg, str_r_orig);

	*(uint32_t*)reg_dstn = *(uint32_t*)reg_dstn - *(uint32_t*)reg_orig;
    
	log_debug(log_cpu_gral, "Se hizo SUB de %s y %s", str_r_dstn, str_r_orig); // temporal. Sacar luego
}

void instruccion_jnz (t_list* param)
{
    char* str_r = (char*)list_get(param, 0);
    uint32_t num_inst = (uint32_t*)list_get(param, 1);
    
    // para revisar si coincide hubo algun error al cambiar contexto
    log_debug(log_cpu_gral, "PID: %d - TID: %d - Ejecutando: %s - %s %d", contexto_exec.pid, contexto_exec.tid, "JNZ", str_r, num_inst);

    log_info(log_cpu_oblig, "## TID: %d - Ejecutando: %s - %s %d", contexto_exec.tid, "JNZ", str_r, num_inst);

	void* reg = dictionary_get(diccionario_reg, str_r);

    if(*(uint32_t)reg == 0){
        log_debug(log_cpu_gral, "El registro %s tiene valor 0, no se realiza JNZ", str_r);
        return;
    }

    // como no es 0 reemplazo el PC
    contexto_exec.PC = *(uint32_t)reg;
    
	log_debug(log_cpu_gral, "Se hizo JNZ a instruccion %d (%s)", num_inst, str_r); // temporal. Sacar luego
}

void instruccion_log (t_list* param)
{
    char* str_r = (char*)list_get(param, 0);
    
    // para revisar si coincide hubo algun error al cambiar contexto
    log_debug(log_cpu_gral, "PID: %d - TID: %d - Ejecutando: %s - %s %d", contexto_exec.pid, contexto_exec.tid, "LOG", str_r);

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
    agregar_a_paquete(paq, &(contexto_exec.pid), sizeof(int));
    agregar_a_paquete(paq, &(contexto_exec.tid), sizeof(int));
    agregar_a_paquete(paq, &(contexto_exec.PC), sizeof(uint32_t));
    enviar_paquete(paq, socket_memoria);
    eliminar_paquete(paq);

    if(recibir_codigo(socket_memoria) != OBTENER_INSTRUCCION){
        log_error(log_cpu_gral,"Error en respuesta de siguiente instruccion");
    }
    t_list* list = recibir_paquete(socket_memoria);
    char* instruccion = list_get(list,0);
    list_destroy(list);

    // logs grales y obligatorio
    log_info(log_cpu_gral, "PID: %d - TID: %d - FETCH - Program Counter: %d", contexto_exec.pid, contexto_exec.PC.PC);
    log_info(log_cpu_oblig, "## TID: <%d> - FETCH - Program Counter: <%d>",contexto_exec.tid,contexto_exec.PC);
    log_info(log_cpu_gral, "Instruccion recibida: %s", instruccion);
    
    // actualizo registro program counter
    contexto_exec.PC++;

    return instruccion;
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
    
    // para revisar si coincide hubo algun error al cambiar contexto
    log_debug(log_cpu_gral, "PID: %d - TID: %d - Ejecutando: %s - %s %s", contexto_exec.pid, contexto_exec.tid, "READ_MEM", str_r_dat, str_r_dir);

    log_info(log_cpu_oblig, "## TID: %d - Ejecutando: %s - %s %s", contexto_exec.tid, "READ_MEM", str_r_dat, str_r_dir);

	void* registro_dat = dictionary_get(diccionario_reg, str_r_dat);
    void* registro_dir = dictionary_get(diccionario_reg, str_r_dir);

	
    // envio pedido lectura a memoria (mismo protocolo q antes sin pid-tid q se toman de contexto exec)
    paquete = crear_paquete(ACCESO_LECTURA);
    agregar_a_paquete(paquete, *(uint32_t*)registro_dir, sizeof(uint32_t));
    enviar_paquete(paquete, socket_memoria);
    eliminar_paquete(paquete);

    // recibo respuesta mem
    *valor = (int*)recibir_codigo(socket_memoria);
    respuesta = recibir_paquete(socket_memoria);

    if (*(int*)valor != ACCESO_LECTURA){ // si hubo error logueo y salgo
        log_error(log_cpu_gral, "ERROR: Respuesta memoria diferente a lo esperado");
        list_clean_and_destroy_elements(respuesta, free);
        return;
    }

    *valor = list_get(respuesta, 0);
    *(uint32_t*)registro_dat = (uint32_t*)atoi((char*)valor);

    log_info(log_cpu_oblig, "## TID: %d - Acción: LEER - Dirección Física: %d", contexto_exec.tid, *(uint32_t*)registro_dir);

    list_clean_and_destroy_elements(respuesta, free);
}

void instruccion_write_mem (t_list* param)
{
    // actualmente esta armado para ser legible, pero podria optimizarse a usar solo 3 void* para manejar lista-registros (creo)
    char* str_r_dat = (char*)list_get(param, 0);
    char* str_r_dir = (char*)list_get(param, 1);
    t_paquete* paquete;
    bool resultado;
    
    // para revisar si coincide hubo algun error al cambiar contexto
    log_debug(log_cpu_gral, "PID: %d - TID: %d - Ejecutando: %s - %s %s", contexto_exec.pid, contexto_exec.tid, "WRITE_MEM", str_r_dat, str_r_dir);

    log_info(log_cpu_oblig, "## TID: %d - Ejecutando: %s - %s %s", contexto_exec.tid, "WRITE_MEM", str_r_dat, str_r_dir);

	void* registro_dat = dictionary_get(diccionario_reg, str_r_dat);
    void* registro_dir = dictionary_get(diccionario_reg, str_r_dir);
	
    // envio pedido lectura a memoria (mismo protocolo q antes sin pid-tid q se toman de contexto exec)
    paquete = crear_paquete(ACCESO_LECTURA);
    agregar_a_paquete(paquete, *(uint32_t*)registro_dir, sizeof(uint32_t));
    agregar_a_paquete(paquete, *(uint32_t*)registro_dat, sizeof(uint32_t));
    enviar_paquete(paquete, socket_memoria);
    eliminar_paquete(paquete);

    // recibo respuesta mem
    resultado = recibir_mensaje_de_rta(log_cpu_gral, "WRITE_MEM", socket_memoria);

    if (resultado)
        log_info(log_cpu_oblig, "## TID: %d - Acción: ESCRIBIR - Dirección Física: %d", contexto_exec.tid, *(uint32_t*)registro_dir);
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

t_dictionary* crear_diccionario(t_contexto_exec* r)
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

void terminar_programa() // revisar
{
	liberar_conexion(log_cpu_gral, "Memoria", socket_memoria);
	liberar_conexion(log_cpu_gral, "Kernel del puerto Dispatch", socket_kernel_dispatch);
	liberar_conexion(log_cpu_gral, "Kernel del puerto Interrupt", socket_kernel_interrupt);
	log_destroy(log_cpu_oblig);
	log_destroy(log_cpu_gral);
	config_destroy(config);
}