#include "planificador.h"
#include "quantum.h"
#include "respuesta_memory_dump.h"

// ==========================================================================
// ====  Variables globales (exclusivas del planificador):  =================
// ==========================================================================

int contador_pid = 0;

// ==========================================================================
// ====  Función principal (que inicia el planificador):  ===================
// ==========================================================================

void iniciar_planificador(void) {

    log_debug(log_kernel_gral, "Planificador corto plazo iniciado.");

    if (strcmp(algoritmo_plani, "FIFO") == 0) {
        planific_corto_fifo();
    }
    else if (strcmp(algoritmo_plani, "PRIORIDADES") == 0) {
        planific_corto_prioridades();
    }
    else if (strcmp(algoritmo_plani, "CMN") == 0) {
        planific_corto_multinivel();
    }
    
}

/////////////////////////////////////////////////////
// -----------------------------------------------
//---  TRABAJANDO EN LOS ALGORITMOS...
// -----------------------------------------------
/////////////////////////////////////////////////////

void planific_corto_fifo(void) {

    // DEL TP ANTERIOR: /////////////////////////////////////////////////
    // variables que defino acá porque las repito en varios case del switch
    t_paquete* paquete = NULL;
    char* nombre_interfaz = NULL;
    t_io_blocked* io = NULL;
    int cant_de_pares_direccion_tamanio;
    int* dir = NULL;
    int* tamanio = NULL;
    int fs_codigo;
    char* nombre_archivo = NULL;
    char* nombre_recurso = NULL;
    //////////////////////////////////////////////////////////////////////

    int codigo_recibido = -1;
    // Lista con data del paquete recibido desde cpu.
    t_list* argumentos_recibidos = NULL;

    log_debug(log_kernel_gral, "Planificador corto plazo listo para funcionar con algoritmo FIFO.");

    sem_wait(&sem_cola_ready_unica);
    pthread_mutex_lock(&mutex_hilo_exec);
    ejecutar_siguiente_hilo(cola_ready_unica);
    pthread_mutex_unlock(&mutex_hilo_exec);

    while(true) {

        // Se queda esperando el "desalojo" del proceso.
        argumentos_recibidos = recibir_de_cpu(&codigo_recibido);

        //hay_algun_proceso_en_exec = false;

        //pthread_mutex_lock(&mutex_proceso_exec);


        // EN DESARROLLO... Casi todo del TP viejo
		switch (codigo_recibido) {

        // -!!!!--- ACÁ ESTOY TRABAJANDO ---!!!!-
        // -----   ----!!----   -----   -------!!!!!!!!
            case SYSCALL_MEMORY_DUMP:

            break;

			case SUCCESS:
            pthread_mutex_lock(&mutex_procesos_activos);
            pthread_mutex_lock(&mutex_cola_exit);
            list_add(cola_exit, hilo_exec);
            sem_post(&sem_procesos_exit);
            procesos_activos--;
            log_info(log_kernel_oblig, "Finaliza el proceso %d - Motivo: SUCCESS", hilo_exec->tid); // log Obligatorio.
            log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", hilo_exec->tid); // log Obligatorio.
            hilo_exec = NULL;
            pthread_mutex_unlock(&mutex_cola_exit);
            pthread_mutex_unlock(&mutex_procesos_activos);
            break;

            // Este caso es el que hay que adaptar, hay que mandarlo a blocked y ponerle un contador por el tiempo que realiza la IO. Luego, devolverlo a READY.
            case IO:
            nombre_interfaz = list_get(desalojo_y_argumentos, 1);
            int unidades_de_trabajo = *(int*)list_get(desalojo_y_argumentos, 2);

            // paquete = crear_paquete(IO_OPERACION);
            // agregar_a_paquete(paquete, &(proceso_exec->pid), sizeof(int));
            // agregar_a_paquete(paquete, &unidades_de_trabajo, sizeof(int));

            // pthread_mutex_lock(&mutex_lista_io_blocked);
            // io = encontrar_io(nombre_interfaz);
            // if(io != NULL) {
            //     enviar_paquete(paquete, io->socket);
            //     log_debug(log_kernel_gral, "Proceso %d empieza a usar interfaz %s", proceso_exec->pid, nombre_interfaz);
            //     pthread_mutex_lock(&(proceso_exec->mutex_uso_de_io));
            //     list_add(io->cola_blocked, proceso_exec);
            //     log_info(log_kernel_oblig, "PID: %d - Bloqueado por: INTERFAZ", proceso_exec->pid); // log Obligatorio
            //     log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: BLOCKED", proceso_exec->pid); // log Obligatorio
            //     proceso_exec = NULL;
            // }
            // else {
            //     log_error(log_kernel_gral, "Interfaz %s no encontrada.", nombre_interfaz);
            //     pthread_mutex_lock(&mutex_procesos_activos);
            //     pthread_mutex_lock(&mutex_cola_exit);
            //     list_add(cola_exit, proceso_exec);
            //     procesos_activos--;
            //     sem_post(&sem_cola_exit);
            //     log_info(log_kernel_oblig, "Finaliza el proceso %d - Motivo: INVALID_INTERFACE", proceso_exec->pid); // log Obligatorio
            //     log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", proceso_exec->pid); // log Obligatorio
            //     proceso_exec = NULL;
            //     pthread_mutex_unlock(&mutex_cola_exit);
            //     pthread_mutex_unlock(&mutex_procesos_activos);
            // }
            // pthread_mutex_unlock(&mutex_lista_io_blocked);

            // eliminar_paquete(paquete);
            manejar_solicitud_io(hilo_exec, unidades_de_trabajo);
            break;

            case WAIT:
            nombre_recurso = list_get(desalojo_y_argumentos, 1);

            pthread_mutex_lock(&mutex_lista_recurso_blocked);
            recurso_blocked = encontrar_recurso_blocked(nombre_recurso);
            pthread_mutex_unlock(&mutex_lista_recurso_blocked);

            if( recurso_blocked==NULL )
            {
                log_error(log_kernel_gral, "Recurso %s no encontrado en el sistema.", nombre_recurso);
                pthread_mutex_lock(&mutex_procesos_activos);
                pthread_mutex_lock(&mutex_cola_exit);
                list_add(cola_exit, proceso_exec);
                procesos_activos--;
                sem_post(&sem_cola_exit);
                log_info(log_kernel_oblig, "Finaliza el proceso %d - Motivo: INVALID_RESOURCE", proceso_exec->pid); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", proceso_exec->pid); // log Obligatorio
                proceso_exec = NULL;
                pthread_mutex_unlock(&mutex_cola_exit);
                pthread_mutex_unlock(&mutex_procesos_activos);
                break;
            }

            pthread_mutex_lock(&mutex_lista_recurso_blocked);
            recurso_blocked->instancias_disponibles--;

            // Si hay instancias disponibles:
            if (recurso_blocked->instancias_disponibles >= 0) {
                asignar_recurso_ocupado(proceso_exec, nombre_recurso);
                log_debug(log_kernel_gral, "Una instancia del recurso %s fue asignada al proceso %d", nombre_recurso, proceso_exec->pid);
            }
            // Si NO hay instancias disponibles:
            else {
                list_add(recurso_blocked->cola_blocked, proceso_exec);
                log_info(log_kernel_oblig, "PID: %d - Bloqueado por: %s", proceso_exec->pid, nombre_recurso); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: BLOCKED", proceso_exec->pid); // log Obligatorio
                proceso_exec = NULL;
            }
            pthread_mutex_unlock(&mutex_lista_recurso_blocked);
            break;

            case SIGNAL:
            nombre_recurso = list_get(desalojo_y_argumentos, 1);

            pthread_mutex_lock(&mutex_lista_recurso_blocked);
            recurso_blocked = encontrar_recurso_blocked(nombre_recurso);
            pthread_mutex_unlock(&mutex_lista_recurso_blocked);

            if( recurso_blocked==NULL )
            {
                log_error(log_kernel_gral, "Recurso %s no encontrado en el sistema.", nombre_recurso);
                pthread_mutex_lock(&mutex_procesos_activos);
                pthread_mutex_lock(&mutex_cola_exit);
                list_add(cola_exit, proceso_exec);
                procesos_activos--;
                sem_post(&sem_cola_exit);
                log_info(log_kernel_oblig, "Finaliza el proceso %d - Motivo: INVALID_RESOURCE", proceso_exec->pid); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", proceso_exec->pid); // log Obligatorio
                proceso_exec = NULL;
                pthread_mutex_unlock(&mutex_cola_exit);
                pthread_mutex_unlock(&mutex_procesos_activos);
                break;
            }

            pthread_mutex_lock(&mutex_cola_ready);
            pthread_mutex_lock(&mutex_lista_recurso_blocked);
            recurso_blocked->instancias_disponibles++;

            recurso_ocupado = encontrar_recurso_ocupado(proceso_exec->recursos_ocupados, nombre_recurso);
            if (recurso_ocupado != NULL) {
                (recurso_ocupado->instancias)--;
            }
            else {
                log_warning(log_kernel_gral, "El proceso %d hizo SIGNAL del recurso %s antes de hacer un WAIT. Creemos que esto no sucedera en las pruebas.", proceso_exec->pid, nombre_recurso);
            }
            log_debug(log_kernel_gral, "Instancia del recurso %s liberada por el proceso %d", nombre_recurso, proceso_exec->pid);
            
            // Si hay procesos bloqueados por el recurso, desbloqueo al primero de ellos:
            if (!list_is_empty(recurso_blocked->cola_blocked)) {
                t_pcb* proceso_desbloqueado = list_remove(recurso_blocked->cola_blocked, 0);
                asignar_recurso_ocupado(proceso_desbloqueado, nombre_recurso);
                list_add(cola_ready, proceso_desbloqueado);
                char* pids_en_cola_ready = string_lista_de_pid_de_lista_de_pcb(cola_ready);
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: BLOCKED - Estado Actual: READY", proceso_desbloqueado->pid); // log Obligatorio
                log_info(log_kernel_oblig, "Cola Ready: [%s]", pids_en_cola_ready); // log Obligatorio
                log_debug(log_kernel_gral, "Una instancia del recurso %s fue asignada al proceso %d", nombre_recurso, proceso_desbloqueado->pid);
                free(pids_en_cola_ready);
            }
            pthread_mutex_unlock(&mutex_lista_recurso_blocked);
            pthread_mutex_unlock(&mutex_cola_ready);
            break;
            
            default:
            log_error(log_kernel_gral, "El motivo de desalojo del proceso %d no se puede interpretar, es desconocido.", proceso_exec->pid);
            break;
		}

        if(desalojo.motiv!=WAIT && desalojo.motiv!=SIGNAL)
        {
            hilo_exec = NULL;
        }


        //pthread_mutex_unlock(&mutex_proceso_exec);
        list_destroy_and_destroy_elements(desalojo_y_argumentos, (void*)free);
	}
}

