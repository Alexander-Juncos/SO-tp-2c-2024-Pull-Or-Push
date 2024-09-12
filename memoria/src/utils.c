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

    memoria = malloc(sizeof(t_memoria_particionada));

    // distingue tipo de particiones
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

    // distingue algoritmo
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
    
    // cargo particiones (considera fijas-dinamicas)
    memoria->lista_particiones = crear_lista_de_particiones();

    // si no tiene particiones aborta
    if (memoria->lista_particiones == NULL)
    {
        log_error(log_memoria_gral, "Memoria sin particiones, abortando");
        free(memoria);
        return false;
    }

    // reservo espacio usuario
    memoria->espacio_usuario = malloc(memoria->tamano_memoria);

    // inicio lista procesos y su mutex
    procesos_cargados = list_create();
    pthread_mutex_init(&mutex_procesos_cargados, NULL);
    
    log_debug(log_memoria_gral, "Memoria iniciada correctamente");
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

t_list* crear_lista_de_particiones() 
{
    char** array_particiones;
	t_list* lista_particiones = list_create();
	int puntero = 0;
    int i=0;
    t_particion* particion;

    if (memoria->particiones_dinamicas == false)
    {
        array_particiones = config_get_array_value(config, "PARTICIONES");
	    while (array_particiones[i] != NULL) 
        {
            particion = malloc(sizeof(t_particion));
            particion->base = puntero;
            puntero += atoi(array_particiones[i]); //
            particion->limite = puntero - 1; // el -1 para que cumpla el rango ej: 0->256-1 = 256 de tam_particion
            list_add(lista_particiones, particion);
            i++;
	    }
        free(array_particiones);
    }
    else
    { // crea una partición q ocupa toda la memoria
        particion = malloc(sizeof(t_particion));
        particion->base = puntero;
        puntero = memoria->tamano_memoria;
        particion->limite = puntero - 1;
        list_add(lista_particiones, particion);
    }

    // La condicion de paso ayuda a comprobar q todas las particiones sumadas formen toda la memoria... (capaz no es correcto?)
    if (puntero != memoria->tamano_memoria){
        log_debug(log_memoria_gral, "particiones no completan el tamano de memoria, particiones no creadas");
        list_destroy_and_destroy_elements(lista_particiones, free); //ya libera el puntero
        return NULL;
    }
	return lista_particiones;
}

t_list *cargar_instrucciones(char *directorio, int pid, int tid)
{
    FILE *archivo;
    size_t lineaSize = 0; // esto es necesario para que getline() funcione bien
    char *lineaInstruccion = NULL; // esto es necesario para que getline() funcione bien
    int cant_instrucciones_cargadas = 0;
    char *base_dir = config_get_string_value(config, "PATH_INSTRUCCIONES");

    log_debug(log_memoria_gral, "Cargando instrucciones del hilo %d, del proceso %d....", tid, pid);
    
    // Crear una nueva cadena para la ruta completa
    size_t tamano_ruta = strlen(base_dir) + strlen(directorio) + 1;
    char *dir_completa = malloc(tamano_ruta);
    if (dir_completa == NULL) {
        return NULL;
    }
    strcpy(dir_completa, base_dir);
    strcat(dir_completa, directorio);
    
    archivo = fopen(dir_completa, "r");
    free(dir_completa); // ahora podemos liberar la memoria de dir_completa

    if (archivo == NULL) {
        log_error(log_memoria_gral, "No se pudo abrir el archivo de instrucciones");
        return NULL;
    }

    t_list *lista = list_create();
    if (lista == NULL) {
        log_error(log_memoria_gral, "No se pudo crear la lista para cargar las instrucciones leidas");
        fclose(archivo);
        return NULL;
    }

    while (getline(&lineaInstruccion, &lineaSize, archivo) != -1) {
        char *instruccion_copia = strdup(lineaInstruccion); // Copiar la línea leída
        if (instruccion_copia != NULL) {
            string_trim_right(&instruccion_copia);
            list_add(lista, instruccion_copia);
            cant_instrucciones_cargadas++;
        }
    }

    log_debug(log_memoria_gral, "Se cargaron correctamente %d instrucciones para el hilo %d, del proceso %d", cant_instrucciones_cargadas, tid, pid);

    fclose(archivo);
    free(lineaInstruccion);
    return lista;
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