#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT 8080

int main() {
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char *response = "HTTP/1.1 200 OK\nContent-Type: text/html\n\n<html><head><head><meta charset=\"UTF-8\"></head><body><h1>Olá, UNIFEI!</h1></body></html>";

    while (1)
    {
    // Cria o socket
        if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
            perror("Falha ao criar o socket");
            exit(EXIT_FAILURE);
        }

        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(PORT);

        // Faz o bind do socket ao endereço e porta
        if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
            perror("Falha ao fazer o bind");
            exit(EXIT_FAILURE);
        }

        // Escuta por conexões
        if (listen(server_fd, 3) < 0) {
            perror("Falha ao escutar por conexões");
            exit(EXIT_FAILURE);
        }

        // Aceita uma nova conexão
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("Falha ao aceitar a conexão");
            exit(EXIT_FAILURE);
        }

        // Envia a resposta para o cliente
        send(new_socket, response, strlen(response), 0);
        printf("Resposta enviada\n");
    }
    

    return 0;
}