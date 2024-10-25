#include <main.h>

// Forma de ejecutar el módulo:
// ./bin/memoria  <pathConfig> [...args]
//     argv[0] |        argv[1]       |
int main(int argc, char* argv[]) {
    /******************** Variables ********************/
    bool modulo_en_testeo = true; // gestiona si los logs auxiliares se muestran en consola o no
    int setsockt_val_aux = 1; // para setsockop y recibir resultado pthread
    char* puerto;
    pthread_t hilo_recepcion;

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

    if (iniciar_memoria() == false){
        log_error(log_memoria_gral, "Error al iniciar la memoria, abortando");
        exit(3);
    }

    puerto = config_get_string_value(config, "PUERTO_ESCUCHA");
    socket_escucha = iniciar_servidor(puerto);
    setsockopt(socket_escucha, SOL_SOCKET, SO_REUSEADDR, &setsockt_val_aux, sizeof(int));

    saludar("Memoria");

    /****************** Conexion a FS ******************/
    // Solo placeholder para cumplir con check 1 (van a ser conexiones temporales cuando se requeria (DUMP))
    char* ip_fs = config_get_string_value(config, "IP_FILESYSTEM");
    char* puerto_fs = config_get_string_value(config, "PUERTO_FILESYSTEM");
    int socket_temp = crear_conexion(ip_fs, puerto_fs);
    enviar_handshake(MEMORIA, socket_temp);
    recibir_y_manejar_rta_handshake(log_memoria_gral, "FileSystem", socket_temp); // esto ya emite y todo
    liberar_conexion(log_memoria_gral, "FileSystem", socket_temp);
    
    /****************** Servidor CPU *******************/
    socket_cpu = esperar_cliente(socket_escucha);
    if (recibir_handshake(socket_cpu) != CPU){
        terminar_programa();
        return EXIT_FAILURE;
    }
    enviar_handshake(HANDSHAKE_OK, socket_cpu);

    /***************** Servidor Kernel *****************/
    // valor_aux = pthread_create(&hilo_recepcion, NULL, rutina_recepcion, NULL);
    // if (valor_aux != 0) {
	// 	log_error(log_memoria_gral, "Error al crear hilo_recepcion");
	// } 
    if (pthread_create(&hilo_recepcion, NULL, rutina_recepcion, NULL) != 0) {
		log_error(log_memoria_gral, "Error al crear hilo_recepcion");
	} // si esto les parece bien lo podemos dejar así

    /****************** Logíca Memoria *****************/
    atender_cpu();

    pthread_join(hilo_recepcion, NULL); // bloquea el main hasta q fin_programa sea true

    // liberar memoria

    terminar_programa();
    return 0;
}

// ==========================================================================
// ====  Funcion Hilo main (serv. cpu):  ====================================
// ==========================================================================

void atender_cpu()
{
    int operacion;
    t_list* pedido;
    // t_paquete* paquete; // no lo termine usando
    void* aux_datos_pedido = NULL;
    void* aux_datos_paquete = NULL;

    log_debug(log_memoria_gral, "Atendiendo CPU");

    while (!fin_programa)
    {
        operacion = recibir_codigo(socket_cpu);
        switch (operacion)
        {
        case CONTEXTO_EJECUCION: // COMPLETA
            pedido = recibir_paquete(socket_cpu);

            rutina_contexto_ejecucion(pedido);

            list_destroy_and_destroy_elements(pedido, free);
            break;

        case ACTUALIZAR_CONTEXTO_EJECUCION: // COMPLETA
            pedido = recibir_paquete(socket_cpu);

            actualizar_contexto_ejecucion(pedido);

            enviar_mensaje("OK",socket_cpu); // faltaba agregarlo

            list_destroy_and_destroy_elements(pedido, free);
            break;

        case OBTENER_INSTRUCCION: // COMPLETA
            pedido = recibir_paquete(socket_cpu);

            log_debug(log_memoria_gral, 
                        "Operacion: OBTENER_INSTRUCCION - PID:TID <%d:%d>",
                        contexto_ejecucion->pcb->pid,
                        contexto_ejecucion->tcb->tid);

            aux_datos_pedido = list_get(pedido, 0);
            aux_datos_paquete = obtener_instruccion(*(uint32_t*)aux_datos_pedido);

            retardo_operacion();
            
            enviar_mensaje((char*)aux_datos_paquete, socket_cpu);

            log_info(log_memoria_oblig, "## Obtener instrucción - (PID:TID) - (%d:%d) - Instrucción: %s",
            contexto_ejecucion->pcb->pid, contexto_ejecucion->tcb->tid, (char*)aux_datos_paquete);

            list_destroy_and_destroy_elements(pedido, free);
            // no hay q liberar aux_datos_paquete ya q apunta a la instruccion en el tcb
            aux_datos_pedido = NULL;
            aux_datos_paquete = NULL;
            break;
        
        case ACCESO_LECTURA:
            pedido = recibir_paquete(socket_cpu);

            rutina_acceso_lectura(pedido);

            list_destroy_and_destroy_elements(pedido, free);
            break;
        
        case ACCESO_ESCRITURA:
            pedido = recibir_paquete(socket_cpu);

            rutina_acceso_escritura(pedido);

            list_destroy_and_destroy_elements(pedido, free);
            break;

        default:
            // consumo lo q haya llegado (para liberar socket) (esto puede causar algun error)
            pedido = recibir_paquete(socket_cpu);
            list_destroy_and_destroy_elements(pedido, free);
            enviar_mensaje("Recibi operación: ERROR", socket_cpu);
            log_error(log_memoria_gral, "Operacion invalida");
            break;
        }
    }
}