#include <main.h>

int main(int argc, char* argv[]) {
    /******************** Variables ********************/
    bool modulo_en_testeo = true; // gestiona si los logs auxiliares se muestran en consola o no
    char*   ip,
            puerto;

    /****************** Inicialización *****************/
    // estaria bueno q se pudieran manejar diferentes a traves de arg de main
    config = iniciar_config("default"); 
    iniciar_logs(modulo_en_testeo);

    /*
        Hacer lo requerido para que arranque memoria (descargar config), iniciar el espacio de 
        memoria y sus estructuras segun corresponda (llamar a funciones q lo hagan), etc
    */

    saludar("Memoria");
    
    /****************** Servidor CPU *******************/
    /*
        descargar de config - crear conexion - hanshake (y lo q CPU necesita para funcionar)
        se puede usar como referencia mi implementación del tp anterior
    */

    /***************** Servidor Kernel *****************/
    /*
        descargar de config - crear conexion - hanshake (para Disp e Interr)
        se puede usar como referencia mi implementación del tp anterior
        
    */

    /*
        recordar de utilizar setsockopt()
        int yes = 1;
        setsockopt(socket_escucha, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    */

    /****************** Logíca Memoria *****************/
    /*
        Realmente es la logica para atender lo q necesite CPU... Kernel se manejara por hilos
        dejar un join para q el main espere al hilo q gestione las conexiones temporales de kernel
        + limpieza de estructuras y liberar la memoria
    */

    terminar_programa();
    return 0;
}