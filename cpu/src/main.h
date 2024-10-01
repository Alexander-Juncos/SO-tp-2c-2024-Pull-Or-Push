#ifndef MAIN_CPU_H_
#define MAIN_CPU_H_

#include "utils.h"
#include <readline/readline.h>

// ==========================================================================
// ====  Funciones :  =======================================================
// ==========================================================================

void* rutina_hilo_interrupcion (void*);
void rutina_main_cpu(void);

#endif /* MAIN_CPU_H_ */