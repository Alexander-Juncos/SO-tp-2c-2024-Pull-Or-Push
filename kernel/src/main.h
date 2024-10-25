#ifndef MAIN_KERNEL_H_
#define MAIN_KERNEL_H_

#include <utils/general.h>
#include <utils.h>

// ============================================================

/**
* @brief  Setea las variables globales "cod_algoritmo_planif_corto" e "ingresar_a_ready"
*         según el algoritmo de planificación a usar.
* @param algoritmo : El algoritmo de planificación cargado de la config.
*/
void setup_algoritmo_plani_corto_plazo(char* algoritmo);

#endif /* MAIN_KERNEL_H */
