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
    
    pthread_mutex_init(&mutex_interrupcion, NULL);

    if (pthread_create(hilo_interrupt, NULL, rutina_hilo_interrupcion, NULL) != 0)
        log_error(log_cpu_gral, "Error al crear hilo interrupcion");

    /******************** Logíca CPU *******************/
    imprimir_mensaje("pude completar check 1");
    /*
        usar valor tipo_interrupcion para saber si hay q recibir contexto o no
    */

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

    /*
        logica recepcion de interrupciones
        actualizar tipo_interrupcion para q cuando se checkee se pueda atender
        tiene mutex -> mutex_interrupcion
    */
}