void planific_corto_prioridades(void) {

    // DEL TP ANTERIOR: /////////////////////////////////////////////////
    // variables que defino acá porque las repito en varios case del switch
    t_paquete* paquete = NULL;
    char* nombre_interfaz = NULL;
    t_io_blocked* io = NULL;
    int cant_de_pares_direccion_tamanio;
    int* dir = NULL;
    int* tamanio = NULL;
    int fs_codigo;
    char* nombre_archivo = NULL;
    char* nombre_recurso = NULL;
    t_recurso_blocked* recurso_blocked = NULL;
    t_recurso_ocupado* recurso_ocupado = NULL;
    // Lista con data del paquete recibido desde cpu. El elemento 0 es el t_desalojo, el resto son argumentos.
    t_list* desalojo_y_argumentos = NULL;
    //////////////////////////////////////////////////////////////////////

    log_debug(log_kernel_gral, "Planificador corto plazo listo para funcionar con algoritmo PRIORIDADES.");

    // crear proceso inicial, (con su hilo main), y mandarlo a NEW.

    // EN DESARROLLO... Casi todo del TP viejo
    while(true) {

        if(hilo_exec == NULL) {
            sem_wait(&sem_procesos_ready);
            
            pthread_mutex_lock(&mutex_hilo_exec);
            pthread_mutex_lock(&mutex_cola_ready_unica);
            // pone proceso de estado READY a estado EXEC. Y envia contexto de ejecucion al cpu.
            ejecutar_siguiente_hilo(cola_ready_unica);
            log_info(log_kernel_oblig, "PID: %d - Estado Anterior: READY - Estado Actual: EXEC", proceso_exec->pid);
            hay_algun_proceso_en_exec = true;
            pthread_mutex_unlock(&mutex_cola_ready);
            pthread_mutex_unlock(&mutex_proceso_exec);
        }
        // Este es el caso en que vuelve a cpu el mismo proceso luego de un WAIT/SIGNAL exitoso:
        else {
            // solo se envia contexto de ejecucion al cpu.
            t_contexto_de_ejecucion contexto_de_ejecucion = contexto_de_ejecucion_de_pcb(proceso_exec);
            enviar_contexto_de_ejecucion(contexto_de_ejecucion, socket_cpu_dispatch);
            hay_algun_proceso_en_exec = true;
        }

        // Se queda esperando el desalojo del proceso.
        recibir_y_verificar_codigo(socket_cpu_dispatch, , );

        hay_algun_proceso_en_exec = false;

        pthread_mutex_lock(&mutex_proceso_exec);

        desalojo_y_argumentos = recibir_paquete(socket_cpu_dispatch);
		t_desalojo desalojo = deserializar_desalojo(list_get(desalojo_y_argumentos, 0));
        actualizar_contexto_de_ejecucion_de_pcb(desalojo.contexto, proceso_exec);

        // falta adaptar los nuevos motivos de desalojo ya que en este caso no hay consola
		switch (desalojo.motiv) {
			case SUCCESS:
            pthread_mutex_lock(&mutex_procesos_activos);
            pthread_mutex_lock(&mutex_cola_exit);
            list_add(cola_exit, proceso_exec);
            sem_post(&sem_procesos_exit);
            procesos_activos--;
            log_info(log_kernel_oblig, "Finaliza el proceso %d - Motivo: SUCCESS", proceso_exec->pid); // log Obligatorio.
            log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", proceso_exec->pid); // log Obligatorio.
            proceso_exec = NULL;
            pthread_mutex_unlock(&mutex_cola_exit);
            pthread_mutex_unlock(&mutex_procesos_activos);
            break;

            case OUT_OF_MEMORY:
            pthread_mutex_lock(&mutex_procesos_activos);
            pthread_mutex_lock(&mutex_cola_exit);
            list_add(cola_exit, proceso_exec);
            sem_post(&sem_procesos_exit);
            procesos_activos--;
            log_info(log_kernel_oblig, "Finaliza el proceso %d - Motivo: OUT_OF_MEMORY", proceso_exec->pid); // log Obligatorio.
            log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", proceso_exec->pid); // log Obligatorio.
            proceso_exec = NULL;
            pthread_mutex_unlock(&mutex_cola_exit);
            pthread_mutex_unlock(&mutex_procesos_activos);
            break;

            // Del TP viejo. Hay que adaptarlo a este, sólo tiene que hacer un sleep.
            case IO:
            nombre_interfaz = list_get(desalojo_y_argumentos, 1);
            int unidades_de_trabajo = *(int*)list_get(desalojo_y_argumentos, 2);

            paquete = crear_paquete(IO_OPERACION);
            agregar_a_paquete(paquete, &(proceso_exec->pid), sizeof(int));
            agregar_a_paquete(paquete, &unidades_de_trabajo, sizeof(int));

            pthread_mutex_lock(&mutex_lista_io_blocked);
            io = encontrar_io(nombre_interfaz);
            if(io != NULL) {
                enviar_paquete(paquete, io->socket);
                log_debug(log_kernel_gral, "Proceso %d empieza a usar interfaz %s", proceso_exec->pid, nombre_interfaz);
                pthread_mutex_lock(&(proceso_exec->mutex_uso_de_io));
                list_add(io->cola_blocked, proceso_exec);
                log_info(log_kernel_oblig, "PID: %d - Bloqueado por: INTERFAZ", proceso_exec->pid); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: BLOCKED", proceso_exec->pid); // log Obligatorio
                proceso_exec = NULL;
            }
            else {
                log_error(log_kernel_gral, "Interfaz %s no encontrada.", nombre_interfaz);
                pthread_mutex_lock(&mutex_procesos_activos);
                pthread_mutex_lock(&mutex_cola_exit);
                list_add(cola_exit, proceso_exec);
                procesos_activos--;
                sem_post(&sem_cola_exit);
                log_info(log_kernel_oblig, "Finaliza el proceso %d - Motivo: INVALID_INTERFACE", proceso_exec->pid); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", proceso_exec->pid); // log Obligatorio
                proceso_exec = NULL;
                pthread_mutex_unlock(&mutex_cola_exit);
                pthread_mutex_unlock(&mutex_procesos_activos);
            }
            pthread_mutex_unlock(&mutex_lista_io_blocked);

            eliminar_paquete(paquete);
            break;

            case WAIT:
            nombre_recurso = list_get(desalojo_y_argumentos, 1);

            pthread_mutex_lock(&mutex_lista_recurso_blocked);
            recurso_blocked = encontrar_recurso_blocked(nombre_recurso);
            pthread_mutex_unlock(&mutex_lista_recurso_blocked);

            if( recurso_blocked==NULL )
            {
                log_error(log_kernel_gral, "Recurso %s no encontrado en el sistema.", nombre_recurso);
                pthread_mutex_lock(&mutex_procesos_activos);
                pthread_mutex_lock(&mutex_cola_exit);
                list_add(cola_exit, proceso_exec);
                procesos_activos--;
                sem_post(&sem_cola_exit);
                log_info(log_kernel_oblig, "Finaliza el proceso %d - Motivo: INVALID_RESOURCE", proceso_exec->pid); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", proceso_exec->pid); // log Obligatorio
                proceso_exec = NULL;
                pthread_mutex_unlock(&mutex_cola_exit);
                pthread_mutex_unlock(&mutex_procesos_activos);
                break;
            }

            pthread_mutex_lock(&mutex_lista_recurso_blocked);
            recurso_blocked->instancias_disponibles--;

            // Si hay instancias disponibles:
            if (recurso_blocked->instancias_disponibles >= 0) {
                asignar_recurso_ocupado(proceso_exec, nombre_recurso);
                log_debug(log_kernel_gral, "Una instancia del recurso %s fue asignada al proceso %d", nombre_recurso, proceso_exec->pid);
            }
            // Si NO hay instancias disponibles:
            else {
                list_add(recurso_blocked->cola_blocked, proceso_exec);
                log_info(log_kernel_oblig, "PID: %d - Bloqueado por: %s", proceso_exec->pid, nombre_recurso); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: BLOCKED", proceso_exec->pid); // log Obligatorio
                proceso_exec = NULL;
            }
            pthread_mutex_unlock(&mutex_lista_recurso_blocked);
            break;

            case SIGNAL:
            nombre_recurso = list_get(desalojo_y_argumentos, 1);

            pthread_mutex_lock(&mutex_lista_recurso_blocked);
            recurso_blocked = encontrar_recurso_blocked(nombre_recurso);
            pthread_mutex_unlock(&mutex_lista_recurso_blocked);

            if( recurso_blocked==NULL )
            {
                log_error(log_kernel_gral, "Recurso %s no encontrado en el sistema.", nombre_recurso);
                pthread_mutex_lock(&mutex_procesos_activos);
                pthread_mutex_lock(&mutex_cola_exit);
                list_add(cola_exit, proceso_exec);
                procesos_activos--;
                sem_post(&sem_cola_exit);
                log_info(log_kernel_oblig, "Finaliza el proceso %d - Motivo: INVALID_RESOURCE", proceso_exec->pid); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", proceso_exec->pid); // log Obligatorio
                proceso_exec = NULL;
                pthread_mutex_unlock(&mutex_cola_exit);
                pthread_mutex_unlock(&mutex_procesos_activos);
                break;
            }

            pthread_mutex_lock(&mutex_cola_ready);
            pthread_mutex_lock(&mutex_lista_recurso_blocked);
            recurso_blocked->instancias_disponibles++;

            recurso_ocupado = encontrar_recurso_ocupado(proceso_exec->recursos_ocupados, nombre_recurso);
            if (recurso_ocupado != NULL) {
                (recurso_ocupado->instancias)--;
            }
            else {
                log_warning(log_kernel_gral, "El proceso %d hizo SIGNAL del recurso %s antes de hacer un WAIT. Creemos que esto no sucedera en las pruebas.", proceso_exec->pid, nombre_recurso);
            }
            log_debug(log_kernel_gral, "Instancia del recurso %s liberada por el proceso %d", nombre_recurso, proceso_exec->pid);
            
            // Si hay procesos bloqueados por el recurso, desbloqueo al primero de ellos:
            if (!list_is_empty(recurso_blocked->cola_blocked)) {
                t_pcb* proceso_desbloqueado = list_remove(recurso_blocked->cola_blocked, 0);
                asignar_recurso_ocupado(proceso_desbloqueado, nombre_recurso);
                list_add(cola_ready, proceso_desbloqueado);
                char* pids_en_cola_ready = string_lista_de_pid_de_lista_de_pcb(cola_ready);
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: BLOCKED - Estado Actual: READY", proceso_desbloqueado->pid); // log Obligatorio
                log_info(log_kernel_oblig, "Cola Ready: [%s]", pids_en_cola_ready); // log Obligatorio
                log_debug(log_kernel_gral, "Una instancia del recurso %s fue asignada al proceso %d", nombre_recurso, proceso_desbloqueado->pid);
                free(pids_en_cola_ready);
            }
            pthread_mutex_unlock(&mutex_lista_recurso_blocked);
            pthread_mutex_unlock(&mutex_cola_ready);
            break;
            
            default:
            log_error(log_kernel_gral, "El motivo de desalojo del proceso %d no se puede interpretar, es desconocido.", proceso_exec->pid);
            break;
		}

        if(desalojo.motiv!=WAIT && desalojo.motiv!=SIGNAL)
        {
            proceso_exec = NULL;
        }


        pthread_mutex_unlock(&mutex_proceso_exec);
        list_destroy_and_destroy_elements(desalojo_y_argumentos, (void*)free);
	}
}

