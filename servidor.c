/* SERVIDOR: Gestor de tareas colaborativas */

/* Inclusión de ficheros .h habituales */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Inclusión de ficheros .h para los sockets */
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MIDA_PAQUET 256  /* Tamaño máximo del paquete */
#define MAX_TAREAS 100    /* Número máximo de tareas que se pueden gestionar */

/* Estructura para las tareas */
typedef struct {
    int id;
    char descripcion[200];
    int completada;
} Tarea;

/* Lista global de tareas */
Tarea tareas[MAX_TAREAS];
int contador_tareas = 0;

void agregar_tarea(char *descripcion) {
    if (contador_tareas < MAX_TAREAS) {
        tareas[contador_tareas].id = contador_tareas + 1;
        strcpy(tareas[contador_tareas].descripcion, descripcion);
        tareas[contador_tareas].completada = 0;
        contador_tareas++;
    }
}

void completar_tarea(int id) {
    for (int i = 0; i < contador_tareas; i++) {
        if (tareas[i].id == id) {
            tareas[i].completada = 1;
        }
    }
}

void eliminar_tarea(int id) {
    for (int i = 0; i < contador_tareas; i++) {
        if (tareas[i].id == id) {
            /* Desplazar las tareas hacia arriba para eliminar el hueco */
            for (int j = i; j < contador_tareas - 1; j++) {
                tareas[j] = tareas[j + 1];
            }
            contador_tareas--;
            break;
        }
    }
}

void consultar_tareas(char *respuesta) {
    strcpy(respuesta, "Tareas:\n");
    for (int i = 0; i < contador_tareas; i++) {
        char linea[256];
        sprintf(linea, "ID: %d | Descripción: %s | Completada: %s\n",
                tareas[i].id, tareas[i].descripcion, tareas[i].completada ? "Sí" : "No");
        strcat(respuesta, linea);
    }
}

int main(int argc, char **argv) {
    if (argc == 2) {
        int s;  /* El socket */
        struct sockaddr_in socket_servidor;  /* Datos del socket donde escucha el servidor */
        struct sockaddr_in contacte_client;  /* Dirección y puerto desde donde el cliente envía el paquete */
        socklen_t contacte_client_mida = sizeof(contacte_client);  /* Longitud de la dirección del cliente */

        char paquet[MIDA_PAQUET];  /* Buffer para los datos enviados/recibidos */

        /* Queremos un socket de internet no orientado a la conexión */
        s = socket(AF_INET, SOCK_DGRAM, 0);

        /* Configuramos los datos del socket del servidor */
        socket_servidor.sin_family = AF_INET;  /* Socket en Internet */
        socket_servidor.sin_addr.s_addr = INADDR_ANY;  /* Cualquier NIC */
        socket_servidor.sin_port = htons(atoi(argv[1]));  /* Puerto donde estará escuchando el servidor */

        /* Enlazamos el socket */
        if (bind(s, (struct sockaddr *)&socket_servidor, sizeof(socket_servidor)) < 0) {
            printf("No se ha podido enlazar el socket\n");
            return 1;
        } else {
            printf("Servidor operativo en el puerto %d!\n", atoi(argv[1]));

            while (1) {
                printf("Esperando petición de algún cliente...\n");
                
                /* Esperamos recibir un paquete */
                recvfrom(s, paquet, MIDA_PAQUET, 0, (struct sockaddr *)&contacte_client, &contacte_client_mida);
                printf("Paquete recibido!\n");

                /* Procesamos el comando recibido */
                char comando[20], descripcion[200];
                int id;
                sscanf(paquet, "%s", comando);

                if (strcmp(comando, "1") == 0) {
                    /* Crear tarea */
                    sscanf(paquet, "1 crear tarea: %[^\n]", descripcion);
                    agregar_tarea(descripcion);
                    sprintf(paquet, "Tarea creada: %s\n", descripcion);
                } else if (strcmp(comando, "2") == 0) {
                    /* Completar tarea */
                    sscanf(paquet, "2 completar tarea: %d", &id);
                    completar_tarea(id);
                    sprintf(paquet, "Tarea con ID %d completada\n", id);
                } else if (strcmp(comando, "3") == 0) {
                    /* Eliminar tarea */
                    sscanf(paquet, "3 eliminar tarea: %d", &id);
                    eliminar_tarea(id);
                    sprintf(paquet, "Tarea con ID %d eliminada\n", id);
                } else if (strcmp(comando, "4") == 0) {
                    /* Consultar tareas */
                    consultar_tareas(paquet);
                } else {
                    sprintf(paquet, "Comando no reconocido\n");
                }

                /* Enviamos la respuesta al cliente */
                sendto(s, paquet, MIDA_PAQUET, 0, (struct sockaddr *)&contacte_client, contacte_client_mida);
                printf("Respuesta enviada al cliente.\n");
            }
        }

        /* Cerramos el socket */
        close(s);
    } else {
        printf("El número de parámetros no es el correcto!\n");
    }
    return 0;
}
