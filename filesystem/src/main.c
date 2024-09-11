#include <main.h>

// Forma de ejecutar el módulo:
// ./bin/filesystem  <pathConfig> [...args]
//     argv[0] |        argv[1]       |
int main(int argc, char* argv[]) {
    /******************** Variables ********************/
    bool modulo_en_testeo = true; // gestiona si los logs auxiliares se muestran en consola o no
    int aux = 1; // para setsockop y errores pthread
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

    /*
        Hacer lo requerido para que arranque el FS (descargar config), iniciarlo y a sus estructuras 
        segun corresponda (llamar a funciones q lo hagan), etc
    */

    puerto = config_get_string_value(config, "PUERTO_ESCUCHA");
    socket_escucha = iniciar_servidor(puerto);
    setsockopt(socket_escucha, SOL_SOCKET, SO_REUSEADDR, &aux, sizeof(int));

    saludar("File System");
    
    /**************** Servidor Memoria *****************/
    rutina_recepcion(); // la logica se va a distribuir desde aca
    // int socket_temp = esperar_cliente(socket_escucha); 
    // if (recibir_handshake(socket_temp) != MEMORIA){
    //     terminar_programa();
    //     return EXIT_FAILURE;
    // }
    // enviar_handshake(HANDSHAKE_OK, socket_temp);
    
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
        aux_socket_cliente_temp;
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

	return NULL;
}


void* rutina_ejecucion (void*)
{
    
}