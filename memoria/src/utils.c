#include "utils.h"

// ==========================================================================
// ====  Variables globales:  ===============================================
// ==========================================================================

int socket_cpu = 1;
int socket_escucha = 1;
int socket_cliente_temp = 1; // para que los hilos puedan tomar su cliente, protegido x semaforo (a implementar)

t_config *config; 
t_log *log_memoria_oblig; 
t_log *log_memoria_gral; 

// ==========================================================================
// ====  Funciones Internas:  ===============================================
// ==========================================================================



// ==========================================================================
// ====  Funciones Auxiliares:  =============================================
// ==========================================================================

void retardo_operacion()
{
    unsigned int tiempo_en_microsegs = config_get_int_value(config, "RETARDO_RESPUESTA")*MILISEG_A_MICROSEG;
    usleep(tiempo_en_microsegs);
}