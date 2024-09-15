#include <main.h>

// Forma de ejecutar el módulo:
// ./bin/memoria  <pathConfig> [...args]
//     argv[0] |        argv[1]       |
int main(int argc, char* argv[]) {
    /******************** Variables ********************/
    bool modulo_en_testeo = true; // gestiona si los logs auxiliares se muestran en consola o no
    int setsockt_val_aux = 1; // para setsockop y recibir resultado pthread
    char* puerto;
    pthread_t hilo_recepcion;

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

    if (iniciar_memoria() == false){
        log_error(log_memoria_gral, "Error al iniciar la memoria, abortando");
        exit(3);
    }

    puerto = config_get_string_value(config, "PUERTO_ESCUCHA");
    socket_escucha = iniciar_servidor(puerto);
    setsockopt(socket_escucha, SOL_SOCKET, SO_REUSEADDR, &setsockt_val_aux, sizeof(int));

    saludar("Memoria");

    /****************** Conexion a FS ******************/
    // Solo placeholder para cumplir con check 1 (van a ser conexiones temporales cuando se requeria (DUMP))
    char* ip_fs = config_get_string_value(config, "IP_FILESYSTEM");
    char* puerto_fs = config_get_string_value(config, "PUERTO_FILESYSTEM");
    int socket_temp = crear_conexion(ip_fs, puerto_fs);
    enviar_handshake(MEMORIA, socket_temp);
    recibir_y_manejar_rta_handshake(log_memoria_gral, "FileSystem", socket_temp); // esto ya emite y todo
    liberar_conexion(log_memoria_gral, "FileSystem", socket_temp);
    
    /****************** Servidor CPU *******************/
    socket_cpu = esperar_cliente(socket_escucha);
    if (recibir_handshake(socket_cpu) != CPU){
        terminar_programa();
        return EXIT_FAILURE;
    }
    enviar_handshake(HANDSHAKE_OK, socket_cpu);

    /***************** Servidor Kernel *****************/
    // valor_aux = pthread_create(&hilo_recepcion, NULL, rutina_recepcion, NULL);
    // if (valor_aux != 0) {
	// 	log_error(log_memoria_gral, "Error al crear hilo_recepcion");
	// } 
    if (pthread_create(&hilo_recepcion, NULL, rutina_recepcion, NULL) != 0) {
		log_error(log_memoria_gral, "Error al crear hilo_recepcion");
	} // si esto les parece bien lo podemos dejar así

    /****************** Logíca Memoria *****************/
    atender_cpu();

    pthread_join(hilo_recepcion, NULL); // bloquea el main hasta q fin_programa sea true

    // liberar memoria

    terminar_programa();
    return 0;
}

// ==========================================================================
// ====  Funcion Hilo main (serv. cpu):  ====================================
// ==========================================================================

void atender_cpu()
{
    int operacion;
    t_list* pedido;
    t_paquete* paquete;

    log_debug(log_memoria_gral, "Atendiendo CPU");

    while (!fin_programa)
    {
        operacion = recibir_codigo(socket_cpu);
        switch (operacion)
        {
        case CONTEXTO_EJECUCION:
            pedido = recibir_paquete(socket_cpu);

            // Stub temporal
            enviar_mensaje("Recibi operación: CONTEXTO_EJECUCION", socket_cpu);
            log_debug(log_memoria_gral, "Operacion: CONTEXTO_EJECUCION");

            list_destroy_and_destroy_elements(pedido, free);
            break;
        
        case OBTENER_INSTRUCCION:
            pedido = recibir_paquete(socket_cpu);

            // Stub temporal
            enviar_mensaje("Recibi operación: OBTENER_INSTRUCCION", socket_cpu);
            log_debug(log_memoria_gral, "Operacion: OBTENER_INSTRUCCION");

            list_destroy_and_destroy_elements(pedido, free);
            break;
        
        case ACCESO_LECTURA:
            pedido = recibir_paquete(socket_cpu);

            // Stub temporal
            enviar_mensaje("Recibi operación: ACCESO_LECTURA", socket_cpu);
            log_debug(log_memoria_gral, "Operacion: ACCESO_LECTURA");

            list_destroy_and_destroy_elements(pedido, free);
            break;
        
        case ACCESO_ESCRITURA:
            pedido = recibir_paquete(socket_cpu);

            // Stub temporal
            enviar_mensaje("Recibi operación: ACCESO_ESCRITURA", socket_cpu);
            log_debug(log_memoria_gral, "Operacion: ACCESO_ESCRITURA");

            list_destroy_and_destroy_elements(pedido, free);
            break;

        default:
            // consumo lo q haya llegado (para liberar socket) (esto puede causar algun error)
            pedido = recibir_paquete(socket_cpu);
            list_destroy_and_destroy_elements(pedido, free);
            enviar_mensaje("Recibi operación: ERROR", socket_cpu);
            log_error(log_memoria_gral, "Operacion invalida");
            break;
        }
    }
}

