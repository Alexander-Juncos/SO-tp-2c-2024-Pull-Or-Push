#include <main.h>

int main(int argc, char* argv[]) {
    /******************** Variables ********************/
    char*   ip,
            puerto;

    /****************** Inicialización *****************/
    // estaria bueno q se pudieran manejar diferentes a traves de arg de main
    config = iniciar_config("default"); 

    log_cpu_gral = log_create("cpu_general.log", "CPU", true, LOG_LEVEL_DEBUG);
    /*
    Tomar de config el LOG_LEVEL y convertirlo para usarlo en log_kernel_oblig
    */
	log_cpu_oblig = log_create("cpu_obligatorio.log", "CPU", true, LOG_LEVEL_INFO);

    saludar("cpu");
    
    /***************** Conexión Memoria ****************/
     /*
        descargar de config - crear conexion - hanshake (y lo q CPU necesita para funcionar)
    */

    /***************** Conexión Kernel *****************/
     /*
        descargar de config - crear conexion - hanshake (para Disp e Interr)
    */

    /******************** Logíca CPU *******************/
     /*
        
    */

    terminar_programa();
    return 0;
}