// La idea es crear una cola nueva de ready por cada nivel de prioridad que se va conociendo.
void planific_corto_multinivel(void) {

    // DEL TP ANTERIOR: /////////////////////////////////////////////////
    // variables que defino acá porque las repito en varios case del switch
    t_paquete* paquete = NULL;
    char* nombre_interfaz = NULL;
    t_io_blocked* io = NULL;
    int cant_de_pares_direccion_tamanio;
    int* dir = NULL;
    int* tamanio = NULL;
    int fs_codigo;
    char* nombre_archivo = NULL;
    char* nombre_recurso = NULL;
    t_recurso_blocked* recurso_blocked = NULL;
    t_recurso_ocupado* recurso_ocupado = NULL;
    // Lista con data del paquete recibido desde cpu. El elemento 0 es el t_desalojo, el resto son argumentos.
    t_list* desalojo_y_argumentos = NULL;
    //////////////////////////////////////////////////////////////////////

    diccionario_ready_multinivel = dictionary_create();


    log_debug(log_kernel_gral, "Planificador corto plazo listo para funcionar con algoritmo COLAS MULTINIVEL.");

    // crear proceso inicial, (con su hilo main), y mandarlo a NEW.

    // EN DESARROLLO... Casi todo del TP viejo
    while(true) {

        if(hilo_exec == NULL) {
            sem_wait(&sem_procesos_ready);
            
            pthread_mutex_lock(&mutex_proceso_exec);
            pthread_mutex_lock(&mutex_cola_ready);
            // pone proceso de estado READY a estado EXEC. Y envia contexto de ejecucion al cpu.
            ejecutar_siguiente_hilo(diccionario_ready_multinivel);
            log_info(log_kernel_oblig, "PID: %d - Estado Anterior: READY - Estado Actual: EXEC", proceso_exec->pid);
            hay_algun_proceso_en_exec = true;
            pthread_mutex_unlock(&mutex_cola_ready);
            pthread_mutex_unlock(&mutex_proceso_exec);
        }
        // Este es el caso en que vuelve a cpu el mismo proceso luego de un WAIT/SIGNAL exitoso:
        else {
            // solo se envia contexto de ejecucion al cpu.
            t_contexto_de_ejecucion contexto_de_ejecucion = contexto_de_ejecucion_de_pcb(proceso_exec);
            enviar_contexto_de_ejecucion(contexto_de_ejecucion, socket_cpu_dispatch);
            hay_algun_proceso_en_exec = true;
        }

        // Se queda esperando el desalojo del proceso.
        recibir_y_verificar_codigo(socket_cpu_dispatch, , );

        hay_algun_proceso_en_exec = false;

        pthread_mutex_lock(&mutex_proceso_exec);

        desalojo_y_argumentos = recibir_paquete(socket_cpu_dispatch);
		t_desalojo desalojo = deserializar_desalojo(list_get(desalojo_y_argumentos, 0));
        actualizar_contexto_de_ejecucion_de_pcb(desalojo.contexto, proceso_exec);

        // falta adaptar los nuevos motivos de desalojo ya que en este caso no hay consola
		switch (desalojo.motiv) {
			case SUCCESS:
            pthread_mutex_lock(&mutex_procesos_activos);
            pthread_mutex_lock(&mutex_cola_exit);
            list_add(cola_exit, proceso_exec);
            sem_post(&sem_procesos_exit);
            procesos_activos--;
            log_info(log_kernel_oblig, "Finaliza el proceso %d - Motivo: SUCCESS", proceso_exec->pid); // log Obligatorio.
            log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", proceso_exec->pid); // log Obligatorio.
            proceso_exec = NULL;
            pthread_mutex_unlock(&mutex_cola_exit);
            pthread_mutex_unlock(&mutex_procesos_activos);
            break;

            case OUT_OF_MEMORY:
            pthread_mutex_lock(&mutex_procesos_activos);
            pthread_mutex_lock(&mutex_cola_exit);
            list_add(cola_exit, proceso_exec);
            sem_post(&sem_procesos_exit);
            procesos_activos--;
            log_info(log_kernel_oblig, "Finaliza el proceso %d - Motivo: OUT_OF_MEMORY", proceso_exec->pid); // log Obligatorio.
            log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", proceso_exec->pid); // log Obligatorio.
            proceso_exec = NULL;
            pthread_mutex_unlock(&mutex_cola_exit);
            pthread_mutex_unlock(&mutex_procesos_activos);
            break;

            case IO:
            nombre_interfaz = list_get(desalojo_y_argumentos, 1);
            int unidades_de_trabajo = *(int*)list_get(desalojo_y_argumentos, 2);

            paquete = crear_paquete(IO_OPERACION);
            agregar_a_paquete(paquete, &(proceso_exec->pid), sizeof(int));
            agregar_a_paquete(paquete, &unidades_de_trabajo, sizeof(int));

            pthread_mutex_lock(&mutex_lista_io_blocked);
            io = encontrar_io(nombre_interfaz);
            if(io != NULL) {
                enviar_paquete(paquete, io->socket);
                log_debug(log_kernel_gral, "Proceso %d empieza a usar interfaz %s", proceso_exec->pid, nombre_interfaz);
                pthread_mutex_lock(&(proceso_exec->mutex_uso_de_io));
                list_add(io->cola_blocked, proceso_exec);
                log_info(log_kernel_oblig, "PID: %d - Bloqueado por: INTERFAZ", proceso_exec->pid); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: BLOCKED", proceso_exec->pid); // log Obligatorio
                proceso_exec = NULL;
            }
            else {
                log_error(log_kernel_gral, "Interfaz %s no encontrada.", nombre_interfaz);
                pthread_mutex_lock(&mutex_procesos_activos);
                pthread_mutex_lock(&mutex_cola_exit);
                list_add(cola_exit, proceso_exec);
                procesos_activos--;
                sem_post(&sem_cola_exit);
                log_info(log_kernel_oblig, "Finaliza el proceso %d - Motivo: INVALID_INTERFACE", proceso_exec->pid); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", proceso_exec->pid); // log Obligatorio
                proceso_exec = NULL;
                pthread_mutex_unlock(&mutex_cola_exit);
                pthread_mutex_unlock(&mutex_procesos_activos);
            }
            pthread_mutex_unlock(&mutex_lista_io_blocked);

            eliminar_paquete(paquete);
            break;

            case WAIT:
            nombre_recurso = list_get(desalojo_y_argumentos, 1);

            pthread_mutex_lock(&mutex_lista_recurso_blocked);
            recurso_blocked = encontrar_recurso_blocked(nombre_recurso);
            pthread_mutex_unlock(&mutex_lista_recurso_blocked);

            if( recurso_blocked==NULL )
            {
                log_error(log_kernel_gral, "Recurso %s no encontrado en el sistema.", nombre_recurso);
                pthread_mutex_lock(&mutex_procesos_activos);
                pthread_mutex_lock(&mutex_cola_exit);
                list_add(cola_exit, proceso_exec);
                procesos_activos--;
                sem_post(&sem_cola_exit);
                log_info(log_kernel_oblig, "Finaliza el proceso %d - Motivo: INVALID_RESOURCE", proceso_exec->pid); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", proceso_exec->pid); // log Obligatorio
                proceso_exec = NULL;
                pthread_mutex_unlock(&mutex_cola_exit);
                pthread_mutex_unlock(&mutex_procesos_activos);
                break;
            }

            pthread_mutex_lock(&mutex_lista_recurso_blocked);
            recurso_blocked->instancias_disponibles--;

            // Si hay instancias disponibles:
            if (recurso_blocked->instancias_disponibles >= 0) {
                asignar_recurso_ocupado(proceso_exec, nombre_recurso);
                log_debug(log_kernel_gral, "Una instancia del recurso %s fue asignada al proceso %d", nombre_recurso, proceso_exec->pid);
            }
            // Si NO hay instancias disponibles:
            else {
                list_add(recurso_blocked->cola_blocked, proceso_exec);
                log_info(log_kernel_oblig, "PID: %d - Bloqueado por: %s", proceso_exec->pid, nombre_recurso); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: BLOCKED", proceso_exec->pid); // log Obligatorio
                proceso_exec = NULL;
            }
            pthread_mutex_unlock(&mutex_lista_recurso_blocked);
            break;

            case SIGNAL:
            nombre_recurso = list_get(desalojo_y_argumentos, 1);

            pthread_mutex_lock(&mutex_lista_recurso_blocked);
            recurso_blocked = encontrar_recurso_blocked(nombre_recurso);
            pthread_mutex_unlock(&mutex_lista_recurso_blocked);

            if( recurso_blocked==NULL )
            {
                log_error(log_kernel_gral, "Recurso %s no encontrado en el sistema.", nombre_recurso);
                pthread_mutex_lock(&mutex_procesos_activos);
                pthread_mutex_lock(&mutex_cola_exit);
                list_add(cola_exit, proceso_exec);
                procesos_activos--;
                sem_post(&sem_cola_exit);
                log_info(log_kernel_oblig, "Finaliza el proceso %d - Motivo: INVALID_RESOURCE", proceso_exec->pid); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", proceso_exec->pid); // log Obligatorio
                proceso_exec = NULL;
                pthread_mutex_unlock(&mutex_cola_exit);
                pthread_mutex_unlock(&mutex_procesos_activos);
                break;
            }

            pthread_mutex_lock(&mutex_cola_ready);
            pthread_mutex_lock(&mutex_lista_recurso_blocked);
            recurso_blocked->instancias_disponibles++;

            recurso_ocupado = encontrar_recurso_ocupado(proceso_exec->recursos_ocupados, nombre_recurso);
            if (recurso_ocupado != NULL) {
                (recurso_ocupado->instancias)--;
            }
            else {
                log_warning(log_kernel_gral, "El proceso %d hizo SIGNAL del recurso %s antes de hacer un WAIT. Creemos que esto no sucedera en las pruebas.", proceso_exec->pid, nombre_recurso);
            }
            log_debug(log_kernel_gral, "Instancia del recurso %s liberada por el proceso %d", nombre_recurso, proceso_exec->pid);
            
            // Si hay procesos bloqueados por el recurso, desbloqueo al primero de ellos:
            if (!list_is_empty(recurso_blocked->cola_blocked)) {
                t_pcb* proceso_desbloqueado = list_remove(recurso_blocked->cola_blocked, 0);
                asignar_recurso_ocupado(proceso_desbloqueado, nombre_recurso);
                list_add(cola_ready, proceso_desbloqueado);
                char* pids_en_cola_ready = string_lista_de_pid_de_lista_de_pcb(cola_ready);
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: BLOCKED - Estado Actual: READY", proceso_desbloqueado->pid); // log Obligatorio
                log_info(log_kernel_oblig, "Cola Ready: [%s]", pids_en_cola_ready); // log Obligatorio
                log_debug(log_kernel_gral, "Una instancia del recurso %s fue asignada al proceso %d", nombre_recurso, proceso_desbloqueado->pid);
                free(pids_en_cola_ready);
            }
            pthread_mutex_unlock(&mutex_lista_recurso_blocked);
            pthread_mutex_unlock(&mutex_cola_ready);
            break;
            
            default:
            log_error(log_kernel_gral, "El motivo de desalojo del proceso %d no se puede interpretar, es desconocido.", proceso_exec->pid);
            break;
		}

        if(desalojo.motiv!=WAIT && desalojo.motiv!=SIGNAL)
        {
            proceso_exec = NULL;
        }


        pthread_mutex_unlock(&mutex_proceso_exec);
        list_destroy_and_destroy_elements(desalojo_y_argumentos, (void*)free);
	}
}

