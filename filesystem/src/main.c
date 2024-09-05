#include <main.h>

// Forma de ejecutar el módulo:
// ./bin/filesystem  <pathConfig> [...args]
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
        Hacer lo requerido para que arranque el FS (descargar config), iniciarlo y a sus estructuras 
        segun corresponda (llamar a funciones q lo hagan), etc
    */

    puerto = config_get_string_value(config, "PUERTO_ESCUCHA");
    socket_escucha = iniciar_servidor(puerto);
    setsockopt(socket_escucha, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    saludar("File System");
    
    /**************** Servidor Memoria *****************/
    /*
        descargar de config - crear conexion - hanshake (y lo q CPU necesita para funcionar)
        las conexiones van a ser temporales => posiblemente tenga q esperar un msj de fin de ejecución
        /////////////////// Logíca FS ///////////////////
        /
            Realmente es la logica para atender memoria (en teoria no va separado de la parte de servidor)
            ya q son conexiones temporales la logica en realidad contendria la parte de servidor
            + limpieza de estructuras y liberar la memoria
        /
        Tendria q tener un hilo distruibuidor de hilos q reciba cada nuevo cliente (similar a memoria en tp anterior)
    */
    int socket_temp = esperar_cliente(socket_escucha); 
    if (!(recibir_handshake(socket_temp))){
        terminar_programa();
        return EXIT_FAILURE;
    }
    enviar_handshake(HANDSHAKE_OK, socket_temp);
    // lo de arriba es solo para poder hacer check1
    imprimir_mensaje("pude completar check 1");
    
    terminar_programa();
    return 0;
}