// ==========================================================================
// ====  Funciones Servidor:  ===============================================
// ==========================================================================

void* rutina_recepcion (void* nada)
{
    pthread_t hilo_ejecucion;
    int error;
    int aux_socket_cliente_temp;
    pthread_mutex_init(&mutex_socket_cliente_temp, NULL);

    log_debug(log_memoria_gral, "Hilo recepcion listo.");

    while (!fin_programa)
    {
        aux_socket_cliente_temp = esperar_cliente(socket_escucha);
        pthread_mutex_lock(&mutex_socket_cliente_temp);
        socket_cliente_temp = aux_socket_cliente_temp;
        // el hilo ejecución lo desbloquea luego de descargar el socket

        error = pthread_create(&hilo_ejecucion, NULL, rutina_ejecucion, NULL);
        if (error != 0) {
			log_error(log_memoria_gral, "Error al crear hilo_ejecucion");
		}
		else {
			pthread_detach(hilo_ejecucion);
		}
    }
    pthread_mutex_destroy(&mutex_socket_cliente_temp);
    return NULL;
}


void* rutina_ejecucion (void* nada)
{
    int socket_cliente;
    int operacion;
    t_list* pedido;
    
    // descargo socket y libero variable global
    socket_cliente = socket_cliente_temp;
    pthread_mutex_unlock(&mutex_socket_cliente_temp);

    if (recibir_handshake(socket_cliente) != KERNEL){
        log_error(log_memoria_gral, "Error en handshake.");
        enviar_handshake(HANDSHAKE_INVALIDO, socket_cliente);
        return NULL;
    }
    enviar_handshake(HANDSHAKE_OK, socket_cliente);

    log_debug(log_memoria_gral, "Hilo ejecución esperando codigo");

    operacion = recibir_codigo(socket_cliente);
    switch (operacion)
    {
        case CREAR_PROCESO:
            pedido = recibir_paquete(socket_cliente);

            // Stub temporal
            enviar_mensaje("Recibi operación: CREAR_PROCESO", socket_cliente);
            log_debug(log_memoria_gral, "Operacion: CREAR_PROCESO");

            // t_pcb_mem* pcb_new = iniciar_pcb(pid);
            // si es pcb = NULL entonces no hay particiones disponibles
            // avisa a kernel INSUFICIENTE
            // de otra forma le dice que OK

            list_destroy_and_destroy_elements(pedido, free);
            break;
        
        case FINALIZAR_PROCESO:
            pedido = recibir_paquete(socket_cliente);

            // Stub temporal
            enviar_mensaje("Recibi operación: FINALIZAR_PROCESO", socket_cliente);
            log_debug(log_memoria_gral, "Operacion: FINALIZAR_PROCESO");

            list_destroy_and_destroy_elements(pedido, free);
            break;
        
        case CREAR_HILO:
            pedido = recibir_paquete(socket_cliente);

            // Stub temporal
            enviar_mensaje("Recibi operación: CREAR_HILO", socket_cliente);
            log_debug(log_memoria_gral, "Operacion: CREAR_HILO");

            /*
                recibe: PID, TID (creador), TID (nuevo), script
            */

            // t_tcb_mem* tcb_new = iniciar_tcb(/* pid, tid nuevo, script */);
            // pushear a lista del pcb del contexto_ejecucion el nuevo tcb_new (ultima posicion)
            // enviar_mensaje("OK", socket_cliente);

            list_destroy_and_destroy_elements(pedido, free);
            break;
        
        case FINALIZAR_HILO:
            pedido = recibir_paquete(socket_cliente);

            // Stub temporal
            enviar_mensaje("Recibi operación: FINALIZAR_HILO", socket_cliente);
            log_debug(log_memoria_gral, "Operacion: FINALIZAR_HILO");

            list_destroy_and_destroy_elements(pedido, free);
            break;

        case MEMORY_DUMP:
            pedido = recibir_paquete(socket_cliente);

            // Stub temporal
            enviar_mensaje("Recibi operación: MEMORY_DUMP", socket_cliente);
            log_debug(log_memoria_gral, "Operacion: MEMORY DUMP"); 

            if (memory_dump_fs(pedido) == false) { // hace toda la comunicacion con fs
                // enviar mensaje error
            }
            // enviar mensaje ok

            list_destroy_and_destroy_elements(pedido, free);
            break;
    
        default:
            // consumo lo q haya llegado (para liberar socket) (esto puede causar algun error)
            pedido = recibir_paquete(socket_cliente);
            list_destroy_and_destroy_elements(pedido, free);
            enviar_mensaje("Recibi operación: ERROR", socket_cliente);
            log_error(log_memoria_gral, "Operacion invalida");
            break;
    }

    liberar_conexion(log_memoria_gral, "Hilo Serv. temp. memoria", socket_cliente); // como servidor creo q es innecesario salvo x su log...
    return NULL;
}