// DEL TP VIEJO, COMO REFERENCIA
/*
void planific_corto_rr(void) {

    // Lista con data del paquete recibido desde cpu. El elemento 0 es el t_desalojo, el resto son argumentos.
    t_list* desalojo_y_argumentos = NULL;

    // variables que defino acá porque las repito en varios case del switch
    t_paquete* paquete = NULL;
    char* nombre_interfaz = NULL;
    t_io_blocked* io = NULL;
    int cant_de_pares_direccion_tamanio;
    int* dir = NULL;
    int* tamanio = NULL;
    int fs_codigo;
    char* nombre_archivo = NULL;
    char* nombre_recurso = NULL;
    t_recurso_blocked* recurso_blocked = NULL;
    t_recurso_ocupado* recurso_ocupado = NULL;
    int backup_pid_de_proceso_en_exec;

    log_debug(log_kernel_gral, "Planificador corto plazo listo para funcionar con algoritmo RR.");

    while(1) {

        if(proceso_exec == NULL) {
            sem_wait(&sem_procesos_ready);
            
            pthread_mutex_lock(&mutex_proceso_exec);
            pthread_mutex_lock(&mutex_cola_ready);
            // pone proceso de estado READY a estado EXEC. Y envia contexto de ejecucion al cpu.
            ejecutar_sig_proceso();
            log_info(log_kernel_oblig, "PID: %d - Estado Anterior: READY - Estado Actual: EXEC", proceso_exec->pid);
            hay_algun_proceso_en_exec = true;
            backup_pid_de_proceso_en_exec = proceso_exec->pid;
            pthread_mutex_unlock(&mutex_cola_ready);
            pthread_mutex_unlock(&mutex_proceso_exec);

        }
        // Este es el caso en que vuelve a cpu el mismo proceso luego de un WAIT/SIGNAL exitoso:
        else {
            pthread_mutex_lock(&mutex_proceso_exec);
            // actualiza quantum y envia contexto de ejecucion al cpu.
            proceso_exec->quantum -= ms_transcurridos; // SE PUEDE MEJORAR. ES PROVISORIO
            t_contexto_de_ejecucion contexto_de_ejecucion = contexto_de_ejecucion_de_pcb(proceso_exec);
            enviar_contexto_de_ejecucion(contexto_de_ejecucion, socket_cpu_dispatch);
            hay_algun_proceso_en_exec = true;
            backup_pid_de_proceso_en_exec = proceso_exec->pid;
            pthread_mutex_unlock(&mutex_proceso_exec);
        }

        // Pone a correr el quantum y se queda esperando el desalojo del proceso.
        esperar_cpu_rr();

        hay_algun_proceso_en_exec = false;
        log_debug(log_kernel_gral, "Milisegundos aprox. de tiempo de proceso %d en cpu: %d", backup_pid_de_proceso_en_exec, ms_transcurridos);

        pthread_mutex_lock(&mutex_proceso_exec);

        desalojo_y_argumentos = recibir_paquete(socket_cpu_dispatch);
		t_desalojo desalojo = deserializar_desalojo(list_get(desalojo_y_argumentos, 0));
        actualizar_contexto_de_ejecucion_de_pcb(desalojo.contexto, proceso_exec);

        // falta adaptar los nuevos motivos de desalojo ya que en este caso no hay consola
		switch (desalojo.motiv) {
			case SUCCESS:
            pthread_mutex_lock(&mutex_procesos_activos);
            pthread_mutex_lock(&mutex_cola_exit);
            list_add(cola_exit, proceso_exec);
            sem_post(&sem_procesos_exit);
            procesos_activos--;
            log_info(log_kernel_oblig, "Finaliza el proceso %d - Motivo: SUCCESS", proceso_exec->pid); // log Obligatorio.
            log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", proceso_exec->pid); // log Obligatorio.
            proceso_exec = NULL;
            pthread_mutex_unlock(&mutex_cola_exit);
            pthread_mutex_unlock(&mutex_procesos_activos);
            break;

            case OUT_OF_MEMORY:
            pthread_mutex_lock(&mutex_procesos_activos);
            pthread_mutex_lock(&mutex_cola_exit);
            list_add(cola_exit, proceso_exec);
            sem_post(&sem_procesos_exit);
            procesos_activos--;
            log_info(log_kernel_oblig, "Finaliza el proceso %d - Motivo: OUT_OF_MEMORY", proceso_exec->pid); // log Obligatorio.
            log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", proceso_exec->pid); // log Obligatorio.
            proceso_exec = NULL;
            pthread_mutex_unlock(&mutex_cola_exit);
            pthread_mutex_unlock(&mutex_procesos_activos);
            break;

			case INTERRUPTED_BY_USER:
            pthread_mutex_lock(&mutex_procesos_activos);
            pthread_mutex_lock(&mutex_cola_exit);
            list_add(cola_exit, proceso_exec);
            sem_post(&sem_procesos_exit);
            procesos_activos--;
            log_info(log_kernel_oblig, "Finaliza el proceso %d - Motivo: INTERRUPTED_BY_USER", proceso_exec->pid); // log Obligatorio.
            log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", proceso_exec->pid); // log Obligatorio.
            proceso_exec = NULL;
            pthread_mutex_unlock(&mutex_cola_exit);
            pthread_mutex_unlock(&mutex_procesos_activos);
            break;

            case INTERRUPTED_BY_QUANTUM:
            pthread_mutex_lock(&mutex_cola_ready);
            proceso_exec->quantum = quantum_de_config;
            list_add(cola_ready, proceso_exec);
            sem_post(&sem_procesos_ready);
            char* pids_en_cola_ready = string_lista_de_pid_de_lista_de_pcb(cola_ready);
            log_info(log_kernel_oblig, "PID: %d - Desalojado por fin de Quantum", proceso_exec->pid); // log Obligatorio
            log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: READY", proceso_exec->pid); // log Obligatorio
            log_info(log_kernel_oblig, "Cola Ready: [%s]", pids_en_cola_ready); // log Obligatorio
            proceso_exec = NULL;
            free(pids_en_cola_ready);
            pthread_mutex_unlock(&mutex_cola_ready);
            break;

            case GEN_SLEEP:
            nombre_interfaz = list_get(desalojo_y_argumentos, 1);
            int unidades_de_trabajo = *(int*)list_get(desalojo_y_argumentos, 2);

            paquete = crear_paquete(IO_OPERACION);
            agregar_a_paquete(paquete, &(proceso_exec->pid), sizeof(int));
            agregar_a_paquete(paquete, &unidades_de_trabajo, sizeof(int));

            pthread_mutex_lock(&mutex_lista_io_blocked);
            io = encontrar_io(nombre_interfaz);
            if(io != NULL) {
                enviar_paquete(paquete, io->socket);
                log_debug(log_kernel_gral, "Proceso %d empieza a usar interfaz %s", proceso_exec->pid, nombre_interfaz);
                pthread_mutex_lock(&(proceso_exec->mutex_uso_de_io));
                proceso_exec->quantum = quantum_de_config;
                list_add(io->cola_blocked, proceso_exec);
                log_info(log_kernel_oblig, "PID: %d - Bloqueado por: INTERFAZ", proceso_exec->pid); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: BLOCKED", proceso_exec->pid); // log Obligatorio
                proceso_exec = NULL;
            }
            else {
                log_error(log_kernel_gral, "Interfaz %s no encontrada.", nombre_interfaz);
                pthread_mutex_lock(&mutex_procesos_activos);
                pthread_mutex_lock(&mutex_cola_exit);
                list_add(cola_exit, proceso_exec);
                procesos_activos--;
                sem_post(&sem_procesos_exit);
                log_info(log_kernel_oblig, "Finaliza el proceso %d - Motivo: INVALID_INTERFACE", proceso_exec->pid); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", proceso_exec->pid); // log Obligatorio
                proceso_exec = NULL;
                pthread_mutex_unlock(&mutex_cola_exit);
                pthread_mutex_unlock(&mutex_procesos_activos);
            }
            pthread_mutex_unlock(&mutex_lista_io_blocked);

            eliminar_paquete(paquete);
            break;

            case WAIT:
            nombre_recurso = list_get(desalojo_y_argumentos, 1);

            pthread_mutex_lock(&mutex_lista_recurso_blocked);
            recurso_blocked = encontrar_recurso_blocked(nombre_recurso);
            pthread_mutex_unlock(&mutex_lista_recurso_blocked);

            if( recurso_blocked==NULL )
            {
                log_error(log_kernel_gral, "Recurso %s no encontrado en el sistema.", nombre_recurso);
                pthread_mutex_lock(&mutex_procesos_activos);
                pthread_mutex_lock(&mutex_cola_exit);
                list_add(cola_exit, proceso_exec);
                procesos_activos--;
                sem_post(&sem_procesos_exit);
                log_info(log_kernel_oblig, "Finaliza el proceso %d - Motivo: INVALID_RESOURCE", proceso_exec->pid); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", proceso_exec->pid); // log Obligatorio
                proceso_exec = NULL;
                pthread_mutex_unlock(&mutex_cola_exit);
                pthread_mutex_unlock(&mutex_procesos_activos);
                break;
            }

            pthread_mutex_lock(&mutex_lista_recurso_blocked);
            recurso_blocked->instancias_disponibles--;

            // Si hay instancias disponibles:
            if (recurso_blocked->instancias_disponibles >= 0) {
                asignar_recurso_ocupado(proceso_exec, nombre_recurso);
                log_debug(log_kernel_gral, "Una instancia del recurso %s fue asignada al proceso %d", nombre_recurso, proceso_exec->pid);
            }
            // Si NO hay instancias disponibles:
            else {
                proceso_exec->quantum = quantum_de_config;
                list_add(recurso_blocked->cola_blocked, proceso_exec);
                log_info(log_kernel_oblig, "PID: %d - Bloqueado por: %s", proceso_exec->pid, nombre_recurso); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: BLOCKED", proceso_exec->pid); // log Obligatorio
                proceso_exec = NULL;
            }
            pthread_mutex_unlock(&mutex_lista_recurso_blocked);
            break;

            case SIGNAL:
            nombre_recurso = list_get(desalojo_y_argumentos, 1);

            pthread_mutex_lock(&mutex_lista_recurso_blocked);
            recurso_blocked = encontrar_recurso_blocked(nombre_recurso);
            pthread_mutex_unlock(&mutex_lista_recurso_blocked);

            if( recurso_blocked==NULL )
            {
                log_error(log_kernel_gral, "Recurso %s no encontrado en el sistema.", nombre_recurso);
                pthread_mutex_lock(&mutex_procesos_activos);
                pthread_mutex_lock(&mutex_cola_exit);
                list_add(cola_exit, proceso_exec);
                procesos_activos--;
                sem_post(&sem_procesos_exit);
                log_info(log_kernel_oblig, "Finaliza el proceso %d - Motivo: INVALID_RESOURCE", proceso_exec->pid); // log Obligatorio
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: EXIT", proceso_exec->pid); // log Obligatorio
                proceso_exec = NULL;
                pthread_mutex_unlock(&mutex_cola_exit);
                pthread_mutex_unlock(&mutex_procesos_activos);
                break;
            }

            pthread_mutex_lock(&mutex_cola_ready);
            pthread_mutex_lock(&mutex_lista_recurso_blocked);
            recurso_blocked->instancias_disponibles++;

            recurso_ocupado = encontrar_recurso_ocupado(proceso_exec->recursos_ocupados, nombre_recurso);
            if (recurso_ocupado != NULL) {
                (recurso_ocupado->instancias)--;
            }
            else {
                log_warning(log_kernel_gral, "El proceso %d hizo SIGNAL del recurso %s antes de hacer un WAIT. Creemos que esto no sucedera en las pruebas.", proceso_exec->pid, nombre_recurso);
            }
            log_debug(log_kernel_gral, "Instancia del recurso %s liberada por el proceso %d", nombre_recurso, proceso_exec->pid);
            
            // Si hay procesos bloqueados por el recurso, desbloqueo al primero de ellos:
            if (!list_is_empty(recurso_blocked->cola_blocked)) {
                t_pcb* proceso_desbloqueado = list_remove(recurso_blocked->cola_blocked, 0);
                asignar_recurso_ocupado(proceso_desbloqueado, nombre_recurso);
                list_add(cola_ready, proceso_desbloqueado);
                sem_post(&sem_procesos_ready);
                char* pids_en_cola_ready = string_lista_de_pid_de_lista_de_pcb(cola_ready);
                log_info(log_kernel_oblig, "PID: %d - Estado Anterior: BLOCKED - Estado Actual: READY", proceso_desbloqueado->pid); // log Obligatorio
                log_info(log_kernel_oblig, "Cola Ready: [%s]", pids_en_cola_ready); // log Obligatorio
                log_debug(log_kernel_gral, "Una instancia del recurso %s fue asignada al proceso %d", nombre_recurso, proceso_desbloqueado->pid);
                free(pids_en_cola_ready);
            }
            pthread_mutex_unlock(&mutex_lista_recurso_blocked);
            pthread_mutex_unlock(&mutex_cola_ready);
            break;
            
            default:
            log_error(log_kernel_gral, "El motivo de desalojo del proceso %d no se puede interpretar, es desconocido.", proceso_exec->pid);
            break;
            
		}

        if(desalojo.motiv!=WAIT && desalojo.motiv!=SIGNAL)
        {
            proceso_exec = NULL;
        }


        pthread_mutex_unlock(&mutex_proceso_exec);
        list_destroy_and_destroy_elements(desalojo_y_argumentos, (void*)free);
	}

}
*/

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// ==========================================================================
// ====  Funciones Externas:  ===============================================
// ==========================================================================

