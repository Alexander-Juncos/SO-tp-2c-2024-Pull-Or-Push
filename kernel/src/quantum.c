#include "quantum.h"


t_temporal* timer;
int ms_transcurridos = 0;
bool interrumpir_inmediatamente = false;

pthread_mutex_t mutex_rutina_quantum;

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

t_list* esperar_cpu_rr(int* codigo_operacion) {
    pthread_mutex_init(&mutex_rutina_quantum, NULL);
    pthread_t thread_quantum;
    timer = temporal_create();
    pthread_create(&thread_quantum, NULL, rutina_quantum, NULL);
    pthread_detach(thread_quantum);

    t_list* argumentos_recibidos = NULL;
    argumentos_recibidos = recibir_de_cpu(codigo_operacion);

    temporal_stop(timer);
    pthread_mutex_lock(&mutex_rutina_quantum);
    pthread_cancel(thread_quantum);
    ms_transcurridos += temporal_gettime(timer);
    if(ms_transcurridos >= quantum_de_config) {
        interrumpir_inmediatamente = true;
    }
    temporal_destroy(timer);
    pthread_mutex_unlock(&mutex_rutina_quantum);
    return argumentos_recibidos;
}

void reiniciar_quantum(void) {
    ms_transcurridos = 0;
    interrumpir_inmediatamente = false;
}
