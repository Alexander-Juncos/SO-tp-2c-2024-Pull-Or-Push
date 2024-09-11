#include <main.h>

// Forma de ejecutar el módulo:
// ./bin/filesystem  <pathConfig> [...args]
//     argv[0] |        argv[1]       |
int main(int argc, char* argv[]) {
    /******************** Variables ********************/
    bool modulo_en_testeo = true; // gestiona si los logs auxiliares se muestran en consola o no
    int setsockt_val_aux = 1; // para setsockop
    char* puerto;

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
        config = iniciar_config("default"); // PARA PROBAR LOS HANDSHAKES.

        /* COMENTO ESTO POR AHORA, PARA PROBAR LOS HANDSHAKES.
        *
        imprimir_mensaje("Error: demasiados argumentos, solo se acepta <pathConfig>");
        exit(3);
        */
    }
    iniciar_logs(modulo_en_testeo);

    iniciar_fs();

    puerto = config_get_string_value(config, "PUERTO_ESCUCHA");
    socket_escucha = iniciar_servidor(puerto);
    setsockopt(socket_escucha, SOL_SOCKET, SO_REUSEADDR, &setsockt_val_aux, sizeof(int));

    saludar("File System");
    
    /**************** Servidor Multihilo ***************/
    rutina_recepcion(); // la logica se va a distribuir desde aca
    
    terminar_programa();
    return 0;
}

// ==========================================================================
// ====  Funciones Servidor:  ===============================================
// ==========================================================================

void rutina_recepcion (void)
{
    pthread_t hilo_ejecucion;
    int error;
    int aux_socket_cliente_temp;
    pthread_mutex_init(&mutex_socket_cliente_temp, NULL);

    log_debug(log_fs_gral, "Hilo recepcion listo.");

    while (!fin_programa)
    {
        aux_socket_cliente_temp = esperar_cliente(socket_escucha);
        pthread_mutex_lock(&mutex_socket_cliente_temp);
        socket_cliente_temp = aux_socket_cliente_temp;
        // el hilo ejecución lo desbloquea luego de descargar el socket

        error = pthread_create(&hilo_ejecucion, NULL, rutina_ejecucion, NULL);
        if (error != 0) {
			log_error(log_fs_gral, "Error al crear hilo_ejecucion");
		}
		else {
			pthread_detach(hilo_ejecucion);
		}
    }
    pthread_mutex_destroy(&mutex_socket_cliente_temp);
}


void* rutina_ejecucion (void*)
{
    int socket_cliente;
    int operacion;
    t_list* pedido;
    
    // descargo socket y libero variable global
    socket_cliente = socket_cliente_temp;
    pthread_mutex_unlock(&mutex_socket_cliente_temp);

    if (recibir_handshake(socket_cliente) != MEMORIA){
        log_error(log_fs_gral, "Error en handshake.");
        return NULL;
    }
    enviar_handshake(HANDSHAKE_OK, socket_cliente);

    log_debug(log_fs_gral, "Hilo ejecución esperando codigo");

    operacion = recibir_codigo(socket_cliente);
    switch (operacion)
    {
        case MEMORY_DUMP:
            pedido = recibir_paquete(socket_cliente);
            
            /* 
                haciendo el dump en el fs
            */

            list_destroy_and_destroy_elements(pedido, free);
            enviar_mensaje("OK", socket_cliente);
            log_debug(log_fs_gral, "Memory Dump hecho");
            break;
    
        default:
            // consumo lo q haya llegado (para liberar socket) (esto puede causar algun error)
            pedido = recibir_paquete(socket_cliente);
            list_destroy_and_destroy_elements(pedido, free);
            enviar_mensaje("ERROR", socket_cliente);
            log_error(log_fs_gral, "Operacion invalida");
            break;
    }

    liberar_conexion(log_fs_gral, "MEMORIA", socket_cliente); // como servidor creo q es innecesario salvo x su log...
    return NULL;
}