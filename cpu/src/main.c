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
