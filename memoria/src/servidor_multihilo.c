#include <servidor_multihilo.h>

pthread_mutex_t mutex_socket_cliente_temp;
int socket_cliente_temp = 1;

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

            rutina_crear_proceso(pedido, socket_cliente);

            list_destroy_and_destroy_elements(pedido, free);
            break;
        
        case FINALIZAR_PROCESO: // PENDIENTE IMPLEMENTAR + FUNCIONES
            pedido = recibir_paquete(socket_cliente);
            // el paquete se encuentra "vacio"...

            // Stub temporal
            enviar_mensaje("Recibi operación: FINALIZAR_PROCESO", socket_cliente);
            log_debug(log_memoria_gral, "Operacion: FINALIZAR_PROCESO");

            list_destroy_and_destroy_elements(pedido, free);
            break;
        
        case CREAR_HILO:
            pedido = recibir_paquete(socket_cliente);

            rutina_crear_hilo(pedido, socket_cliente);

            list_destroy_and_destroy_elements(pedido, free);
            break;
        
        case FINALIZAR_HILO:
            pedido = recibir_paquete(socket_cliente);

            rutina_finalizar_hilo(pedido, socket_cliente);

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