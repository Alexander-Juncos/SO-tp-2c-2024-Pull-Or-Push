#ifndef MAIN_FS_H_
#define MAIN_FS_H_

// revisar sino sobran
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>

#include "utils.h"

// ==========================================================================
// ====  Funciones Servidor:  ===============================================
// ==========================================================================

void rutina_recepcion (void);
void* rutina_ejecucion (void*);

#endif /* MAIN_FS_H_ */