void ejecutar_siguiente_hilo(t_list* cola_ready) {
    hilo_exec = list_remove(cola_ready, 0);
    enviar_orden_de_ejecucion_al_cpu(hilo_exec);
    log_debug(log_kernel_gral, "## (%d:%d) - EJECUTANDO", hilo_exec->pid_pertenencia, hilo_exec->tid);
}

t_list* recibir_de_cpu(int* codigo_operacion) {
    t_list* argumentos_recibidos = NULL;
    *codigo_operacion = recibir_codigo(socket_cpu_dispatch);
    if (*codigo_operacion >= 0) {
        argumentos_recibidos = recibir_paquete(socket_cpu_dispatch);
    }
    else {
        log_error(log_kernel_gral, "No se pudo recibir el codigo de operacion enviado por CPU.");
    }
    return argumentos_recibidos;
}

// ==========================================================================
// ====  Funciones Internas:  ===============================================
// ==========================================================================

t_pcb* nuevo_proceso(int tamanio, int prioridad_hilo_main, char* path_instruc_hilo_main) {
    t_pcb* nuevo_pcb = crear_pcb(contador_pid, tamanio);
    contador_pid++;
    t_tcb* nuevo_tcb = nuevo_hilo(nuevo_pcb, prioridad_hilo_main, path_instruc_hilo_main);
    nuevo_pcb->hilo_main = nuevo_tcb;
    return nuevo_pcb;
}

