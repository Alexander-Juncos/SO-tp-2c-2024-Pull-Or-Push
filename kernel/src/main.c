#include <main.h>

// Forma de ejecutar el módulo:
// ./bin/kernel [archivo_pseudocodigo] [tamanio_proceso] <pathConfig> [...args]
//     argv[0] |        argv[1]       |     argv[2]     |   argv[3]   | 
/* posible argv[4] podria ser un 1 o 0 para indicar si estamos en modo debug o no, 
requeriria mas logica de gestion de errores... a menos q fuera al inicio como argv[1]*/
int main(int argc, char* argv[]) {

    /******************** Variables ********************/
    bool modulo_en_testeo = true; // gestiona si los logs auxiliares se muestran en consola o no

    char*   ip,
            puerto;

    /****************** Inicialización *****************/
    
    if (argc == 3) // si no recibe ruta para archivo config
    {
        config = iniciar_config("default"); 
    } 
    else if (argc == 4) // si recibe ruta para archivo config
    {
        config = config_create(argv[3]);
    } 
    else
    {
        imprimir_mensaje("Error: Debe ingresar, como minimo, un [archivo_pseudocodigo] y el [tamanio_proceso]");
        exit(3);
    }

    // argv[1] => ruta de archivo de pseudocodigo (para memoria)
    // argv[2] => tamanio para reserva inicial de memoria

    iniciar_logs(modulo_en_testeo);

    saludar("kernel");

    /****************** Conexión CPU *******************/
    /*
        descargar de config - crear conexion - hanshake (para Disp e Interr)
    */

    /******************* Cuerpo Main *******************/
    
    terminar_programa();
    return 0;
}