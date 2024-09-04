#include <main.h>

// Forma de ejecutar el módulo:
// ./bin/memoria  <pathConfig> [...args]
//     argv[0] |        argv[1]       |
int main(int argc, char* argv[]) {
    /******************** Variables ********************/
    bool modulo_en_testeo = true; // gestiona si los logs auxiliares se muestran en consola o no
    int yes = 1; // para setsockop
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
        imprimir_mensaje("Error: demasiados argumentos, solo se acepta <pathConfig>");
        exit(3);
    }
    iniciar_logs(modulo_en_testeo);

    /*
        Hacer lo requerido para que arranque memoria (descargar config), iniciar el espacio de 
        memoria y sus estructuras segun corresponda (llamar a funciones q lo hagan), etc
    */

    puerto = config_get_string_value(config, "PUERTO_ESCUCHA");
    socket_escucha = iniciar_servidor(puerto);
    setsockopt(socket_escucha, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    saludar("Memoria");
    
    /****************** Servidor CPU *******************/
    socket_cpu = esperar_cliente(socket_escucha);
    if (!(recibir_handshake(socket_cpu))){
        terminar_programa();
        return EXIT_FAILURE;
    }
    enviar_handshake(HANDSHAKE_OK);

    /***************** Servidor Kernel *****************/
    // esto solo hasta q se implemente servidor multihilo
    socket_cliente_temp = esperar_cliente(socket_escucha);
    if (!(recibir_handshake(socket_cliente_temp))){
        terminar_programa();
        return EXIT_FAILURE;
    }
    enviar_handshake(HANDSHAKE_OK);
    liberar_conexion(log_memoria_gral, "Kernel", socket_escucha);
    // crear hilo para servidor multihilo

    /****************** Conexion a FS ******************/
    // Solo placeholder para cumplir con check 1 (van a ser conexiones temporales cuando se requeria (DUMP))
    char* ip_fs = config_get_string_value(config, "IP_FILESYSTEM");
    char* puerto_fs = config_get_string_value(config, "PUERTO_FILESYSTEM");
    int socket_temp = crear_conexion(ip_fs, puerto_fs);
    enviar_handshake(MEMORIA, socket_temp);
    recibir_y_manejar_rta_handshake(log_memoria_gral, "FileSystem", socket_temp); // esto ya emite y todo
    liberar_conexion(log_memoria_gral, "FileSystem", socket_temp);

    /****************** Logíca Memoria *****************/
    // atender cpu (un bucle)
    // join del hilo del servidor multihilo (para q espere a q finalicen las peticiones)
    // liberar memoria

    terminar_programa();
    return 0;
}