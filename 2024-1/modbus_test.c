#include <stdio.h>   /* Standard input/output definitions */
#include <string.h>  /* String function definitions */
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */

/*
Só roda em Linux: arquivo de serial="/dev/ttyUSB0"
Precisa verificar se o ESP32 está gravado e conectado!
Caso sua Holding Register para as variáveis seja diferente, deverá mudar o CRC
TEMP 10
UMID 11
PRES 12
Calcular CRC: https://crccalc.com/
Passar a string Hex completa para calcular sem CRC
Usar o CRC-16/MODBUS com os bytes invertidos
*/

int main()
{
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

    float tempVal = 0.0f;
    float umidVal = 0.0f;
    float presVal = 0.0f;

    while (1) // repete infinitamente
    {
        printf("Temp: ");
        bytes_escritos = write(fd, temp, 8);

        usleep(200 * 1000); // 200 ms

        bytes_lidos = read(fd, resposta, 8);
        if (bytes_lidos > 4)
        {
            tempVal = (int)((resposta[3] << 8) + resposta[4]) / 10.0f;
            printf("%.1f\n", tempVal);
        }
        else
            printf("Sem resposta!\n");

        usleep(200 * 1000);

        printf("umid: ");
        bytes_escritos = write(fd, umid, sizeof(umid));

        usleep(200 * 1000);

        bytes_lidos = read(fd, resposta, 8);
        if (bytes_lidos > 4)
        {
            umidVal = (int)((resposta[3] << 8) + resposta[4]) / 100.0f;
            printf("%.2f\n", umidVal);
        }
        else
            printf("Sem resposta!\n");
        
        usleep(200 * 1000);

        printf("pres: ");
        bytes_escritos = write(fd, pres, sizeof(pres));

        usleep(200 * 1000);

        bytes_lidos = read(fd, resposta, 8);
        if (bytes_lidos > 4)
        {
            presVal = (int)((resposta[3] << 8) + resposta[4]) / 100.0f;
            printf("%.2f\n", presVal);
        }
        else
            printf("Sem resposta!\n");
    }

    close(fd);

    printf("Terminado\n");
    return 0;
}