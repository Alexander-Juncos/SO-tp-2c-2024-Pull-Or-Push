#ifndef UTILS_CONEXIONES_H_
#define UTILS_CONEXIONES_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdint.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <assert.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <utils/general.h>

// ==========================================================================
// ====  Estructuras protocolos: ============================================
// ==========================================================================

typedef enum 
{
    MENSAJE_ERROR,
	MENSAJE,
    /*
        a desarrollar 
    */
    HANDSHAKE
} op_code;

typedef enum
{   
    // Módulo cliente

    KERNEL,
    KERNEL_D,
    KERNEL_I,
    CPU,
    MEMORIA,
    FS,

    // Módulo Servidor

    HANDSHAKE_OK,
    HANDSHAKE_INVALIDO
} handshake_code; 

typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef struct
{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;

// ==========================================================================
// ====  Funciones conexión:  ===============================================
// ==========================================================================

int crear_conexion(char* ip, char* puerto);
int iniciar_servidor(char* puerto);
int esperar_cliente(int socket);
void liberar_conexion(t_log* log, char* nombre_conexion, int socket); // revisar si es necesaria

// ==========================================================================
// ====  Funciones handshake: ===============================================
// ==========================================================================

bool recibir_y_manejar_rta_handshake(t_log* logger, const char* nombre_servidor, int socket);
void enviar_handshake(handshake_code handshake_codigo, int socket);
int recibir_handshake(int socket);

// ==========================================================================
// ====  Funciones comunicación: ============================================
// ==========================================================================

bool recibir_y_manejar_rta_handshake(t_log* logger, const char* nombre_servidor, int socket);
void enviar_handshake(handshake_code handshake_codigo, int socket);
int recibir_handshake(int socket);

/// @brief Recibe un código de operación y el paquete que lo acompania, que debería estar vacío. Osea que solo debería tener el código de operación.
/// @param logger           : Instancia de log Para loguear lo recibido.
/// @param cod_esperado     : El código que se espera recibir.
/// @param nombre_conexion  : Nombre de quien se está recibiendo. Para loguear.
/// @param socket           : Socket de la conexión.
/// @return                 : Retorna true si se recibió la respuesta esperada, y false en cualquier otro caso.
bool recibir_y_verificar_cod_respuesta_empaquetado(t_log* logger, op_code cod_esperado, char* nombre_conexion, int socket);

void* recibir_buffer(int*, int);

/// @brief Recibe solamente un código de operación.
/// @param socket           : Socket de la conexión.
int recibir_codigo(int socket);
void recibir_mensaje(int);
// Recibe un paquete con valores cuyo tamanio va obteniendo uno a uno, y retorna una t_list* de los mismos.
t_list* recibir_paquete(int socket);
void* serializar_paquete(t_paquete* paquete, int bytes);
void enviar_mensaje(char* mensaje, int socket_cliente);
void crear_buffer(t_paquete* paquete);
t_paquete* crear_paquete(int cod_op);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);

// Igual que agregar_a_paquete(), pero para datos estáticos (cuyo tamanio es fijo).
// Por lo que no agrega el tamanio del mismo al stream.
// Si se usa esta función al armar el paquete, no se puede usar recibir_paquete() para recibirla.
void agregar_estatico_a_paquete(t_paquete* paquete, void* valor, int tamanio); 
// la agregue x si alguien la requiere pero me parece mejor usar simplemente agregar_a_paquete (xq coincide con recibir_paquete)

int enviar_paquete(t_paquete* paquete, int socket);
void eliminar_paquete(t_paquete* paquete);

#endif /* UTILS_CONEXIONES_H_ */