#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h> //UBUNTU
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <time.h>

#define PORT 8080
#define BUFFER_SIZE 1024

struct Medicao {
    time_t instante;    // Armazena o instante de tempo
    float temperatura;  // Armazena a temperatura em graus Celsius
    float umidade;      // Armazena a umidade em porcentagem
    float pressao;      // Armazena a pressão em hPa
};

int main()
{
    struct Medicao medidas[10];
    int med_n = 0;

    /* Inicizalição do servidor web*/
    char buffer[BUFFER_SIZE];
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("socket creation failed");
        exit(1);
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        exit(1);
    }
    if (listen(server_socket, 3) < 0) {
        perror("listen failed");
        exit(1);
    }
    printf("Server started on port %d...\n", PORT);

    // Abrindo porta serial do ESP32
    printf("Configurando porta Serial:\n");
    char *portname = "/dev/ttyUSB0";
    int fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        perror("Erro ao abrir a porta serial");
        return 1;
    }
    // Configurando comunicação da porta serial do ESP32
    struct termios tty;
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
    //tty.c_cflag &= ~CRTSCTS;
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
    // transmissao da porta serial em MODBUS
    int bytes_escritos = 0;
    int bytes_lidos = 0;
    char resposta[10];
    // comandos modbus para cada medida
    // ID_SLAVE, CMD_Leitura (0x03), Med_H, Med_L, SE_H, SE_L, CRC_L, CRC_H}
    char temp[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x01, 0x84, 0x0A};
    char umid[] = {0x01, 0x03, 0x00, 0x58, 0x00, 0x01, 0x05, 0xD9};
    char pres[] = {0x01, 0x03, 0x00, 0x43, 0x00, 0x01, 0x75, 0xDE};

    /* controle de tempo na chamada da serial*/
    clock_t tempo_anterior = clock();
    clock_t tempo_atual;
    double tempo_difer;

    while (1) {
        /* Leitura ModBus a cada 1 segundo*/
        tempo_atual = clock();
        tempo_difer = (double)(tempo_atual - tempo_anterior) / CLOCKS_PER_SEC * 1000;
        if (tempo_difer >= 1000) {
        {
            // instante de tempo
            medidas[med_n].instante = time(NULL);
            // medicao de temperatura
            bytes_escritos = write(fd, temp, 8);
            usleep(10 * 1000);
            bytes_lidos = read(fd, resposta, 8);
            medidas[med_n].temperatura = (int)((resposta[3] << 8) + resposta[4]) / 10.0f;
            // medicao de umidade
            bytes_escritos = write(fd, umid, 8);
            usleep(10 * 1000);
            bytes_lidos = read(fd, resposta, 8);
            medidas[med_n].umidade = (int)((resposta[3] << 8) + resposta[4]) / 10.0f;
            // medicao de pressao
            bytes_escritos = write(fd, pres, 8);
            usleep(10 * 1000);
            bytes_lidos = read(fd, resposta, 8);
            medidas[med_n].pressao = (int)((resposta[3] << 8) + resposta[4]) / 10.0f;
            // retorna para inicio do vetor se > 9
            med_n = (med_n >= 9)? 0 : med_n + 1 ;
        }

        /* Conexao do cliente no servidor Web */
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket < 0) {
            perror("accept failed");
            continue;
        }

        char response[] = "HTTP/1.1 200 OK\r\n"
                      "Content-Type: text/html\r\n"
                      "\r\n"
                      "<html><body><h1>Welcome to my web server!</h1></body></html>";

        recv(client_socket, buffer, BUFFER_SIZE, 0);
        printf("Received request: %s\n", buffer);

        send(client_socket, response, strlen(response), 0);
        close(client_socket);
    }

    return 0;
}