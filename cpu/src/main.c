#include <main.h>

// Forma de ejecutar el módulo:
// ./bin/cpu  <pathConfig> [...args]
//     argv[0] |        argv[1]       |
int main(int argc, char* argv[]) {
    /******************** Variables ********************/
    bool modulo_en_testeo = true; // gestiona si los logs auxiliares se muestran en consola o no
    char*   ip;
    char*   puerto;
    pthread_t hilo_interrupt;

    /****************** Inicialización *****************/
    if (argc == 1) // si no recibe ruta para archivo config
    {
        config = iniciar_config("default"); 
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
    
    /***************** Conexión Memoria ****************/
    ip = config_get_string_value(config, "IP_MEMORIA");
    puerto = config_get_string_value(config, "PUERTO_MEMORIA");
    socket_memoria = crear_conexion(ip, puerto);
    enviar_handshake(CPU, socket_memoria);
    recibir_y_manejar_rta_handshake(log_cpu_gral, "Memoria", socket_memoria);

    /***************** Conexión Kernel *****************/

    // prueba conexion - limpiar y rehacer despues de check 1
    puerto = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
    socket_escucha_puerto_dispatch = iniciar_servidor(puerto);
    socket_kernel_dispatch = esperar_cliente(socket_escucha_puerto_dispatch);
    if (recibir_handshake(socket_kernel_dispatch) != KERNEL_D){
        terminar_programa();
        return EXIT_FAILURE;
    }
    enviar_handshake(HANDSHAKE_OK, socket_kernel_dispatch);
    
    /******************** Logíca CPU *******************/
    pthread_mutex_init(&mutex_interrupcion, NULL);

    if (pthread_create(&hilo_interrupt, NULL, rutina_hilo_interrupcion, NULL) != 0)
        log_error(log_cpu_gral, "Error al crear hilo interrupcion");

    rutina_main_cpu();

    terminar_programa();
    return 0;
}

void* rutina_hilo_interrupcion (void*)
{
    int operacion;
    t_list* recibido;

    char* puerto = config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT");
    socket_escucha_puerto_interrupt = iniciar_servidor(puerto);
    socket_kernel_interrupt = esperar_cliente(socket_escucha_puerto_interrupt);
    if (recibir_handshake(socket_kernel_interrupt) != KERNEL_I){
        terminar_programa();
        return EXIT_FAILURE;
    }
    enviar_handshake(HANDSHAKE_OK, socket_kernel_interrupt);

    // lo tome del tp anterior, aunque no se su funcion bien...
    // (comentario tp anterior) agregué esto para que el recv() de check interrupt no se quede esperando
    // Alexis 1: Esta función lo que permite es que el socket especificado permita hacer recv()'s NO bloqueantes.
    //           Así se chequea por interrupciones, y si no hay bytes por recibir, simplemente avanza.
    fcntl(socket_kernel_interrupt, F_SETFL, O_NONBLOCK); 

    /*
        logica recepcion de interrupciones
        El pid y tid recibido en la comunicacion va a ser usado para comprobar si Kernel le mandó interrupcion al 
        tid en ejecucion, o no (en este caso la desestima).
        tiene mutex -> mutex_interrupcion

        Interrupción Recibida: “## Llega interrupción al puerto Interrupt”.

    */
    return;
}

void rutina_main_cpu(void)
{
    /************************* VARIABLES  ****************************************************/
    char* instruccion_raw;
    t_list* instruccion_procesada;
    void* auxiliar_op;
    int operacion;
    desalojado = true;
    bool interrupcion_previa;

    diccionario_reg = crear_diccionario_reg(&contexto_exec);


    /************************* CICLO EJECUCION ***********************************************/
    while(true)
    {
        if (desalojado)
        {
            recibir_pedido_ejecucion();
        }

        // para no crear una SC enorme para nada
        phtread_mutex_lock(mutex_interrupcion);
        interrupcion_previa = hay_interrupcion;
        pthread_mutex_unlock(mutex_interrupcion);

        if (interrupcion_previa)
        {
            operacion = INTERRUPCION_PENDIENTE; // si desp de recibir_pedido sigue, salteo el fetch
            instruccion_raw = string_new(); // lo inicio xq es mas facil q poner mas condiciones para evitar doble free
        }
        else
        {
            instruccion_raw = fetch();
            instruccion_procesada = decode(instruccion_raw);
            auxiliar_op = list_remove(instruccion_procesada, 0);
            operacion = *(int*)auxiliar_op;
            free(auxiliar_op);
        }

        switch (operacion)
        {
        /********************************** INTERRUPCION ************************************/
        case INTERRUPCION_PENDIENTE:
            // no hago nada y salgo de switch
            break;

        /********************************** INSTRUCCIONES **********************************/
        case SET:
            instruccion_set(instruccion_procesada);
            list_clean_and_destroy_elements(instruccion_procesada, free);
            break;
        case READ_MEM:
            instruccion_read_mem(instruccion_procesada);
            list_clean_and_destroy_elements(instruccion_procesada, free);
            break;
        case WRITE_MEM:
            instruccion_write_mem(instruccion_procesada);
            list_clean_and_destroy_elements(instruccion_procesada, free);
            break;
        case SUM:
            instruccion_sum(instruccion_procesada);
            list_clean_and_destroy_elements(instruccion_procesada, free);
            break;
        case SUB:
            instruccion_sub(instruccion_procesada);
            list_clean_and_destroy_elements(instruccion_procesada, free);
            break;
        case JNZ:
            instruccion_jnz(instruccion_procesada);
            list_clean_and_destroy_elements(instruccion_procesada, free);
            break;
        case LOG:
            instruccion_log(instruccion_procesada);
            list_clean_and_destroy_elements(instruccion_procesada, free);
            break;

        /********************************** SYSCALLS ************************************/
        case DUMP_MEMORY:
            syscall_dump_memory();
            list_clean_and_destroy_elements(instruccion_procesada, free);
            break;
        case IO:
            syscall_io(instruccion_procesada);
            list_clean_and_destroy_elements(instruccion_procesada, free);
            break;
        case PROCESS_CREATE:
            syscall_process_create(instruccion_procesada);
            list_clean_and_destroy_elements(instruccion_procesada, free);
            break;
        case THREAD_CREATE:
            syscall_thread_create(instruccion_procesada);
            list_clean_and_destroy_elements(instruccion_procesada, free);
            break;
        case THREAD_JOIN:
            syscall_thread_join(instruccion_procesada);
            list_clean_and_destroy_elements(instruccion_procesada, free);
            break;
        case THREAD_CANCEL:
            syscall_thread_cancel(instruccion_procesada);
            list_clean_and_destroy_elements(instruccion_procesada, free);
            break;
        case MUTEX_CREATE:
            syscall_mutex_create(instruccion_procesada);
            list_clean_and_destroy_elements(instruccion_procesada, free);
            break;
        case MUTEX_LOCK:
            syscall_mutex_lock(instruccion_procesada);
            list_clean_and_destroy_elements(instruccion_procesada, free);
            break;
        case MUTEX_UNLOCK:
            syscall_mutex_unlock(instruccion_procesada);
            list_clean_and_destroy_elements(instruccion_procesada, free);
            break;
        case THREAD_EXIT:
            syscall_thread_exit(instruccion_procesada);
            list_clean_and_destroy_elements(instruccion_procesada, free);
            break;
        case PROCESS_EXIT:
            syscall_process_exit(instruccion_procesada);
            list_clean_and_destroy_elements(instruccion_procesada, free);
            break;

        
        default:
            log_error(log_cpu_gral, "Operacion desconocida, codigo recibido: %d", operacion);
            break;
        }

        // si hay interrupcion y la estoy gestionando no me interesa q entre otra
        pthread_mutex_lock(mutex_interrupcion);
        if (hay_interrupcion)
            interrupcion(INTERRUPCION);
        pthread_mutex_unlock(mutex_interrupcion);

        free(instruccion_raw);

        // actualizo registro program counter
        contexto_exec.PC++;
    } 
}
