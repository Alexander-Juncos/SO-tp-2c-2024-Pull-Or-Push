#include "respuesta_memory_dump.h"

// variables globales para este hilo:
// ============================================
// por ahora ninguna...
// ============================================

void* rutina_respuesta_memory_dump(void* socket_de_la_conexion) {
    // DESARROLLANDO
    exito_al_crear_proceso_en_memoria = recibir_mensaje_de_rta(log_kernel_gral, "CREAR PROCESO", socket_memoria);
    liberar_conexion(log_kernel_gral, "Memoria (en )", socket_memoria);
    return exito_al_crear_proceso_en_memoria;

}

