#include "planificador.h"
#include "quantum.h"

/* TODO SACADO DEL TP ANTERIOR!! */

void* rutina_planificador(void* puntero_null) {

    char* algoritmo_planificacion = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
    hay_algun_proceso_en_exec = false;

    log_debug(log_kernel_gral, "Hilo de Planificador corto plazo iniciado.");

    if (strcmp(algoritmo_planificacion, "FIFO") == 0) {
        planific_corto_fifo();
    }
    else if (strcmp(algoritmo_planificacion, "PRIORIDADES") == 0) {
        planific_corto_prioridades();
    }
    else if (strcmp(algoritmo_planificacion, "MULTINIVEL") == 0) {
        planific_corto_multinivel();
    }
    
    return NULL;
}

/////////////////////////////////////////////////////

void planific_corto_fifo(void) {

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

    log_debug(log_kernel_gral, "Planificador corto plazo listo para funcionar con algoritmo FIFO.");

    while(1) {

        if(proceso_exec == NULL) {
            sem_wait(&sem_procesos_ready);
            
            pthread_mutex_lock(&mutex_proceso_exec);
            pthread_mutex_lock(&mutex_cola_ready);
            // pone proceso de estado READY a estado EXEC. Y envia contexto de ejecucion al cpu.
            ejecutar_sig_proceso();
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
        recibir_y_verificar_codigo(socket_cpu_dispatch, DESALOJO, "DESALOJO");

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

void planific_corto_prioridades(void) {

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

    log_debug(log_kernel_gral, "Planificador corto plazo listo para funcionar con algoritmo FIFO.");

    while(1) {

        if(proceso_exec == NULL) {
            sem_wait(&sem_procesos_ready);
            
            pthread_mutex_lock(&mutex_proceso_exec);
            pthread_mutex_lock(&mutex_cola_ready);
            // pone proceso de estado READY a estado EXEC. Y envia contexto de ejecucion al cpu.
            ejecutar_sig_proceso();
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
        recibir_y_verificar_codigo(socket_cpu_dispatch, DESALOJO, "DESALOJO");

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

// Este faltaría adaptarlo bien a las colas, la idea es crear una cola nueva de ready por cada nivel de prioridad que se va conociendo.
void planific_corto_multinivel(void) {

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

    log_debug(log_kernel_gral, "Planificador corto plazo listo para funcionar con algoritmo FIFO.");

    while(1) {

        if(proceso_exec == NULL) {
            sem_wait(&sem_procesos_ready);
            
            pthread_mutex_lock(&mutex_proceso_exec);
            pthread_mutex_lock(&mutex_cola_ready);
            // pone proceso de estado READY a estado EXEC. Y envia contexto de ejecucion al cpu.
            ejecutar_sig_proceso();
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
        recibir_y_verificar_codigo(socket_cpu_dispatch, DESALOJO, "DESALOJO");

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

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////


void ejecutar_sig_proceso(void) {

    proceso_exec = list_remove(cola_ready, 0);
    
    t_contexto_de_ejecucion contexto_de_ejecucion = contexto_de_ejecucion_de_pcb(proceso_exec);
    enviar_contexto_de_ejecucion(contexto_de_ejecucion, socket_cpu_dispatch);
}

void recibir_y_verificar_codigo(int socket, op_code cod, char* traduccion_de_cod) {
    if (recibir_codigo(socket) != cod) {
        log_error(log_kernel_gral, "Codigo erroneo. Se esperaba %s.", traduccion_de_cod);
    }
}

/* OBSOLETO. SE PUEDE SACAR
t_recurso* encontrar_recurso_del_sistema(char* nombre) {

	bool _es_mi_recurso(t_recurso* recurso) {
		return strcmp(recurso->nombre, nombre) == 0;
	}

    return list_find(recursos_del_sistema, (void*)_es_mi_recurso);
}
*/

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