/* CLIENTE: Gestor de Tareas Colaborativas */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* Inclusión de ficheros .h para los sockets */
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MIDA_PAQUET 1024  /* Tamaño del paquete máximo */

int main(int argc, char **argv)
{
    if (argc == 3)  /* Se espera la dirección IP del servidor y el puerto como argumentos */
    {
        int sockfd;  /* El socket */
        struct sockaddr_in servidor_addr;  /* Dirección y puerto del servidor */
        char paquet[MIDA_PAQUET];  /* Para almacenar datos a enviar/recibir */
        int user_id = -1; /* ID de usuario, -1 indica que no está autenticado */

        /* Crear un socket TCP */
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("Error al crear el socket");
            exit(EXIT_FAILURE);
        }

        /* Configuramos la dirección del servidor */
        servidor_addr.sin_family = AF_INET;
        servidor_addr.sin_port = htons(atoi(argv[2]));
        if (inet_pton(AF_INET, argv[1], &servidor_addr.sin_addr) <= 0) {
            perror("Dirección IP no válida");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        /* Conectar al servidor */
        if (connect(sockfd, (struct sockaddr *)&servidor_addr, sizeof(servidor_addr)) < 0) {
            perror("Error al conectar con el servidor");
            close(sockfd);
            exit(EXIT_FAILURE);
        }
        printf("Conectado al servidor %s:%s\n", argv[1], argv[2]);

        int opcion = 0;
        bool autenticado = false;

        /* Menú de Registro / Inicio de Sesión */
        while (!autenticado)
        {
            printf("\n--- Gestor de Tareas Colaborativas ---\n");
            printf("Selecciona una opción:\n");
            printf("1) Registrarse\n");
            printf("2) Iniciar Sesión\n");
            printf("Opción: ");
            if (scanf("%d", &opcion) != 1) {
                printf("Entrada no válida. Inténtalo de nuevo.\n");
                /* Limpiar el buffer de entrada */
                while (getchar() != '\n');
                continue;
            }
            getchar();  /* Consumir el salto de línea que queda en el buffer después de scanf */

            switch(opcion) 
            {
                case 1: /* Registrarse */
                {
                    /* Montamos el comando para enviar al servidor */
                    strcpy(paquet, "REGISTER");

                    /* Enviamos el paquete */
                    if (send(sockfd, paquet, strlen(paquet), 0) < 0) {
                        perror("Error al enviar el paquete");
                        continue;
                    }
                    printf("Paquete enviado! Esperando respuesta...\n");

                    /* Recibimos la respuesta del servidor */
                    ssize_t bytes_recibidos = recv(sockfd, paquet, MIDA_PAQUET - 1, 0);
                    if (bytes_recibidos < 0) {
                        perror("Error al recibir la respuesta del servidor");
                        continue;
                    }
                    paquet[bytes_recibidos] = '\0';  /* Asegurar que el paquete está terminado en null */
                    printf("Respuesta del servidor: %s\n", paquet);

                    /* Procesar respuesta */
                    if (strncmp(paquet, "OK", 2) == 0) {
                        sscanf(paquet, "OK %d", &user_id);
                        printf("Registro exitoso. Tu ID de usuario es: %d\n", user_id);
                        autenticado = true;
                    } else {
                        printf("Error: %s\n", paquet);
                    }
                    break;
                }
                case 2: /* Iniciar sesión */
                {
                    int login_id;
                    printf("Introduce tu ID de usuario: ");
                    if (scanf("%d", &login_id) != 1) {
                        printf("Entrada no válida. Inténtalo de nuevo.\n");
                        /* Limpiar el buffer de entrada */
                        while (getchar() != '\n');
                        continue;
                    }
                    getchar();  /* Consumir el salto de línea */

                    /* Montamos el comando para enviar al servidor */
                    sprintf(paquet, "LOGIN %d", login_id);

                    /* Enviamos el paquete */
                    if (send(sockfd, paquet, strlen(paquet), 0) < 0) {
                        perror("Error al enviar el paquete");
                        continue;
                    }
                    printf("Paquete enviado! Esperando respuesta...\n");

                    /* Recibimos la respuesta del servidor */
                    ssize_t bytes_recibidos = recv(sockfd, paquet, MIDA_PAQUET - 1, 0);
                    if (bytes_recibidos < 0) {
                        perror("Error al recibir la respuesta del servidor");
                        continue;
                    }
                    paquet[bytes_recibidos] = '\0';  /* Asegurar que el paquete está terminado en null */
                    printf("Respuesta del servidor: %s\n", paquet);

                    /* Procesar respuesta */
                    if (strncmp(paquet, "OK", 2) == 0) {
                        user_id = login_id;
                        printf("Inicio de sesión exitoso. Tu ID de usuario es: %d\n", user_id);
                        autenticado = true;
                    } else {
                        printf("Error: %s\n", paquet);
                    }
                    
                    break;
                }

                default:
                    printf("Opción no válida. Inténtalo de nuevo.\n");
                    break;
            }
        }

        /* Menú para seleccionar la operación de gestión de tareas */
        opcion = 0;
        while (opcion != 5)  
        {
            printf("\n--- Gestor de Tareas Colaborativas ---\n");
            printf("Selecciona una opción:\n");
            printf("1) Crear tarea\n");
            printf("2) Completar tarea\n");
            printf("3) Eliminar tarea\n");
            printf("4) Consultar tareas\n");
            printf("5) Salir\n");
            printf("Opción: ");
            if (scanf("%d", &opcion) != 1) {
                printf("Entrada no válida. Inténtalo de nuevo.\n");
                /* Limpiar el buffer de entrada */
                while (getchar() != '\n');
                continue;
            }
            getchar();  /* Consumir el salto de línea que queda en el buffer después de scanf */

            switch(opcion)
            {
                case 1:  /* Crear tarea */
                    {
                        char descripcion[200];
                        printf("Introduce la descripción de la nueva tarea: ");
                        fgets(descripcion, sizeof(descripcion), stdin);
                        descripcion[strcspn(descripcion, "\n")] = 0;  /* Eliminar el salto de línea */

                        /* Montamos el comando para enviar al servidor */
                        sprintf(paquet, "CREATE_TASK %d %s", user_id, descripcion);
                        break;
                    }

                case 2:  /* Completar tarea */
                    {
                        int id;
                        printf("Introduce el ID de la tarea a completar: ");
                        if (scanf("%d", &id) != 1) {
                            printf("Entrada no válida. Inténtalo de nuevo.\n");
                            /* Limpiar el buffer de entrada */
                            while (getchar() != '\n');
                            continue;
                        }
                        getchar();  /* Consumir el salto de línea */

                        /* Montamos el comando para enviar al servidor */
                        sprintf(paquet, "COMPLETE_TASK %d %d", user_id, id);
                        break;
                    }

                case 3:  /* Eliminar tarea */
                    {
                        int id;
                        printf("Introduce el ID de la tarea a eliminar: ");
                        if (scanf("%d", &id) != 1) {
                            printf("Entrada no válida. Inténtalo de nuevo.\n");
                            /* Limpiar el buffer de entrada */
                            while (getchar() != '\n');
                            continue;
                        }
                        getchar();  /* Consumir el salto de línea */

                        /* Montamos el comando para enviar al servidor */
                        sprintf(paquet, "DELETE_TASK %d %d", user_id, id);
                        break;
                    }

                case 4:  /* Consultar tareas */
                    {
                        /* Montamos el comando para consultar la lista de tareas */
                        sprintf(paquet, "LIST_TASKS %d", user_id);
                        break;
                    }

                case 5:  /* Salir */
                    {
                        printf("Saliendo del gestor de tareas.\n");
                        /* Enviar comando de salida al servidor */
                        sprintf(paquet, "EXIT %d", user_id);
                        if (send(sockfd, paquet, strlen(paquet), 0) < 0) {
                            perror("Error al enviar el paquete de salida");
                        }
                        close(sockfd);
                        exit(0);
                    }

                default:
                    {
                        printf("Opción no válida. Inténtalo de nuevo.\n");
                        continue;
                    }
            }

            if (opcion >=1 && opcion <=4) {
                /* Enviamos el paquete */
                if (send(sockfd, paquet, strlen(paquet), 0) < 0) {
                    perror("Error al enviar el paquete");
                    continue;
                }
                printf("Paquete enviado! Esperando respuesta...\n");

                /* Recibimos la respuesta del servidor */
                ssize_t bytes_recibidos = recv(sockfd, paquet, MIDA_PAQUET - 1, 0);
                if (bytes_recibidos < 0) {
                    perror("Error al recibir la respuesta del servidor");
                    continue;
                }
                paquet[bytes_recibidos] = '\0';  /* Asegurar que el paquete está terminado en null */
                printf("Respuesta del servidor:\n%s\n", paquet);
            }
        }

        /* Cerramos el socket */
        close(sockfd);
    }
    else
    {
        printf("El número de parámetros no es el correcto! Uso: %s <dirección IP del servidor> <puerto>\n", argv[0]);
    }

    return 0;
}
