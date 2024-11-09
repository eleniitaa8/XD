/* SERVIDOR: Gestor de Tareas Colaborativas Asíncrono */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <ifaddrs.h>
#include <arpa/inet.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MIDA_PAQUET 1024     /* Tamaño máximo del paquete */
#define MAX_TAREAS 100       /* Número máximo de tareas por usuario */
#define MAX_USUARIOS 100     /* Número máximo de usuarios */

/* Estructura para las tareas */
typedef struct {
    int id;
    char descripcion[200];
    bool completada;
} Tarea;

/* Estructura para los usuarios */
typedef struct {
    int id;                     /* ID del usuario, corresponde a la posición en el array */
    Tarea tareas[MAX_TAREAS];  /* Array de tareas */
    int contador_tareas;       /* Número de tareas activas */
    int contador_ids_tareas;   /* Contador para asignar IDs únicos a las tareas */
    pthread_mutex_t mutex;     /* Mutex para sincronizar operaciones de este usuario */
} Usuario;

/* Lista global de usuarios */
Usuario usuarios[MAX_USUARIOS];
int contador_usuarios = 0;

/* Mutex para proteger la lista de usuarios */
pthread_mutex_t mutex_usuarios = PTHREAD_MUTEX_INITIALIZER;

/* Función para listar las IPs del servidor */
void listar_ips(int puerto) {
    struct ifaddrs *ifaddr, *ifa;
    char ip[INET_ADDRSTRLEN];

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return;
    }

    printf("Servidor escuchando en las siguientes direcciones IP en el puerto %d:\n", puerto);

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;
        if (ifa->ifa_addr->sa_family == AF_INET) {  /* Solo IPv4 */
            struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
            inet_ntop(AF_INET, &(sa->sin_addr), ip, INET_ADDRSTRLEN);
            printf(" - %s:%d (%s)\n", ip, puerto, ifa->ifa_name);
        }
    }

    freeifaddrs(ifaddr);
}

/* Funciones para gestionar tareas de un usuario */

/* Agregar una tarea */
bool agregar_tarea(Usuario *usuario, char *descripcion) {
    for (int i = 0; i < MAX_TAREAS; i++) {
        if (usuario->tareas[i].id == 0) {  /* Posición libre */
            usuario->tareas[i].id = usuario->contador_ids_tareas++;
            strncpy(usuario->tareas[i].descripcion, descripcion, sizeof(usuario->tareas[i].descripcion) - 1);
            usuario->tareas[i].descripcion[sizeof(usuario->tareas[i].descripcion) - 1] = '\0';  /* Asegurar terminación */
            usuario->tareas[i].completada = false;
            usuario->contador_tareas++;
            return true;
        }
    }
    return false;  /* No hay espacio para más tareas */
}

/* Completar una tarea */
bool completar_tarea(Usuario *usuario, int id) {
    for (int i = 0; i < MAX_TAREAS; i++) {
        if (usuario->tareas[i].id == id) {
            usuario->tareas[i].completada = true;
            return true;
        }
    }
    return false;  /* Tarea no encontrada */
}

/* Eliminar una tarea */
bool eliminar_tarea(Usuario *usuario, int id) {
    for (int i = 0; i < MAX_TAREAS; i++) {
        if (usuario->tareas[i].id == id) {
            usuario->tareas[i].id = 0;             /* Marcar como inactiva */
            usuario->tareas[i].descripcion[0] = '\0';
            usuario->tareas[i].completada = false;
            usuario->contador_tareas--;
            return true;
        }
    }
    return false;  /* Tarea no encontrada */
}