void ingresar_a_new(t_pcb* pcb) {
    list_add(cola_new, pcb);
    sem_post(&sem_cola_new);
}

t_tcb* nuevo_hilo(t_pcb* pcb_creador, int prioridad, char* path_instrucciones) {
    t_tcb* nuevo_tcb = crear_tcb(pcb_creador->pid, pcb_creador->sig_tid_a_asignar, prioridad, path_instrucciones);
    pcb_creador->sig_tid_a_asignar++;
    asociar_tid(pcb_creador, nuevo_tcb);
    return nuevo_tcb;
}

void enviar_orden_de_ejecucion_al_cpu(t_tcb* tcb) {
    t_paquete* paquete = crear_paquete(EJECUCION);
    agregar_a_paquete(paquete, (void*)&(tcb->pid_pertenencia), sizeof(int));
    agregar_a_paquete(paquete, (void*)&(tcb->tid), sizeof(int));
    enviar_paquete(paquete, socket_cpu_dispatch);
    eliminar_paquete(paquete);
}

void enviar_pedido_de_dump_a_memoria(t_tcb* tcb) {
    int socket_memoria = crear_conexion(ip_memoria, puerto_memoria);
    enviar_pedido_de_dump(tcb->pid_pertenencia, tcb->tid, socket_memoria);
    pthread_mutex_lock(&mutex_cola_blocked_memory_dump);
    list_add(cola_blocked_memory_dump, tcb);
    pthread_mutex_unlock(&mutex_cola_blocked_memory_dump);
    
    t_recepcion_respuesta_memory_dump* info_para_recibir_rta = malloc(sizeof(t_recepcion_respuesta_memory_dump));
    info_para_recibir_rta->tcb = tcb;
    info_para_recibir_rta->socket_de_la_conexion = socket_memoria;
    // acá no debería haber problema de memory leak, pues al terminar el hilo detacheado, lo liberaría.
    pthread_t* thread_respuesta_memory_dump = malloc(sizeof(pthread_t));
    pthread_create(thread_respuesta_memory_dump, NULL, rutina_respuesta_memory_dump, (void*)info_para_recibir_rta);
    pthread_detach(*thread_respuesta_memory_dump);
}

