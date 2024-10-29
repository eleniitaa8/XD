/* SERVIDOR: Gestor de tareas colaborativas */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MIDA_PAQUET 1024  /* Tamaño máximo del paquete */
#define MAX_TAREAS 100     /* Número máximo de tareas por usuario */
#define MAX_USUARIOS 100   /* Número máximo de usuarios */

/* Estructura para las tareas */
typedef struct {
    int id;
    char descripcion[200];
    bool completada;
} Tarea;

/* Estructura para los usuarios */
typedef struct {
    int id;  /* ID del usuario, corresponde a la posición en el array */
    Tarea tareas[MAX_TAREAS];
    int contador_tareas;
} Usuario;

/* Lista global de usuarios */
Usuario usuarios[MAX_USUARIOS];
int contador_usuarios = 0;

/* Mutex para proteger la lista de usuarios */
pthread_mutex_t mutex_usuarios = PTHREAD_MUTEX_INITIALIZER;

/* Funciones para gestionar tareas de un usuario */
void agregar_tarea(Usuario *usuario, char *descripcion) {
    if (usuario->contador_tareas < MAX_TAREAS) {
        usuario->tareas[usuario->contador_tareas].id = usuario->contador_tareas + 1;
        strcpy(usuario->tareas[usuario->contador_tareas].descripcion, descripcion);
        usuario->tareas[usuario->contador_tareas].completada = false;
        usuario->contador_tareas++;
    }
}

void completar_tarea(Usuario *usuario, int id) {
    for (int i = 0; i < usuario->contador_tareas; i++) {
        if (usuario->tareas[i].id == id) {
            usuario->tareas[i].completada = true;
            break;
        }
    }
}

void eliminar_tarea(Usuario *usuario, int id) {
    for (int i = 0; i < usuario->contador_tareas; i++) {
        if (usuario->tareas[i].id == id) {
            /* Desplazar las tareas hacia arriba para eliminar el hueco */
            for (int j = i; j < usuario->contador_tareas - 1; j++) {
                usuario->tareas[j] = usuario->tareas[j + 1];
            }
            usuario->contador_tareas--;
            break;
        }
    }
}

void consultar_tareas(Usuario *usuario, char *respuesta) {
    strcpy(respuesta, "Tareas:\n");
    for (int i = 0; i < usuario->contador_tareas; i++) {
        char linea[256];
        sprintf(linea, "ID: %d | Descripción: %s | Completada: %s\n",
                usuario->tareas[i].id, usuarios[i].tareas[i].descripcion, 
                usuarios[i].tareas[i].completada ? "Sí" : "No");
        strcat(respuesta, linea);
    }
}

