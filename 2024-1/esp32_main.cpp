#include <Arduino.h>
// Instalar MODBUS-ESP8266 (ESP32)
#include "ModbusRTU.h"

#define ID_SLAVE 0x22 //34 dec
#define TEMP 0x00  // 0 dec
#define UMID 0x58 // 88 dec
#define PRES 0b01000011 // 67 dec

// Criar o objeto modbus
ModbusRTU mb;

void setup()
{
    Serial.begin(9600, 8N1); //9.600bps e 8bits de dados e 1bit paridade
    mb.begin(&Serial); // ligou modbus na serial
    // Definir o número do slave
    mb.Slave(ID_SLAVE);
    // Definir as variáveis do slave matr 8867
    mb.addHReg(TEMP);
    mb.addHReg(UMID);
    mb.addHReg(PRES);
}

long int tempo = 0;

void loop()
{
    if (millis() - tempo > 1000) // passo 1 segundo
    {
        // gerando valores sinteticos para as medidas
        static int temp = random(0, 400); // economiza RAM = static
        static int umid = random(0, 100);
        static int pres = random(0, 200);
        // inserindo medidas no Modbus
        mb.HReg(TEMP, temp);
        mb.HReg(UMID, umid);
        mb.HReg(PRES, pres);
        // atualizar para próximo segundo
        tempo = millis();
    }
    // rotina do modbus
    mb.task();
    yield();
}