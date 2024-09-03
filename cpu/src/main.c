#include <main.h>

// Forma de ejecutar el módulo:
// ./bin/cpu  <pathConfig> [...args]
//     argv[0] |        argv[1]       |
int main(int argc, char* argv[]) {
    /******************** Variables ********************/
    bool modulo_en_testeo = true; // gestiona si los logs auxiliares se muestran en consola o no
    char*   ip,
            puerto;

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
