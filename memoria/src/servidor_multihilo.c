#include <servidor_multihilo.h>
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
            enviar_mensaje("OK", socket_cliente);
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