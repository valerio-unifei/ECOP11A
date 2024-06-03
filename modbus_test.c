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

void main()
{
    printf("Configurando porta Serial:\n");
    char *portname = "/dev/ttyUSB0";
    int fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        perror("Erro ao abrir a porta serial");
        return 1;
    }
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
    int bytes_written = 0;
    int bytes_read = 0;
    char response[10];

    /* 0x01, 0x03, 0x00, 0xHH, 0x00, 0x01, 0xC2, 0xC1 */
    char temp[] = {0x01, 0x03, 0x00, 0x0A, 0x00, 0x01, 0xA4, 0x08};
    char umid[] = {0x01, 0x03, 0x00, 0x0B, 0x00, 0x01, 0xF5, 0xC8};
    char pres[] = {0x01, 0x03, 0x00, 0x0C, 0x00, 0x01, 0x44, 0x09};

    float tempVal = 0.0f;
    float umidVal = 0.0f;
    float presVal = 0.0f;


    while (1)
    {
        printf("Temp: ");
        bytes_written = write(fd, temp, sizeof(temp));
        usleep(200 * 1000);
        bytes_read = read(fd, response, 8);
        if (bytes_read > 4)
        {
            tempVal = (int)((response[3] << 8) + response[4]) / 10.0f;
            printf("%.1f\n", tempVal);
        }
        else
            printf("Sem resposta!\n");

        usleep(200 * 1000);
        printf("umid: ");
        bytes_written = write(fd, umid, sizeof(umid));
        usleep(200 * 1000);
        bytes_read = read(fd, response, 8);
        if (bytes_read > 4)
        {
            umidVal = (int)((response[3] << 8) + response[4]) / 100.0f;
            printf("%.2f\n", umidVal);
        }
        else
            printf("Sem resposta!\n");
        
        usleep(200 * 1000);
        printf("pres: ");
        bytes_written = write(fd, pres, sizeof(pres));
        usleep(200 * 1000);
        bytes_read = read(fd, response, 8);
        if (bytes_read > 4)
        {
            presVal = (int)((response[3] << 8) + response[4]) / 100.0f;
            printf("%.2f\n", presVal);
        }
        else
            printf("Sem resposta!\n");
    }

    close(fd);

    printf("Terminado\n");
    return 0;
}