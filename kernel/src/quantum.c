#include "quantum.h"

t_temporal* timer;
int ms_transcurridos;

pthread_mutex_t mutex_rutina_quantum;

//////////////////////////////////////

void* rutina_quantum(void *puntero_null) {
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    unsigned int quantum_en_microsegs = (proceso_exec->quantum)*MILISEG_A_MICROSEG;
    usleep(quantum_en_microsegs);

    pthread_mutex_lock(&mutex_rutina_quantum);
    enviar_orden_de_interrupcion(DESALOJAR);
    pthread_mutex_unlock(&mutex_rutina_quantum);
    
    return NULL;
}

void esperar_cpu_rr(void)
{   pthread_mutex_init(&mutex_rutina_quantum, NULL);
    pthread_t hilo_quantum;
    timer = temporal_create();
    pthread_create(&hilo_quantum, NULL, rutina_quantum, NULL);
    pthread_detach(hilo_quantum);

    recibir_y_verificar_codigo(socket_cpu_dispatch, DESALOJO, "DESALOJO");

    temporal_stop(timer);
    pthread_mutex_lock(&mutex_rutina_quantum);
    pthread_cancel(hilo_quantum);
    ms_transcurridos = temporal_gettime(timer);
    temporal_destroy(timer);
    pthread_mutex_unlock(&mutex_rutina_quantum);
}