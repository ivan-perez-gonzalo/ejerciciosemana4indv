#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <pthread.h>

int client_counter;

// Mutex para asegurar acceso exclusivo
pthread_mutex_t access_mutex = PTHREAD_MUTEX_INITIALIZER;

int client_index;
int client_socks[100];

void *HandleClient(void *client_sock)
{
    int conn_sock;
    int *sock_ptr;
    sock_ptr = (int *)client_sock;
    conn_sock = *sock_ptr;

    char request[512];
    char response[512];
    int bytes_received;

    int disconnect = 0;
    // Bucle para atender peticiones del cliente hasta que se desconecte
    while (disconnect == 0)
    {
        // Recibir la solicitud
        bytes_received = read(conn_sock, request, sizeof(request));
        printf("Mensaje recibido\n");

        // Añadimos el carácter nulo para terminar la cadena
        request[bytes_received] = '\0';

        printf("Petición: %s\n", request);

        // Analizar el código de la solicitud
        char *token = strtok(request, "/");
        int operation_code = atoi(token);
        int form_id;
        float temp_value;

        if (operation_code != 0)
        {
            token = strtok(NULL, "/");
            form_id = atoi(token);
            token = strtok(NULL, "/");
            temp_value = atof(token);
            printf("Código: %d, Temperatura: %.2f\n", operation_code, temp_value);
        }

        if (operation_code == 0) // Solicitud de desconexión
        {
            disconnect = 1;
        }
        else if (operation_code == 1) // Convertir de Celsius a Fahrenheit
        {
            float fahrenheit_value = (temp_value * 9.0 / 5.0) + 32.0;
            sprintf(response, "1/%d/%.2f Celsius son %.2f Fahrenheit", form_id, temp_value, fahrenheit_value);
        }
        else if (operation_code == 2) // Convertir de Fahrenheit a Celsius
        {
            float celsius_value = (temp_value - 32.0) * 5.0 / 9.0;
            sprintf(response, "2/%d/%.2f Fahrenheit son %.2f Celsius", form_id, temp_value, celsius_value);
        }

        if (operation_code != 0)
        {
            printf("Respuesta: %s\n", response);
            // Enviar la respuesta al cliente
            write(conn_sock, response, strlen(response));
        }

        // Si la solicitud es código 1 o 2, incrementamos el contador global
        if ((operation_code == 1) || (operation_code == 2))
        {
            pthread_mutex_lock(&access_mutex); // Bloqueo para incrementar el contador
            client_counter = client_counter + 1;
            pthread_mutex_unlock(&access_mutex); // Desbloqueo

            // Notificar a todos los clientes conectados
            char notification[20];
            sprintf(notification, "4/%d", client_counter);
            for (int j = 0; j < client_index; j++)
                write(client_socks[j], notification, strlen(notification));
        }
    }
    // Finalizamos el servicio para este cliente
    close(conn_sock);
}

int main(int argc, char *argv[])
{
    int server_sock, listening_sock;
    struct sockaddr_in server_addr;

    // INICIALIZACIONES
    // Creación del socket
    if ((listening_sock = socket(AF_INET
