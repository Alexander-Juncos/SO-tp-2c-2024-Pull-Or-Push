#include <utils.h>

// ==========================================================================
// ====  Variables globales:  ===============================================
// ==========================================================================

int socket_cpu_dispatch = 1;
int socket_cpu_interrupt = 1;

t_config *config = NULL;
int quantum_de_config;

t_log* log_kernel_oblig = NULL;
t_log* log_kernel_gral = NULL;