#include <main.h>

// Forma de ejecutar el módulo:
// ./bin/cpu  <pathConfig> [...args]
//     argv[0] |        argv[1]       |
int main(int argc, char* argv[]) {
    /******************** Variables ********************/
    bool modulo_en_testeo = false; // gestiona si los logs auxiliares se muestran en consola o no
    char*   ip;
    char*   puerto;
    pthread_t hilo_interrupt;

    /****************** Inicialización *****************/
    if (argc == 1) // si no recibe ruta para archivo config
    {
        //config = iniciar_config("default"); 
        imprimir_mensaje("INGRESA EL PARAMETRO, VAGO");
        imprimir_mensaje("./c/c/default.config");
        exit(3);
    }
    else if (argc == 2){
        config = config_create(argv[1]);
    }
    else
    {
        imprimir_mensaje("Error: demasiados argumentos, solo se acepta <pathConfig>");
        exit(3);
    }
    iniciar_logs(modulo_en_testeo);

    saludar("cpu");
    pthread_mutex_init(&mutex_contexto, NULL);
    
    /***************** Conexión Memoria ****************/
    ip = config_get_string_value(config, "IP_MEMORIA");
    puerto = config_get_string_value(config, "PUERTO_MEMORIA");
    socket_memoria = crear_conexion(ip, puerto);
    enviar_handshake(CPU, socket_memoria);
    recibir_y_manejar_rta_handshake(log_cpu_gral, "Memoria", socket_memoria);

    /***************** Conexión Kernel *****************/
    pthread_mutex_init(&mutex_interrupcion, NULL);
    if (pthread_create(&hilo_interrupt, NULL, rutina_hilo_interrupcion, NULL) != 0)
        log_error(log_cpu_gral, "Error al crear hilo interrupcion");

    puerto = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
    socket_escucha_puerto_dispatch = iniciar_servidor(puerto);
    socket_kernel_dispatch = esperar_cliente(socket_escucha_puerto_dispatch);
    if (recibir_handshake(socket_kernel_dispatch) != KERNEL_D){
        terminar_programa();
        return EXIT_FAILURE;
    }
    enviar_handshake(HANDSHAKE_OK, socket_kernel_dispatch);
    
    /******************** Logíca CPU *******************/
    rutina_main_cpu();

    terminar_programa();
    return 0;
}

void* rutina_hilo_interrupcion (void*)
{
    int operacion;
    t_list* recibido;
    int pid,
        tid;
    void* aux_recibido;
    bool en_contexto_exec = 0;

    char* puerto = config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT");
    socket_escucha_puerto_interrupt = iniciar_servidor(puerto);
    socket_kernel_interrupt = esperar_cliente(socket_escucha_puerto_interrupt);
    if (recibir_handshake(socket_kernel_interrupt) != KERNEL_I){
        terminar_programa();
        return EXIT_FAILURE;
    }
    enviar_handshake(HANDSHAKE_OK, socket_kernel_interrupt);
    log_debug(log_cpu_gral, "Conexion Kernel Interrupt OK");

    // lo tome del tp anterior, aunque no se su funcion bien...
    // (comentario tp anterior) agregué esto para que el recv() de check interrupt no se quede esperando
    // Alexis 1: Esta función lo que permite es que el socket especificado permita hacer recv()'s NO bloqueantes.
    //           Así se chequea por interrupciones, y si no hay bytes por recibir, simplemente avanza.
    // fcntl(socket_kernel_interrupt, F_SETFL, O_NONBLOCK); 

    do
    {
        operacion = recibir_codigo(socket_kernel_interrupt);

        pthread_mutex_lock(&mutex_interrupcion);
        if (operacion != INTERRUPCION)
        {
            log_error(log_cpu_gral, "ERROR: se recibio un operacion desconocida al esperar interrupcion, valor: %d", operacion);
            exit(3); // no se si seria correcto para el hilo
        }
        log_info(log_cpu_oblig, "## Llega interrupción al puerto Interrupt");
        
        recibido = recibir_paquete(socket_kernel_interrupt);
        aux_recibido = list_get(recibido, 0);
        pid = *(int*)aux_recibido;
        aux_recibido = list_get(recibido, 1);
        tid = *(int*)aux_recibido;

        pthread_mutex_lock(&mutex_contexto);
        en_contexto_exec = pid == contexto_exec.pid && tid == contexto_exec.tid;
        pthread_mutex_unlock(&mutex_contexto);

        if (en_contexto_exec){
            hay_interrupcion = true;
            log_debug(log_cpu_gral, "PID: %d - TID: %d - Interrupcion Aceptada", pid, tid);
        } else
        {
            log_debug(log_cpu_gral, "Interrupcion ignorada - PID: %d - TID: %d - no coincide con contexto ejecucion (PID: %d - TID: %d).",
                                    pid, tid, contexto_exec.pid, contexto_exec.tid);
        }
        pthread_mutex_unlock(&mutex_interrupcion);

        list_destroy_and_destroy_elements(recibido, free);
    } while (operacion > 0);

    free(puerto);

    return NULL;
}

