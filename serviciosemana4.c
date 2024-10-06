#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <pthread.h>
#include <ctype.h> // Para tolower() y toupper()

int global_counter;

// Mutex para asegurar el acceso exclusivo
pthread_mutex_t access_mutex = PTHREAD_MUTEX_INITIALIZER;

int client_count;
int client_sockets[100];

void *ClientHandler(void *client_socket) {
    int connection_socket;
    int *socket_pointer;
    socket_pointer = (int *)client_socket;
    connection_socket = *socket_pointer;

    char request[512];
    char reply[512];
    int request_len;

    int end_connection = 0;

    // Bucle para manejar las solicitudes del cliente
    while (end_connection == 0) {
        // Recibimos la solicitud
        request_len = read(connection_socket, request, sizeof(request));
        printf("Received\n");

        // Añadimos el carácter nulo para finalizar la cadena
        request[request_len] = '\0';

        printf("Request: %s\n", request);

        // Procesamos el código de la solicitud
        char *token = strtok(request, "/");
        int operation_code = atoi(token);
        int form_id;
        char user_name[20];

        if (operation_code != 0) {
            token = strtok(NULL, "/");
            form_id = atoi(token);
            token = strtok(NULL, "/");
            strcpy(user_name, token);
            printf("Code: %d, Name: %s\n", operation_code, user_name);
        }

        if (operation_code == 0) // Desconectar
            end_connection = 1;
        else if (operation_code == 1) // Longitud del nombre
            sprintf(reply, "1/%d/%d", form_id, strlen(user_name));
        else if (operation_code == 2) // Si el nombre es bonito
        {
            if ((user_name[0] == 'M') || (user_name[0] == 'S'))
                sprintf(reply, "2/%d/SI", form_id);
            else
                sprintf(reply, "2/%d/NO", form_id);
        } else if (operation_code == 3) // Altura del usuario
        {
            token = strtok(NULL, "/");
            float height = atof(token);
            if (height > 1.70)
                sprintf(reply, "3/%d/%s: eres alto", form_id, user_name);
            else
                sprintf(reply, "3/%d/%s: eres bajo", form_id, user_name);
        } else if (operation_code == 4) // Verificar si el nombre es palíndromo
        {
            int length = strlen(user_name);
            int is_palindrome = 1;
            for (int i = 0, j = length - 1; i < j; i++, j--) {
                if (tolower(user_name[i]) != tolower(user_name[j])) {
                    is_palindrome = 0;
                    break;
                }
            }

            if (is_palindrome)
                sprintf(reply, "4/%d/%s es un palíndromo", form_id, user_name);
            else
                sprintf(reply, "4/%d/%s no es un palíndromo", form_id, user_name);
        } else if (operation_code == 5) // Convertir el nombre a mayúsculas
        {
            for (int i = 0; i < strlen(user_name); i++) {
                user_name[i] = toupper(user_name[i]);
            }
            sprintf(reply, "5/%d/%s", form_id, user_name);
        }

        if (operation_code != 0) {
            printf("Reply: %s\n", reply);
            // Enviar respuesta al cliente
            write(connection_socket, reply, strlen(reply));
        }

        // Aumentar el contador global para ciertos códigos
        if ((operation_code >= 1) && (operation_code <= 5)) {
            pthread_mutex_lock(&access_mutex); // Bloquear para incrementar el contador
            global_counter++;
            pthread_mutex_unlock(&access_mutex); // Desbloquear

            // Notificar a todos los clientes conectados
            char update[20];
            sprintf(update, "4/%d", global_counter);
            for (int j = 0; j < client_count; j++)
                write(client_sockets[j], update, strlen(update));
        }
    }
    // Terminar conexión con el cliente
    close(connection_socket);
}

int main(int argc, char *argv[]) {
    int server_socket, listening_socket;
    struct sockaddr_in server_address;

    // Inicializar socket
    if ((listening_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        printf("Error creando el socket");

    // Asignar el socket al puerto
    memset(&server_address, 0, sizeof(server_address)); // Inicializar a cero
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY); // Asociar socket a cualquier IP
    server_address.sin_port = htons(9050); // Puerto de escucha

    if (bind(listening_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
        printf("Error en el bind");

    if (listen(listening_socket, 3) < 0)
        printf("Error en el listen");

    global_counter = 0;

    pthread_t thread;
    client_count = 0;
    for (;;) {
        printf("Listening for connections\n");

        server_socket = accept(listening_socket, NULL, NULL);
        printf("Connection established\n");

        client_sockets[client_count] = server_socket;

        // Crear hilo para manejar el cliente
        pthread_create(&thread, NULL, ClientHandler, &client_sockets[client_count]);
        client_count++;
    }
}
