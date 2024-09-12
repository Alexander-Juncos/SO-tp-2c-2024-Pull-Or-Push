#include "utils.h"

// ==========================================================================
// ====  Variables globales:  ===============================================
// ==========================================================================

int socket_cpu = 1;
int socket_escucha = 1;

t_config *config; 
t_log *log_memoria_oblig; 
t_log *log_memoria_gral; 

bool fin_programa = 0;

t_list* procesos_cargados; // sus elementos van a ser de tipo t_pcb_mem
pthread_mutex_t mutex_procesos_cargados;

t_memoria_particionada* memoria;

// ==========================================================================
// ====  Funciones Internas:  ===============================================
// ==========================================================================

bool iniciar_memoria()
{
    char* str_auxiliar;
    char** particiones_temp;

    memoria = malloc(sizeof(t_memoria_particionada));

    str_auxiliar = config_get_string_value(config, "ESQUEMA");
    if (strcmp(str_auxiliar, "FIJAS") == 0){
        memoria->particiones_dinamicas = false;
    } else if (strcmp(str_auxiliar, "DINAMICAS")){
        memoria->particiones_dinamicas = true;
    } else {
        log_error(log_memoria_gral, "Error obtener el esquema de particiones");
        return false;
    }
    free(str_auxiliar);

    str_auxiliar = config_get_string_value(config, "ALGORITMO_BUSQUEDA");
    if (strcmp(str_auxiliar, "FIRST") == 0){
        memoria->algorit_busq = FIRST_FIT;
    } else if (strcmp(str_auxiliar, "BEST")){
        memoria->algorit_busq = BEST_FIT;
    } else if (strcmp(str_auxiliar, "WORST")){
        memoria->algorit_busq = WORST_FIT; 
    } else {
        log_error(log_memoria_gral, "Error al obtener el algoritmo busqueda");
        return false;
    }
    free(str_auxiliar);

    memoria->tamano_memoria = config_get_int_value(config, "TAM_MEMORIA");
    retardo_respuesta = config_get_int_value(config, "RETARDO_RESPUESTA");
    path_instrucciones = config_get_string_value(config, "PATH_INSTRUCCIONES");

    if (memoria->particiones_dinamicas == false){
        particiones_temp = config_get_array_value(config, "PARTICIONES");
        /*
            convertilo en algo util (while particiones_temp[i]/=NULL, yo crearia una 
            funcion para hacerlo)
        */
        free(particiones_temp);
    } 

    /*
        inicar espacio usuario y lo q requiera
    */

    // inicio lista procesos y su mutex
    procesos_cargados = list_create();
    pthread_mutex_init(&mutex_procesos_cargados, NULL);
    
    return true;
}

// ==========================================================================
// ====  Funciones Auxiliares:  =============================================
// ==========================================================================

void retardo_operacion()
{
    unsigned int tiempo_en_microsegs = config_get_int_value(config, "RETARDO_RESPUESTA")*MILISEG_A_MICROSEG;
    usleep(tiempo_en_microsegs);
}

void iniciar_logs(bool testeo)
{
    log_memoria_gral = log_create("memoria_general.log", "Memoria", testeo, LOG_LEVEL_DEBUG);
    
    // Log obligatorio
    char * nivel;
    nivel = config_get_string_value(config, "LOG_LEVEL");
    log_memoria_oblig = log_create("memoria_obligatorio.log", "Memoria", true, log_level_from_string(nivel));

    /*
        Ver luego si se quiere manejar caso de que el config este mal () y como cerrar el programa.
    */

    free(nivel);		
}

void terminar_programa()
{
	// Y por ultimo, hay que liberar lo que utilizamos (conexion, log y config) 
	 // con las funciones de las commons y del TP mencionadas en el enunciado /
	liberar_conexion(log_memoria_gral, "CPU", socket_cpu);
	liberar_conexion(log_memoria_gral, "Mi propio Servidor Escucha", socket_escucha);
	log_destroy(log_memoria_oblig);
	log_destroy(log_memoria_gral);
	config_destroy(config);
}