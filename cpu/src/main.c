#include <main.h>

// Forma de ejecutar el módulo:
// ./bin/cpu  <pathConfig> [...args]
//     argv[0] |        argv[1]       |
int main(int argc, char* argv[]) {
    /******************** Variables ********************/
    bool modulo_en_testeo = true; // gestiona si los logs auxiliares se muestran en consola o no
    char*   ip;
    char*   puerto;
    pthread_t hilo_interrupt;

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
    ip = config_get_string_value(config, "IP_MEMORIA");
    puerto = config_get_string_value(config, "PUERTO_MEMORIA");
    socket_memoria = crear_conexion(ip, puerto);
    enviar_handshake(CPU, socket_memoria);
    recibir_y_manejar_rta_handshake(log_cpu_gral, "Memoria", socket_memoria);

    /***************** Conexión Kernel *****************/

    // prueba conexion - limpiar y rehacer despues de check 1
    puerto = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
    socket_escucha_puerto_dispatch = iniciar_servidor(puerto);
    socket_kernel_dispatch = esperar_cliente(socket_escucha_puerto_dispatch);
    if (recibir_handshake(socket_kernel_dispatch) != KERNEL_D){
        terminar_programa();
        return EXIT_FAILURE;
    }
    enviar_handshake(HANDSHAKE_OK, socket_kernel_dispatch);
    
    /******************** Logíca CPU *******************/
    pthread_mutex_init(&mutex_interrupcion, NULL);

    if (pthread_create(&hilo_interrupt, NULL, rutina_hilo_interrupcion, NULL) != 0)
        log_error(log_cpu_gral, "Error al crear hilo interrupcion");

    rutina_main_cpu();

    terminar_programa();
    return 0;
}

void* rutina_hilo_interrupcion (void*)
{
    int operacion;
    t_list* recibido;

    char* puerto = config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT");
    socket_escucha_puerto_interrupt = iniciar_servidor(puerto);
    socket_kernel_interrupt = esperar_cliente(socket_escucha_puerto_interrupt);
    if (recibir_handshake(socket_kernel_interrupt) != KERNEL_I){
        terminar_programa();
        return EXIT_FAILURE;
    }
    enviar_handshake(HANDSHAKE_OK, socket_kernel_interrupt);

    // lo tome del tp anterior, aunque no se su funcion bien...
    // (comentario tp anterior) agregué esto para que el recv() de check interrupt no se quede esperando
    // Alexis 1: Esta función lo que permite es que el socket especificado permita hacer recv()'s NO bloqueantes.
    //           Así se chequea por interrupciones, y si no hay bytes por recibir, simplemente avanza.
    fcntl(socket_kernel_interrupt, F_SETFL, O_NONBLOCK); 

    /*
        logica recepcion de interrupciones
        El pid y tid recibido en la comunicacion va a ser usado para comprobar si Kernel le mandó interrupcion al 
        tid en ejecucion, o no (en este caso la desestima).
        tiene mutex -> mutex_interrupcion

        Interrupción Recibida: “## Llega interrupción al puerto Interrupt”.

    */
    return;
}

void rutina_main_cpu(void)
{
    /************************* VARIABLES  ****************************************************/
    char* instruccion_raw;
    t_list* mensaje_kernel;
    int operacion;

    diccionario_reg = crear_diccionario_reg(&contexto_exec);

    // bucle
        
        // desalojado indica: cambio de contexto, posible ciclo corto (syscall)
        // "ciclo corto": caso en q al ejecutar syscall llegara interrupcion (esperar respuesta kernel
        //  y despues saltarse fetch y pasar directamente a interrupcion )

        // revisar si hay q cargar nuevo pid, si hay cargarlo y desalojado = false
        // si hay q cargarlo emitir:
        // Obtención de Contexto de Ejecución: .

        // fetch [ si hay_interrupcion entonces no hacer fetch / sobreescribirlo ]

        // decode (cargar instruccion a un valor para switch)

        // swith para llamar a cada instruccion segun corresponda... (exec)
        // despues de syscalls marcar desalojado como true
        // las instrucciones no concideran q haya instruccion desconocida

        // si segmentation_fault == false entonces reviso si hubo interrupcion, si hubo interrupcion(INTERRUPCION)
        // si segmentation_fault == true entonces reseteo seg_fault e interrupcion y mando interrupcion (SEGMENTATION_FAULT)

        // actualizo registro program counter
        contexto_exec.PC++;
}