/* Función para manejar cada cliente */
void *manejar_cliente(void *arg) {
    int client_sock = *(int *)arg;
    free(arg);
    char paquet[MIDA_PAQUET];
    ssize_t bytes_recibidos;
    bool conectado = true;

    while (conectado && (bytes_recibidos = recv(client_sock, paquet, MIDA_PAQUET - 1, 0)) > 0) {
        paquet[bytes_recibidos] = '\0';  /* Asegurar que el paquete está terminado en null */
        printf("Paquete recibido: %s\n", paquet);

        /* Procesar el comando recibido */
        char comando[20];
        int id_usuario, id_tarea;
        char descripcion[200];
        char respuesta[MIDA_PAQUET];

        sscanf(paquet, "%s", comando);

        if (strcmp(comando, "REGISTER") == 0) {
            pthread_mutex_lock(&mutex_usuarios);
            if (contador_usuarios < MAX_USUARIOS) {
                int nuevo_id = contador_usuarios;
                usuarios[nuevo_id].id = nuevo_id;
                usuarios[nuevo_id].contador_tareas = 0;
                contador_usuarios++;
                /* Enviar OK y el ID al cliente */
                sprintf(respuesta, "OK %d", nuevo_id);
                send(client_sock, respuesta, strlen(respuesta), 0);
                printf("Usuario registrado con ID: %d\n", nuevo_id);
            } else {
                /* Enviar error */
                strcpy(respuesta, "ERROR Límite de usuarios alcanzado");
                send(client_sock, respuesta, strlen(respuesta), 0);
                printf("Registro fallido: límite de usuarios alcanzado.\n");
            }
            pthread_mutex_unlock(&mutex_usuarios);
        }
        else if (strcmp(comando, "LOGIN") == 0) {
            sscanf(paquet, "LOGIN %d", &id_usuario);
            pthread_mutex_lock(&mutex_usuarios);
            if (id_usuario >=0 && id_usuario < contador_usuarios) {
                /* Enviar OK */
                strcpy(respuesta, "OK");
                send(client_sock, respuesta, strlen(respuesta), 0);
                printf("Usuario con ID %d ha iniciado sesión.\n", id_usuario);
            } else {
                /* Enviar error */
                strcpy(respuesta, "ERROR ID de usuario inválido");
                send(client_sock, respuesta, strlen(respuesta), 0);
                printf("Inicio de sesión fallido para ID %d.\n", id_usuario);
            }
            pthread_mutex_unlock(&mutex_usuarios);
        }
        else if (strcmp(comando, "CREATE_TASK") == 0) {
            sscanf(paquet, "CREATE_TASK %d %[^\n]", &id_usuario, descripcion);
            pthread_mutex_lock(&mutex_usuarios);
            if (id_usuario >=0 && id_usuario < contador_usuarios) {
                agregar_tarea(&usuarios[id_usuario], descripcion);
                strcpy(respuesta, "Tarea creada exitosamente.");
                printf("Tarea creada para usuario %d: %s\n", id_usuario, descripcion);
            } else {
                strcpy(respuesta, "ERROR ID de usuario inválido");
                printf("Error al crear tarea: ID de usuario %d inválido.\n", id_usuario);
            }
            pthread_mutex_unlock(&mutex_usuarios);
            /* Enviar respuesta */
            send(client_sock, respuesta, strlen(respuesta), 0);
        }
        else if (strcmp(comando, "COMPLETE_TASK") == 0) {
            sscanf(paquet, "COMPLETE_TASK %d %d", &id_usuario, &id_tarea);
            pthread_mutex_lock(&mutex_usuarios);
            if (id_usuario >=0 && id_usuario < contador_usuarios) {
                completar_tarea(&usuarios[id_usuario], id_tarea);
                sprintf(respuesta, "Tarea con ID %d completada exitosamente.", id_tarea);
                printf("Tarea con ID %d completada para usuario %d.\n", id_tarea, id_usuario);
            } else {
                strcpy(respuesta, "ERROR ID de usuario inválido");
                printf("Error al completar tarea: ID de usuario %d inválido.\n", id_usuario);
            }
            pthread_mutex_unlock(&mutex_usuarios);
            /* Enviar respuesta */
            send(client_sock, respuesta, strlen(respuesta), 0);
        }
        else if (strcmp(comando, "DELETE_TASK") == 0) {
            sscanf(paquet, "DELETE_TASK %d %d", &id_usuario, &id_tarea);
            pthread_mutex_lock(&mutex_usuarios);
            if (id_usuario >=0 && id_usuario < contador_usuarios) {
                eliminar_tarea(&usuarios[id_usuario], id_tarea);
                sprintf(respuesta, "Tarea con ID %d eliminada exitosamente.", id_tarea);
                printf("Tarea con ID %d eliminada para usuario %d.\n", id_tarea, id_usuario);
            } else {
                strcpy(respuesta, "ERROR ID de usuario inválido");
                printf("Error al eliminar tarea: ID de usuario %d inválido.\n", id_usuario);
            }
            pthread_mutex_unlock(&mutex_usuarios);
            /* Enviar respuesta */
            send(client_sock, respuesta, strlen(respuesta), 0);
        }
        else if (strcmp(comando, "LIST_TASKS") == 0) {
            sscanf(paquet, "LIST_TASKS %d", &id_usuario);
            pthread_mutex_lock(&mutex_usuarios);
            if (id_usuario >=0 && id_usuario < contador_usuarios) {
                consultar_tareas(&usuarios[id_usuario], respuesta);
                printf("Listado de tareas enviado para usuario %d.\n", id_usuario);
            } else {
                strcpy(respuesta, "ERROR ID de usuario inválido");
                printf("Error al listar tareas: ID de usuario %d inválido.\n", id_usuario);
            }
            pthread_mutex_unlock(&mutex_usuarios);
            /* Enviar respuesta */
            send(client_sock, respuesta, strlen(respuesta), 0);
        }
        else if (strcmp(comando, "EXIT") == 0) {
            sscanf(paquet, "EXIT %d", &id_usuario);
            sprintf(respuesta, "Desconectando usuario %d.\n", id_usuario);
            send(client_sock, respuesta, strlen(respuesta), 0);
            printf("Usuario %d ha cerrado la conexión.\n", id_usuario);
            conectado = false;
        }
        else {
            strcpy(respuesta, "ERROR Comando no reconocido");
            send(client_sock, respuesta, strlen(respuesta), 0);
            printf("Comando no reconocido: %s\n", comando);
        }
    }

    if (bytes_recibidos == 0) {
        printf("Cliente desconectado.\n");
    } else if (bytes_recibidos < 0) {
        perror("Error al recibir datos");
    }

    close(client_sock);
    pthread_exit(NULL);
}

