#include <main.h>

// Forma de ejecutar el módulo:
// ./bin/cpu  <pathConfig> [...args]
//     argv[0] |        argv[1]       |
int main(int argc, char* argv[]) {
    /******************** Variables ********************/
    bool modulo_en_testeo = true; // gestiona si los logs auxiliares se muestran en consola o no
    char*   ip;
    char*   puerto;

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
    puerto = config_get_string_value(config, "IP_MEMORIA");
    socket_memoria = crear_conexion(ip, puerto);
    enviar_handshake(CPU, socket_memoria);
    recibir_y_manejar_rta_handshake(log_cpu_gral, "Memoria", socket_memoria);

    /***************** Conexión Kernel *****************/
    puerto = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
    socket_kernel_dispatch = iniciar_servidor(puerto);
    puerto = config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT");
    socket_kernel_interrupt = iniciar_servidor(puerto);

    // prueba conexion - limpiar y rehacer despues de check 1
    int kernel_dispatch = esperar_cliente(socket_kernel_dispatch);
    if (!(recibir_handshake(kernel_dispatch))){
        terminar_programa();
        return EXIT_FAILURE;
    }
    enviar_handshake(HANDSHAKE_OK, kernel_dispatch);

    int kernel_interrupt = esperar_cliente(socket_kernel_interrupt);
    if (!(recibir_handshake(kernel_interrupt))){
        terminar_programa();
        return EXIT_FAILURE;
    }
    enviar_handshake(HANDSHAKE_OK, kernel_interrupt);

    /******************** Logíca CPU *******************/
    imprimir_mensaje("pude completar check 1");
     /*
        
    */

    terminar_programa();
    return 0;
}
