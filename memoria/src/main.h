#ifndef MAIN_MEMORIA_H_
#define MAIN_MEMORIA_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>

#include "utils.h"
#include <servidor_multihilo.h>

// ==========================================================================
// ====  Variables:  ========================================================
// ==========================================================================

// ==========================================================================
// ====  Funcion Hilo main (serv. cpu):  ====================================
// ==========================================================================

void atender_cpu(void);

#endif /* MAIN_MEMORIA_H */