void manejar_solicitud_io(t_tcb* hilo_exec, int unidades_trabajo) {
    log_info(log_kernel_oblig, "PID: %d - Estado Anterior: EXEC - Estado Actual: BLOCKED (IO)", hilo_exec->tid);

    // Mover el hilo a la cola de BLOQUEADOS para IO
    pthread_mutex_lock(&mutex_cola_blocked_io);
    list_add(cola_blocked_io, hilo_exec);
    pthread_mutex_unlock(&mutex_cola_blocked_io);

    // Simular el tiempo de IO con un sleep en un hilo separado para no bloquear el planificador
    pthread_t hilo_io;
    pthread_create(&hilo_io, NULL, (void*) esperar_y_mover_a_ready, (void*) hilo_exec);
    pthread_detach(hilo_io); // Desconecta el hilo para que no sea necesario join
}

void esperar_y_mover_a_ready(t_tcb* hilo_exec) {
    // Simular IO
    sleep(hilo_exec->unidades_trabajo);  // Tiempo en IO

    // Mover el hilo de vuelta a READY
    pthread_mutex_lock(&mutex_cola_blocked_io);
    list_remove_by_condition(cola_blocked_io, (void*) hilo_exec);  // Eliminar de BLOCKED IO
    pthread_mutex_unlock(&mutex_cola_blocked_io);

    pthread_mutex_lock(&mutex_cola_ready_unica);
    list_add(cola_ready_unica, hilo_exec);
    log_info(log_kernel_oblig, "PID: %d - Estado Anterior: BLOCKED - Estado Actual: READY", hilo_exec->tid);
    pthread_mutex_unlock(&mutex_cola_ready_unica);
}


