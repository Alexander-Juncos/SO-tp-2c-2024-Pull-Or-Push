#include "quantum.h"

t_temporal* timer;
int ms_transcurridos = 0;
bool interrumpir_inmediatamente = false;

pthread_mutex_t mutex_rutina_quantum;

/* TODO SACADO DEL TP ANTERIOR!! */

//////////////////////////////////////

void* rutina_quantum(void *puntero_null) {
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    if(!interrumpir_inmediatamente) {
        unsigned int quantum_en_microsegs = (quantum_de_config - ms_transcurridos)*MILISEG_A_MICROSEG;

        usleep(quantum_en_microsegs);

        pthread_mutex_lock(&mutex_rutina_quantum);
        enviar_orden_de_interrupcion();
        pthread_mutex_unlock(&mutex_rutina_quantum);
    }
    
    return NULL;
}

void esperar_cpu_rr(void) {
    pthread_mutex_init(&mutex_rutina_quantum, NULL);
    pthread_t hilo_quantum;
    timer = temporal_create();
    pthread_create(&hilo_quantum, NULL, rutina_quantum, NULL);
    pthread_detach(hilo_quantum);

    // Falta definir esta funcion.
    recibir_de_cpu(socket_cpu_dispatch, , );

    temporal_stop(timer);
    pthread_mutex_lock(&mutex_rutina_quantum);
    pthread_cancel(hilo_quantum);
    ms_transcurridos += temporal_gettime(timer);
    if(ms_transcurridos >= quantum_de_config) {
        interrumpir_inmediatamente = true;
    }
    temporal_destroy(timer);
    pthread_mutex_unlock(&mutex_rutina_quantum);
}

void reiniciar_quantum(void) {
    ms_transcurridos = 0;
    interrumpir_inmediatamente = false;
}
