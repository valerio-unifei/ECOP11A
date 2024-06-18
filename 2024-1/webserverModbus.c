#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <time.h>

#define min(a, b) ((a < b) ? a : b)

#define PORT 8080
#define BUFFER_SIZE 1024

// Estrutura para armazenar uma medição
struct Medicao {
    time_t instante;    // Armazena o instante de tempo
    float temperatura;  // Armazena a temperatura em graus Celsius
    float umidade;      // Armazena a umidade em porcentagem
    float pressao;      // Armazena a pressão em hPa
};

int main()
{
    struct Medicao medidas[1000]; // Array para armazenar até 10 medições
    int med_n = 0; // Índice atual do array de medições

    // Inicialização do servidor web
    char buffer[BUFFER_SIZE];
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Criação do socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("erro na criação do socket");
        exit(1);
    }

    // Configuração do endereço do servidor
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    // Ligação do socket ao endereço e porta especificados
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("erro na ligação do socket ao endereço e porta");
        exit(1);
    }

    // Configuração do socket para escutar conexões
    if (listen(server_socket, 3) < 0) {
        perror("erro do socket para escutar conexões");
        exit(1);
    }
    printf("Servidor iniciado na porta %d.\n", PORT);

    // Abrindo porta serial do ESP32
    printf("Configurando porta Serial:\n");
    char *portname = "/dev/ttyUSB0";
    int fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
    struct termios tty;
    if (fd > 0) {
        // Configurando comunicação da porta serial do ESP32
        memset(&tty, 0, sizeof(tty));
        if (tcgetattr(fd, &tty) != 0) {
            perror("Erro ao obter os atributos da porta serial");
            return 1;
        }
        cfsetospeed(&tty, B9600);
        cfsetispeed(&tty, B9600);
        tty.c_cflag &= ~PARENB;
        tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CSIZE;
        tty.c_cflag |= CS8;
        tty.c_cflag |= CREAD | CLOCAL;
        tty.c_iflag &= ~(IXON | IXOFF | IXANY);
        tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
        tty.c_oflag &= ~OPOST;
        tty.c_cc[VMIN] = 0;
        tty.c_cc[VTIME] = 10;
        if (tcsetattr(fd, TCSANOW, &tty) != 0) {
            perror("Erro ao configurar a porta serial");
            return 1;
        }
    }
    else {
        perror("Erro ao abrir a porta serial");
    }

    // Comandos Modbus para cada medida
    unsigned char temp[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x01, 0x84, 0x0A};
    unsigned char umid[] = {0x01, 0x03, 0x00, 0x58, 0x00, 0x01, 0x05, 0xD9};
    unsigned char pres[] = {0x01, 0x03, 0x00, 0x43, 0x00, 0x01, 0x75, 0xDE};
    unsigned char resposta[10];

    clock_t tempo_anterior = clock(); // Tempo anterior para controle de tempo
    clock_t tempo_atual; // Tempo atual
    double tempo_difer; // Diferença de tempo

    while (1) {
        // Leitura Modbus a cada 1 segundo
        tempo_atual = clock();
        tempo_difer = (double)(tempo_atual - tempo_anterior) / CLOCKS_PER_SEC * 1000;
        if (tempo_difer >= 1000 && fd >= 0) {
            // Armazenando o instante de tempo da medição
            medidas[med_n].instante = time(NULL);

            // Medição de temperatura
            write(fd, temp, 8);
            usleep(10 * 1000);
            read(fd, resposta, 8);
            medidas[med_n].temperatura = (int)((resposta[3] << 8) + resposta[4]) / 10.0f;

            // Medição de umidade
            write(fd, umid, 8);
            usleep(10 * 1000);
            read(fd, resposta, 8);
            medidas[med_n].umidade = (int)((resposta[3] << 8) + resposta[4]) / 10.0f;

            // Medição de pressão
            write(fd, pres, 8);
            usleep(10 * 1000);
            read(fd, resposta, 8);
            medidas[med_n].pressao = (int)((resposta[3] << 8) + resposta[4]) / 10.0f;

            // Atualiza o índice do array de medições
            med_n = (med_n >= 9) ? 0 : med_n + 1;
            tempo_anterior = tempo_atual; // Atualiza o tempo anterior
        }

        // Conexão do cliente no servidor web
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket < 0) {
            perror("Erro na conexão do cliente\n");
            continue;
        }

        // Recebendo a requisição do cliente
        recv(client_socket, buffer, BUFFER_SIZE, 0);
        printf("Requisição: %s\n", buffer);

        // Preparando a resposta HTTP
        char response_header[] = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n";
        char response_body[BUFFER_SIZE];
        sprintf(response_body,
                "<html><head><title>Medições</title></head><body><h1>Tabela de Medições</h1>"
                "<table border='1'>"
                "<tr><th>Instante</th><th>Temperatura (°C)</th><th>Umidade (%)</th><th>Pressão (hPa)</th></tr>");

        // Adicionando as medições na tabela HTML
        for (int i = 0; i < min(med_n, sizeof(medidas)); i++) {
            char row[256];
            struct tm *tm_info = localtime(&medidas[i].instante);
            char time_str[26];
            strftime(time_str, 26, "%Y-%m-%d %H:%M:%S", tm_info);
            sprintf(row, "<tr><td>%s</td><td>%.1f</td><td>%.1f</td><td>%.1f</td></tr>", time_str, medidas[i].temperatura, medidas[i].umidade, medidas[i].pressao);
            strcat(response_body, row);
        }

        strcat(response_body, "</table></body></html>");

        // Enviando a resposta HTTP para o cliente
        send(client_socket, response_header, strlen(response_header), 0);
        send(client_socket, response_body, strlen(response_body), 0);
        close(client_socket); // Fechando a conexão com o cliente
    }

    close(server_socket); // Fechando o socket do servidor
    return 0;
}
