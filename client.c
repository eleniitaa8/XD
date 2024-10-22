/* CLIENT: Gestor de tareas colaborativas */

/* Inclusión de ficheros .h habituales */
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

#define MIDA_PAQUET 256  /* Tamaño del paquete máximo */

int main(int argc, char **argv)
{
    if (argc == 3)  /* Se espera la dirección IP del servidor y el puerto como argumentos */
    {
        int s;  /* El socket */
        struct sockaddr_in contacte_servidor;  /* Dirección y puerto del servidor */
        char paquet[MIDA_PAQUET];  /* Para almacenar datos a enviar/recibir */
        
        /* Queremos un socket de internet no orientado a la conexión */
        s = socket(AF_INET, SOCK_DGRAM, 0);

        /* Configuramos la dirección del servidor */
        contacte_servidor.sin_family = AF_INET;  /* Socket en Internet */
        contacte_servidor.sin_addr.s_addr = inet_addr(argv[1]);  /* Dirección IP del servidor */
        contacte_servidor.sin_port = htons(atoi(argv[2]));  /* Puerto en el que escucha el servidor */

        int opcion = 0;

        bool registro = 1;
        while (registro!=0)
        {
            printf("\n--- Gestor de Tareas Colaborativas ---\n");
            printf("Selecciona una opción:\n");
            printf("1) Registrarse\n");
            printf("2) Iniciar Sesión\n");
            fscanf("%d", &opcion);
            switch(opcion) 
            {
                case 1: /* Registrarse */
                {
                    char nombre[200];
                    
                    printf("Nombre de usuario: ");
                    fgets(nombre, sizeof(nombre), stdin);
                    // comprobar que no exista
                    nombre[strcspn(nombre, "\n")] = 0;  /* Eliminar el salto de línea */
                    
                    /* Montamos el comando para enviar al servidor */
                    sprintf(paquet, "%s", nombre);
            
                    sendto(s, paquet, MIDA_PAQUET, 0, (struct sockaddr *)&contacte_servidor, sizeof(contacte_servidor));
                    printf("Paquete enviado! Esperando respuesta...\n");

                    /* Recibimos la respuesta del servidor */
                    recvfrom(s, paquet, MIDA_PAQUET, 0, NULL, NULL);  /* NULL -> No es necesario saber desde dónde llega */
                    printf("Respuesta del servidor: %s\n", paquet);

                    if (strcmp(paquet, "OK") == 0) {
                        printf("Registro exitoso.\n");
                        registro = 0;  /* Salir del bucle de registro */
                    } else {
                        printf("Error: %s\n", paquet);
                    }
                    break;
                }
                case 2: /* Iniciar sesión */
                {
                    char nombre[200];
                    printf("nombre de usuario\n");
                    fgets(nombre, sizeof(nombre), stdin);
                    // comprobar que no exista
                    nombre[strcspn(nombre, "\n")] = 0;  /* Eliminar el salto de línea */
                    
                    /* Montamos el comando para enviar al servidor */
                    sprintf(paquet, "%s", nombre);
            
                    sendto(s, paquet, MIDA_PAQUET, 0, (struct sockaddr *)&contacte_servidor, sizeof(contacte_servidor));
                    printf("Paquete enviado! Esperando respuesta...\n");

                    /* Recibimos la respuesta del servidor */
                    recvfrom(s, paquet, MIDA_PAQUET, 0, NULL, NULL);  /* NULL -> No es necesario saber desde dónde llega */
                    printf("Respuesta del servidor: %s\n", paquet);

                    if (strcmp(paquet, "OK") == 0) {
                        printf("Registro exitoso.\n");
                        registro = 0;  /* Salir del bucle de registro */
                    } else {
                        printf("Error: %s\n", paquet);
                    }
                    
                    break;
                }

            }
        }



        opcion = 0;
        while (opcion != 5)  /* Menú para seleccionar la operación de gestión de tareas */
        {
         
            

            printf("\n--- Gestor de Tareas Colaborativas ---\n");
            printf("Selecciona una opción:\n");
            printf("1) Crear tarea\n");
            printf("2) Completar tarea\n");
            printf("3) Eliminar tarea\n");
            printf("4) Consultar tareas\n");
            printf("5) Salir\n");
            printf("Opción: ");
            scanf("%d", &opcion);
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
                        sprintf(paquet, "1 crear tarea: %s", descripcion);
                    }
                    break;

                case 2:  /* Completar tarea */
                    {
                        int id;
                        printf("Introduce el ID de la tarea a completar: ");
                        scanf("%d", &id);

                        /* Montamos el comando para enviar al servidor */
                        sprintf(paquet, "2 completar tarea: %d", id);
                    }
                    break;

                case 3:  /* Eliminar tarea */
                    {
                        int id;
                        printf("Introduce el ID de la tarea a eliminar: ");
                        scanf("%d", &id);

                        /* Montamos el comando para enviar al servidor */
                        sprintf(paquet, "3 eliminar tarea: %d", id);
                    }
                    break;

                case 4:  /* Consultar tareas */
                    /* Montamos el comando para consultar la lista de tareas */
                    sprintf(paquet, "4 consultar tareas");
                    break;

                case 5:  /* Salir */
                    printf("Saliendo del gestor de tareas.\n");
                    continue;

                default:
                    printf("Opción no válida. Inténtalo de nuevo.\n");
                    continue;
            }

            /* Enviamos el paquete */
            sendto(s, paquet, MIDA_PAQUET, 0, (struct sockaddr *)&contacte_servidor, sizeof(contacte_servidor));
            printf("Paquete enviado! Esperando respuesta...\n");

            /* Recibimos la respuesta del servidor */
            recvfrom(s, paquet, MIDA_PAQUET, 0, NULL, NULL);  /* NULL -> No es necesario saber desde dónde llega */
            printf("Respuesta del servidor: %s\n", paquet);
        }

        /* Cerramos el socket */
        close(s);
    }
    else
    {
        printf("El número de parámetros no es el correcto! Uso: %s <dirección IP del servidor> <puerto>\n", argv[0]);
    }

    return 0;
}