int main(int argc, char **argv) {
    if (argc == 2) {
        int sockfd, *new_sock;
        struct sockaddr_in servidor_addr, cliente_addr;
        socklen_t cliente_addr_len = sizeof(cliente_addr);
        pthread_t tid;

        /* Crear un socket TCP */
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("Error al crear el socket");
            exit(EXIT_FAILURE);
        }

        /* Configuramos los datos del socket del servidor */
        servidor_addr.sin_family = AF_INET;
        servidor_addr.sin_addr.s_addr = INADDR_ANY;  /* Cualquier NIC */
        servidor_addr.sin_port = htons(atoi(argv[1]));  /* Puerto donde estará escuchando el servidor */

        /* Enlazamos el socket */
        if (bind(sockfd, (struct sockaddr *)&servidor_addr, sizeof(servidor_addr)) < 0) {
            perror("No se ha podido enlazar el socket");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        /* Escuchar conexiones entrantes */
        if (listen(sockfd, 5) < 0) {
            perror("Error al escuchar conexiones");
            close(sockfd);
            exit(EXIT_FAILURE);
        }
        printf("Servidor operativo en el puerto %d!\n", atoi(argv[1]));

        /* Aceptar conexiones entrantes */
        while (1) {
            int client_sock = accept(sockfd, (struct sockaddr *)&cliente_addr, &cliente_addr_len);
            if (client_sock < 0) {
                perror("Error al aceptar la conexión");
                continue;
            }
            printf("Conexión aceptada de %s:%d\n", inet_ntoa(cliente_addr.sin_addr), ntohs(cliente_addr.sin_port));

            /* Crear un hilo para manejar al cliente */
            new_sock = malloc(sizeof(int));
            if (new_sock == NULL) {
                perror("Error al asignar memoria para el socket del cliente");
                close(client_sock);
                continue;
            }
            *new_sock = client_sock;
            if (pthread_create(&tid, NULL, manejar_cliente, (void *)new_sock) != 0) {
                perror("Error al crear el hilo");
                free(new_sock);
                close(client_sock);
                continue;
            }

            /* Desacoplar el hilo para que se liberen recursos al terminar */
            pthread_detach(tid);
        }

        /* Cerramos el socket del servidor */
        close(sockfd);
    }
    else {
        printf("El número de parámetros no es el correcto! Uso: %s <puerto>\n", argv[0]);
    }
    return 0;
}
