# XD

Desarrollo de la idea: Aplicación de gestión de tareas colaborativas en C usando UDP
La aplicación consistirá en un servidor centralizado que gestionará una lista de tareas compartida, y varios clientes que podrán interactuar con ella en tiempo real a través de mensajes enviados mediante el protocolo UDP.

Cada cliente podrá enviar comandos para:

Crear una tarea.
Modificar el estado de una tarea (por ejemplo, marcar como completada).
Eliminar una tarea.
Consultar el estado de todas las tareas en la lista.
El servidor, que será capaz de manejar múltiples clientes de manera simultánea, mantendrá una lista de tareas y responderá a las solicitudes que reciba de los clientes.

Componentes principales:
Servidor:

Mantiene una lista de tareas compartida.
Procesa las solicitudes enviadas por los clientes.
Envía respuestas o confirmaciones de las operaciones solicitadas.
Clientes:

Envían comandos al servidor para gestionar la lista de tareas (crear, modificar, eliminar, consultar).
Reciben respuestas del servidor.
Detalles de implementación en C:
Servidor UDP:
Gestión de tareas:

Cada tarea tendrá un ID único (generado por el servidor).
La lista de tareas será una estructura que almacene el ID, descripción de la tarea y estado (por hacer o completada).
Estructura sugerida para una tarea:
c
Copiar código
struct Tarea {
    int id;
    char descripcion[256];
    int completada; // 0 = No completada, 1 = Completada
};
Procesamiento de comandos:

El servidor escuchará en un puerto UDP, esperando recibir mensajes de los clientes.
Para cada mensaje recibido, el servidor identificará el tipo de comando (crear, modificar, eliminar, consultar) y procesará la solicitud.
Tras procesar el comando, el servidor enviará una respuesta al cliente confirmando el resultado de la operación.
Manejo de múltiples clientes:

Aunque UDP no es orientado a la conexión, se puede identificar a cada cliente por la dirección IP y el puerto desde el cual envía su solicitud.
Esto permite que múltiples clientes interactúen con el servidor en tiempo real.
Clientes UDP:
Envío de comandos:

El cliente se conectará al servidor usando sockets UDP.
Los comandos se enviarán como mensajes de texto. Por ejemplo:
"1 crear tarea: estudiar"
"2 completar tarea: estudiar"
"3 eliminar tarea: estudiar"
"4 consultar tareas"
El cliente puede construir estos mensajes y enviarlos al servidor.
Recepción de respuestas:

El cliente recibirá una respuesta del servidor tras cada comando. Las respuestas pueden ser mensajes simples como "Tarea creada con éxito" o "Lista de tareas: ...".
Flujo de trabajo de la aplicación:
Cliente envía solicitud al servidor:

Ejemplo: "1 crear tarea: estudiar".
El cliente envía este mensaje al servidor, indicando que quiere crear una tarea llamada "estudiar".
Servidor recibe y procesa la solicitud:

El servidor analiza el mensaje recibido.
Si el comando es para crear una tarea, genera un nuevo ID, agrega la tarea a la lista de tareas y envía una respuesta al cliente confirmando la creación de la tarea.
Cliente recibe la respuesta:

El cliente recibe la respuesta del servidor, que puede ser algo como: "Tarea 'estudiar' creada con ID 1".
Otro cliente consulta la lista de tareas:

Otro cliente podría enviar el comando "4 consultar tareas" al servidor.
El servidor responde con la lista actual de tareas, por ejemplo: "ID: 1, Tarea: estudiar, Estado: no completada".
Estructuras y funciones sugeridas:
Servidor:
c
Copiar código
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#define MAX_TAREAS 100

struct Tarea {
    int id;
    char descripcion[256];
    int completada; // 0 = No completada, 1 = Completada
};

struct Tarea lista_tareas[MAX_TAREAS];
int num_tareas = 0;

void agregar_tarea(char* descripcion) {
    if (num_tareas < MAX_TAREAS) {
        lista_tareas[num_tareas].id = num_tareas + 1;
        strcpy(lista_tareas[num_tareas].descripcion, descripcion);
        lista_tareas[num_tareas].completada = 0;
        num_tareas++;
    }
}

void completar_tarea(int id) {
    for (int i = 0; i < num_tareas; i++) {
        if (lista_tareas[i].id == id) {
            lista_tareas[i].completada = 1;
            break;
        }
    }
}

void eliminar_tarea(int id) {
    for (int i = 0; i < num_tareas; i++) {
        if (lista_tareas[i].id == id) {
            for (int j = i; j < num_tareas - 1; j++) {
                lista_tareas[j] = lista_tareas[j + 1];
            }
            num_tareas--;
            break;
        }
    }
}

void consultar_tareas(char* buffer) {
    strcpy(buffer, "Lista de tareas:\n");
    for (int i = 0; i < num_tareas; i++) {
        char tarea[300];
        sprintf(tarea, "ID: %d, Tarea: %s, Estado: %s\n", lista_tareas[i].id, lista_tareas[i].descripcion,
                lista_tareas[i].completada ? "Completada" : "No completada");
        strcat(buffer, tarea);
    }
}
Cliente:
c
Copiar código
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#define BUF_SIZE 1024

void enviar_comando(char* comando, struct sockaddr_in* server_addr, int sock) {
    sendto(sock, comando, strlen(comando), 0, (struct sockaddr*)server_addr, sizeof(*server_addr));
    
    char buffer[BUF_SIZE];
    recvfrom(sock, buffer, BUF_SIZE, 0, NULL, NULL);
    printf("Respuesta del servidor: %s\n", buffer);
}

int main() {
    int sock;
    struct sockaddr_in server_addr;

    sock = socket(AF_INET, SOCK_DGRAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    // Ejemplo de comando de creación de tarea
    enviar_comando("1 crear tarea: estudiar", &server_addr, sock);
    
    close(sock);
    return 0;
}
Resumen:
Servidor: Mantiene la lista de tareas, procesa solicitudes de clientes y responde con la lista de tareas o confirmaciones.
Clientes: Envían comandos para crear, completar, eliminar o consultar tareas.
UDP: Se utiliza UDP para la comunicación, lo que permite a los clientes interactuar de manera rápida con el servidor, aunque deben manejarse posibles pérdidas de mensajes.