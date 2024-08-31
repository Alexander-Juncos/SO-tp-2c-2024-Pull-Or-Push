#include <main.h>

int main(int argc, char* argv[]) {
    /******************** Variables ********************/
    char*   ip,
            puerto;

    /****************** Inicialización *****************/
    // estaria bueno q se pudieran manejar diferentes a traves de arg de main
    config = iniciar_config("default"); 

    log_fs_gral = log_create("FileSytem_general.log", "FileSytem", true, LOG_LEVEL_DEBUG);
    /*
    Tomar de config el LOG_LEVEL y convertirlo para usarlo en
    */
	log_fs_oblig = log_create("FileSytem_obligatorio.log", "FileSytem", true, LOG_LEVEL_INFO);

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

void terminar_programa()
{
	// Y por ultimo, hay que liberar lo que utilizamos (conexion, log y config) 
	// con las funciones de las commons y del TP mencionadas en el enunciado /
	// liberar_conexion(log_io_gral, nombre,socket); 
	config_destroy(config);
}