/* Consultar tareas */
void consultar_tareas(Usuario *usuario, char *respuesta) {
    strcpy(respuesta, "Tareas:\n");
    for (int i = 0; i < MAX_TAREAS; i++) {
        if (usuario->tareas[i].id != 0) {  /* Solo tareas activas */
            char linea[256];
            sprintf(linea, "ID: %d | Descripción: %s | Completada: %s\n",
                    usuario->tareas[i].id,
                    usuario->tareas[i].descripcion,
                    usuario->tareas[i].completada ? "Sí" : "No");
            strcat(respuesta, linea);
        }
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

        /* Inicializar variables para evitar problemas */
        comando[0] = '\0';
        descripcion[0] = '\0';
        respuesta[0] = '\0';

        /* Extraer el comando principal */
        sscanf(paquet, "%s", comando);

        if (strcmp(comando, "REGISTER") == 0) {
            pthread_mutex_lock(&mutex_usuarios);
            if (contador_usuarios < MAX_USUARIOS) {
                int nuevo_id = contador_usuarios;
                usuarios[nuevo_id].id = nuevo_id;
                usuarios[nuevo_id].contador_tareas = 0;
                usuarios[nuevo_id].contador_ids_tareas = 1;  /* Inicializar contador de IDs de tareas */
                pthread_mutex_init(&usuarios[nuevo_id].mutex, NULL);  /* Inicializar mutex del usuario */
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
            if (id_usuario >= 0 && id_usuario < contador_usuarios) {
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
            /* Extraer el ID de usuario y la descripción de la tarea */
            int items = sscanf(paquet, "CREATE_TASK %d %[^\n]", &id_usuario, descripcion);
            if (items < 2) {
                strcpy(respuesta, "ERROR Formato de CREATE_TASK inválido");
            }
            else {
                pthread_mutex_lock(&mutex_usuarios);
                if (id_usuario >= 0 && id_usuario < contador_usuarios) {
                    Usuario *usuario = &usuarios[id_usuario];
                    pthread_mutex_lock(&usuario->mutex);  /* Bloquear mutex del usuario */
                    bool exito = agregar_tarea(usuario, descripcion);
                    if (exito) {
                        strcpy(respuesta, "Tarea creada exitosamente.");
                        printf("Tarea creada para usuario %d: %s\n", id_usuario, descripcion);
                    }
                    else {
                        strcpy(respuesta, "ERROR No hay espacio para más tareas.");
                        printf("Error al crear tarea: No hay espacio para más tareas para el usuario %d.\n", id_usuario);
                    }
                    pthread_mutex_unlock(&usuario->mutex);  /* Desbloquear mutex del usuario */
                } else {
                    strcpy(respuesta, "ERROR ID de usuario inválido");
                    printf("Error al crear tarea: ID de usuario %d inválido.\n", id_usuario);
                }
                pthread_mutex_unlock(&mutex_usuarios);
            }
            /* Enviar respuesta */
            send(client_sock, respuesta, strlen(respuesta), 0);
        }
        else if (strcmp(comando, "COMPLETE_TASK") == 0) {
            /* Extraer el ID de usuario y el ID de la tarea */
            int items = sscanf(paquet, "COMPLETE_TASK %d %d", &id_usuario, &id_tarea);
            if (items < 2) {
                strcpy(respuesta, "ERROR Formato de COMPLETE_TASK inválido");
            }
            else {
                pthread_mutex_lock(&mutex_usuarios);
                if (id_usuario >= 0 && id_usuario < contador_usuarios) {
                    Usuario *usuario = &usuarios[id_usuario];
                    pthread_mutex_lock(&usuario->mutex);  /* Bloquear mutex del usuario */
                    bool exito = completar_tarea(usuario, id_tarea);
                    if (exito) {
                        sprintf(respuesta, "Tarea con ID %d completada exitosamente.", id_tarea);
                        printf("Tarea con ID %d completada para usuario %d.\n", id_tarea, id_usuario);
                    }
                    else {
                        strcpy(respuesta, "ERROR Tarea no encontrada.");
                        printf("Error al completar tarea: Tarea con ID %d no encontrada para usuario %d.\n", id_tarea, id_usuario);
                    }
                    pthread_mutex_unlock(&usuario->mutex);  /* Desbloquear mutex del usuario */
                } else {
                    strcpy(respuesta, "ERROR ID de usuario inválido");
                    printf("Error al completar tarea: ID de usuario %d inválido.\n", id_usuario);
                }
                pthread_mutex_unlock(&mutex_usuarios);
            }
            /* Enviar respuesta */
            send(client_sock, respuesta, strlen(respuesta), 0);
        }
        else if (strcmp(comando, "DELETE_TASK") == 0) {
            /* Extraer el ID de usuario y el ID de la tarea */
            int items = sscanf(paquet, "DELETE_TASK %d %d", &id_usuario, &id_tarea);
            if (items < 2) {
                strcpy(respuesta, "ERROR Formato de DELETE_TASK inválido");
            }
            else {
                pthread_mutex_lock(&mutex_usuarios);
                if (id_usuario >= 0 && id_usuario < contador_usuarios) {
                    Usuario *usuario = &usuarios[id_usuario];
                    pthread_mutex_lock(&usuario->mutex);  /* Bloquear mutex del usuario */
                    bool exito = eliminar_tarea(usuario, id_tarea);
                    if (exito) {
                        sprintf(respuesta, "Tarea con ID %d eliminada exitosamente.", id_tarea);
                        printf("Tarea con ID %d eliminada para usuario %d.\n", id_tarea, id_usuario);
                    }
                    else {
                        strcpy(respuesta, "ERROR Tarea no encontrada.");
                        printf("Error al eliminar tarea: Tarea con ID %d no encontrada para usuario %d.\n", id_tarea, id_usuario);
                    }
                    pthread_mutex_unlock(&usuario->mutex);  /* Desbloquear mutex del usuario */
                } else {
                    strcpy(respuesta, "ERROR ID de usuario inválido");
                    printf("Error al eliminar tarea: ID de usuario %d inválido.\n", id_usuario);
                }
                pthread_mutex_unlock(&mutex_usuarios);
            }
            /* Enviar respuesta */
            send(client_sock, respuesta, strlen(respuesta), 0);
        }
        else if (strcmp(comando, "LIST_TASKS") == 0) {
            /* Extraer el ID de usuario */
            int items = sscanf(paquet, "LIST_TASKS %d", &id_usuario);
            if (items < 1) {
                strcpy(respuesta, "ERROR Formato de LIST_TASKS inválido");
            }
            else {
                pthread_mutex_lock(&mutex_usuarios);
                if (id_usuario >= 0 && id_usuario < contador_usuarios) {
                    Usuario *usuario = &usuarios[id_usuario];
                    pthread_mutex_lock(&usuario->mutex);  /* Bloquear mutex del usuario */
                    consultar_tareas(usuario, respuesta);
                    printf("Listado de tareas enviado para usuario %d.\n", id_usuario);
                    pthread_mutex_unlock(&usuario->mutex);  /* Desbloquear mutex del usuario */
                } else {
                    strcpy(respuesta, "ERROR ID de usuario inválido");
                    printf("Error al listar tareas: ID de usuario %d inválido.\n", id_usuario);
                }
                pthread_mutex_unlock(&mutex_usuarios);
            }
            /* Enviar respuesta */
            send(client_sock, respuesta, strlen(respuesta), 0);
        }
        else if (strcmp(comando, "EXIT") == 0) {
            /* Extraer el ID de usuario */
            int items = sscanf(paquet, "EXIT %d", &id_usuario);
            if (items < 1) {
                strcpy(respuesta, "ERROR Formato de EXIT inválido");
            }
            else {
                sprintf(respuesta, "Desconectando usuario %d.\n", id_usuario);
                send(client_sock, respuesta, strlen(respuesta), 0);
                printf("Usuario %d ha cerrado la conexión.\n", id_usuario);
                conectado = false;
            }
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
        int puerto = atoi(argv[1]);

        /* Crear un socket TCP */
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("Error al crear el socket");
            exit(EXIT_FAILURE);
        }

        /* Configuramos los datos del socket del servidor */
        servidor_addr.sin_family = AF_INET;
        servidor_addr.sin_addr.s_addr = INADDR_ANY;  /* Cualquier NIC */
        servidor_addr.sin_port = htons(puerto);      /* Puerto donde estará escuchando el servidor */

        /* Enlazamos el socket */
        if (bind(sockfd, (struct sockaddr *)&servidor_addr, sizeof(servidor_addr)) < 0) {
            perror("No se ha podido enlazar el socket");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        /* Listar las IPs del servidor */
        listar_ips(puerto);

        /* Escuchar conexiones entrantes */
        if (listen(sockfd, 5) < 0) {
            perror("Error al escuchar conexiones");
            close(sockfd);
            exit(EXIT_FAILURE);
        }
        printf("Servidor operativo en el puerto %d!\n", puerto);

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
