#include <main.h>

int main(int argc, char* argv[]) {
    /******************** Variables ********************/
    char*   ip,
            puerto;

    /****************** Inicialización *****************/
    // estaria bueno q se pudieran manejar diferentes a traves de arg de main
    config = iniciar_config("default"); 

    log_memoria_gral = log_create("Memoria_general.log", "Memoria", true, LOG_LEVEL_DEBUG);
    /*
    Tomar de config el LOG_LEVEL y convertirlo para usarlo en log_kernel_oblig
    */
	log_memoria_oblig = log_create("Memoria_obligatorio.log", "Memoria", true, LOG_LEVEL_INFO);

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

void terminar_programa()
{
	// Y por ultimo, hay que liberar lo que utilizamos (conexion, log y config) 
	 // con las funciones de las commons y del TP mencionadas en el enunciado /
	liberar_conexion(log_memoria_gral, "CPU", socket_cpu);
	liberar_conexion(log_memoria_gral, "Mi propio Servidor Escucha", socket_escucha);
	log_destroy(log_memoria_oblig);
	log_destroy(log_memoria_gral);
	config_destroy(config);
}