void rutina_main_cpu(void)
{
    /************************* VARIABLES  ****************************************************/
    char* instruccion_raw;
    t_list* instruccion_procesada;
    void* auxiliar_op;
    int operacion;
    desalojado = true;

    diccionario_reg = crear_diccionario_reg(&contexto_exec);
    
    // para que se diferencie en caso de 1ra ejecucion
    contexto_exec.pid = -1;



    /************************* CICLO EJECUCION ***********************************************/
    while(true)
    {
        // reseteo interrupcion, kernel se va a encargar de enviar siempre un pedido q pueda ejecutarse
        // gestionara casos de syscalls y esas cosas
        /*
        pthread_mutex_lock(&mutex_interrupcion);
        hay_interrupcion = false;
        pthread_mutex_unlock(&mutex_interrupcion);
        */

        if (desalojado)
        {
            pthread_mutex_lock(&mutex_contexto);
            recibir_pedido_ejecucion();
            pthread_mutex_unlock(&mutex_contexto);
        }

        if (contexto_exec.pid == -1)
        {
            continue;
        }

        // FETCH + DECODE
        instruccion_raw = fetch();
        instruccion_procesada = decode(instruccion_raw);
        auxiliar_op = list_remove(instruccion_procesada, 0);
        operacion = *(int*)auxiliar_op;
        free(auxiliar_op);
        
        switch (operacion) // EXECUTE
        {
        /********************************** INSTRUCCIONES **********************************/
        case SET:
            instruccion_set(instruccion_procesada);
            break;
        case READ_MEM:
            instruccion_read_mem(instruccion_procesada);
            break;
        case WRITE_MEM:
            instruccion_write_mem(instruccion_procesada);
            break;
        case SUM:
            instruccion_sum(instruccion_procesada);
            break;
        case SUB:
            instruccion_sub(instruccion_procesada);
            break;
        case JNZ:
            instruccion_jnz(instruccion_procesada);
            break;
        case LOG:
            instruccion_log(instruccion_procesada);
            break;

        /********************************** SYSCALLS ************************************/
        case DUMP_MEMORY:
            syscall_dump_memory();
            break;
        case IO:
            syscall_io(instruccion_procesada);
            break;
        case PROCESS_CREATE:
            syscall_process_create(instruccion_procesada);
            break;
        case THREAD_CREATE:
            syscall_thread_create(instruccion_procesada);
            break;
        case THREAD_JOIN:
            syscall_thread_join(instruccion_procesada);
            break;
        case THREAD_CANCEL:
            syscall_thread_cancel(instruccion_procesada);
            break;
        case MUTEX_CREATE:
            syscall_mutex_create(instruccion_procesada);
            break;
        case MUTEX_LOCK:
            syscall_mutex_lock(instruccion_procesada);
            break;
        case MUTEX_UNLOCK:
            syscall_mutex_unlock(instruccion_procesada);
            break;
        case THREAD_EXIT:
            syscall_thread_exit();
            break;
        case PROCESS_EXIT:
            syscall_process_exit(true);
            break;

        default:
            log_error(log_cpu_gral, "Operacion desconocida, codigo recibido: %d", operacion);
            break;
        }

        free(instruccion_raw);
        list_destroy_and_destroy_elements(instruccion_procesada, free);

        // se actualiza registro program counter si corresponde
        if(!se_hizo_jnz) {
            contexto_exec.PC++;
        }
        else {
            se_hizo_jnz = false;
        }

        // CHECK INTERRUPT
        pthread_mutex_lock(&mutex_interrupcion);
        if (hay_interrupcion)
            interrupcion(INTERRUPCION);
        pthread_mutex_unlock(&mutex_interrupcion);

    } 
}