// ==========================================================================
// ====  Funciones Auxiliares:  =============================================
// ==========================================================================

t_pcb* crear_pcb(int pid, int tamanio) {
    t_pcb* pcb = malloc(sizeof(t_pcb));
    pcb->pid = pid;
    pcb->tamanio = tamanio;
    pcb->tids_asociados = list_create();
    pcb->mutex_creados = list_create();
    pcb->sig_tid_a_asignar = 0;
    pcb->hilo_main = NULL;
    return pcb;
}

void enviar_pedido_de_dump(int pid, int tid, int socket) {
    t_paquete* paquete = crear_paquete(MEMORY_DUMP);
    agregar_a_paquete(paquete, (void*)&pid, sizeof(int));
    agregar_a_paquete(paquete, (void*)&tid, sizeof(int));
    enviar_paquete(paquete, socket);
    eliminar_paquete(paquete);
}


/* OBSOLETO. LO DEJO POR LAS DUDAS ==================================
t_recurso* encontrar_recurso_del_sistema(char* nombre) {

	bool _es_mi_recurso(t_recurso* recurso) {
		return strcmp(recurso->nombre, nombre) == 0;
	}

    return list_find(recursos_del_sistema, (void*)_es_mi_recurso);
}

t_recurso_ocupado* encontrar_recurso_ocupado(t_list* lista_de_recursos_ocupados, char* nombre) {

	bool _es_mi_recurso_ocupado(t_recurso_ocupado* recurso) {
		return strcmp(recurso->nombre, nombre) == 0;
	}

    return list_find(lista_de_recursos_ocupados, (void*)_es_mi_recurso_ocupado);
}

t_recurso_blocked* encontrar_recurso_blocked(char* nombre) {

	bool _es_mi_recurso_blocked(t_recurso_blocked* recurso) {
		return strcmp(recurso->nombre, nombre) == 0;
	}

    return list_find(lista_recurso_blocked, (void*)_es_mi_recurso_blocked);
}

void asignar_recurso_ocupado(t_pcb* pcb, char* nombre_recurso) {
    t_recurso_ocupado* recurso_ocupado = encontrar_recurso_ocupado(pcb->recursos_ocupados, nombre_recurso);
    if (recurso_ocupado != NULL) {
        (recurso_ocupado->instancias)++;
    }
    else {
        recurso_ocupado = malloc(sizeof(t_recurso_ocupado));
        recurso_ocupado->nombre = string_duplicate(nombre_recurso);
        recurso_ocupado->instancias = 1;
        list_add(proceso_exec->recursos_ocupados, recurso_ocupado);
    }
}
=====================================================================
*/
