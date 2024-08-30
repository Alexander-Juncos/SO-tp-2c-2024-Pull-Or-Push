#include <main.h>

int main(int argc, char* argv[]) {

    /******************** Variables ********************/
    char*   ip,
            puerto;

    /****************** Inicialización *****************/
    // estaria bueno q se pudieran manejar diferentes a traves de arg de main
    config = iniciar_config("default"); 

    log_kernel_gral = log_create("kernel_general.log", "Kernel", true, LOG_LEVEL_DEBUG);
    /*
    Tomar de config el LOG_LEVEL y convertirlo para usarlo en log_kernel_oblig
    */
	log_kernel_oblig = log_create("kernel_obligatorio.log", "Kernel", true, LOG_LEVEL_INFO);

    saludar("kernel");

    /****************** Conexión CPU *******************/
    /*
        descargar de config - crear conexion - hanshake (para Disp e Interr)
    */

    /******************* Cuerpo Main *******************/
    
    terminar_programa();
    return 0;
}

void terminar_programa()
{
	// Y por ultimo, hay que liberar lo que utilizamos (conexiones, log y config)
	 // con las funciones de las commons y del TP mencionadas en el enunciado /
	liberar_conexion(log_kernel_gral, "CPU Dispatch", socket_cpu_dispatch);
	liberar_conexion(log_kernel_gral, "CPU Interrupt", socket_cpu_interrupt);
	log_destroy(log_kernel_oblig);
	log_destroy(log_kernel_gral);
	config_destroy(config);
}