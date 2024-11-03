#include <main.h>

// Forma de ejecutar el módulo:
// ./bin/kernel [archivo_pseudocodigo] [tamanio_proceso] <pathConfig> [...args]
//     argv[0] |        argv[1]       |     argv[2]     |   argv[3]   | 
/* posible argv[4] podria ser un 1 o 0 para indicar si estamos en modo debug o no, 
requeriria mas logica de gestion de errores... a menos q fuera al inicio como argv[1]*/
int main(int argc, char* argv[]) {

    /******************** Variables ********************/

    bool modulo_en_testeo = true; // gestiona si los logs auxiliares se muestran en consola o no

    char* ip;
    char* puerto;

    /****************** Inicialización *****************/
    
    if (argc == 4) // si recibe ruta para archivo config
    {
        config = config_create(argv[3]);
    }
    else if (argc == 3) // si no recibe ruta para archivo config
    {
        config = iniciar_config("default"); 
    }
    else if (argc == 2)
    {
        imprimir_mensaje("Error: Debe ingresar tambien el [tamanio_proceso]");
    }
    else if (argc <= 1)
    {
        config = iniciar_config("default"); // PARA PROBAR LOS HANDSHAKES

        /* COMENTO ESTO POR AHORA, PARA PROBAR LOS HANDSHAKES.
        -------------------------------------------------------
        imprimir_mensaje("Error: Debe ingresar, como minimo, un [archivo_pseudocodigo] y el [tamanio_proceso]");
        exit(3);
        -------------------------------------------------------
        */
    }

    // argv[1] => ruta de archivo de pseudocodigo (para memoria)
    // argv[2] => tamanio para reserva inicial de memoria

    iniciar_logs(modulo_en_testeo);

    saludar("kernel");

    /****************** Conexión CPU *******************/
    ip = config_get_string_value(config, "IP_CPU");

    puerto = config_get_string_value(config, "PUERTO_CPU_DISPATCH");
    socket_cpu_dispatch = crear_conexion(ip, puerto);
    enviar_handshake(KERNEL_D, socket_cpu_dispatch);
    recibir_y_manejar_rta_handshake(log_kernel_gral, "CPU puerto Dispatch", socket_cpu_dispatch);

    puerto = config_get_string_value(config, "PUERTO_CPU_INTERRUPT");
    socket_cpu_interrupt = crear_conexion(ip, puerto);
    enviar_handshake(KERNEL_I, socket_cpu_interrupt);
    recibir_y_manejar_rta_handshake(log_kernel_gral, "CPU puerto Interrupt", socket_cpu_interrupt);

    /****** Carga datos para conexiones Memoria ********/
    ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");

    /******************* Logica de Kernel *******************/
    t_pcb* proceso_inicial = NULL;

    algoritmo_plani = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
    setup_algoritmo_plani_corto_plazo(algoritmo_plani);
    quantum_de_config = config_get_int_value(config, "QUANTUM");

    pthread_t thread_new;
    pthread_t thread_exit;
    pthread_t thread_io;

    pthread_create(&thread_new, NULL, rutina_new, NULL);
    pthread_create(&thread_exit, NULL, rutina_exit, NULL);
    pthread_create(&thread_io, NULL, rutina_io, NULL);
    
    // El Proceso inicial.
    proceso_inicial = nuevo_proceso(atoi(argv[2]), 0, argv[1]);
    ingresar_a_new(proceso_inicial);


    iniciar_planificador();


    imprimir_mensaje("pudimos completar check 3");

    //terminar_programa();
    return 0;
}

// ============================================================

void setup_algoritmo_plani_corto_plazo(char* algoritmo) {
    if(strcmp(algoritmo, "FIFO") == 0) {
        cod_algoritmo_planif_corto = FIFO;
        ingresar_a_ready = ingresar_a_ready_fifo;
        encontrar_y_remover_tcb_en_ready = encontrar_y_remover_tcb_en_ready_fifo_y_prioridades;
    }
    else if(strcmp(algoritmo, "PRIORIDADES") == 0) {
        cod_algoritmo_planif_corto = PRIORIDADES;
        ingresar_a_ready = ingresar_a_ready_prioridades;
        encontrar_y_remover_tcb_en_ready = encontrar_y_remover_tcb_en_ready_fifo_y_prioridades;
    }
    else if(strcmp(algoritmo, "CMN") == 0) {
        cod_algoritmo_planif_corto = CMN;
        ingresar_a_ready = ingresar_a_ready_multinivel;
        encontrar_y_remover_tcb_en_ready = encontrar_y_remover_tcb_en_ready_multinivel;
    }
    else {
        log_error(log_kernel_gral, "Algoritmo de planificación desconocido.");
        exit(3);
    }
}
