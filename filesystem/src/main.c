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