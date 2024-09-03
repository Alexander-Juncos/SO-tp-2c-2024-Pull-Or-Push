#include <main.h>

// Forma de ejecutar el módulo:
// ./bin/filesystem  <pathConfig> [...args]
//     argv[0] |        argv[1]       |
int main(int argc, char* argv[]) {
    /******************** Variables ********************/
    bool modulo_en_testeo = true; // gestiona si los logs auxiliares se muestran en consola o no
    char*   ip,
            puerto;

    /****************** Inicialización *****************/
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

    saludar("File System");
    
    /**************** Servidor Memoria *****************/
    /*
        descargar de config - crear conexion - hanshake (y lo q CPU necesita para funcionar)
        las conexiones van a ser temporales? posiblemente tenga q esperar un msj de fin de ejecución
    */
    /*
        recordar de utilizar setsockopt()
        int yes = 1;
        setsockopt(socket_escucha, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    */

    /****************** Logíca FS *********************/
    /*
        Realmente es la logica para atender memoria (en teoria no va separado de la parte de servidor)
        ya q son conexiones temporales la logica en realidad contendria la parte de servidor
        + limpieza de estructuras y liberar la memoria
    */

    terminar_programa();
    return 0;
}