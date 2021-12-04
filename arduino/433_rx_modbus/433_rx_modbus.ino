#include <GyverTimer.h>
#include <ModbusKostin.h>
#include <RH_ASK.h>

#define SERIAL_RX 7
#define SERIAL_TX 5
#define DIR_PIN 6
#define RF_PIN 8

#define ADDRESS 0x0D

SoftwareSerial softSerial(SERIAL_RX, SERIAL_TX);
ModbusKostin modbus(ADDRESS, &softSerial, DIR_PIN);
GTimer_ms listenTimer;
RH_ASK driver(2000, RF_PIN, 0, 0);
uint8_t buf[6];
uint8_t buflen = 6;
bool mode = false;

uint16_t calculateCRC(uint8_t u8length) {
  unsigned int temp, temp2, flag;
  temp = 0xFFFF;
  for (uint8_t i = 0; i < u8length; i++) {
    temp = temp ^ buf[i];
    for (uint8_t j = 1; j <= 8; j++) {
      flag = temp & 0x0001;
      temp >>=1;
      if (flag) {
        temp ^= 0xA001;
      }
    }
  }
  // Reverse byte order.
  temp2 = temp >> 8;
  temp = (temp << 8) | temp2;
  temp &= 0xFFFF;
  // the returned value is already swapped
  // crcLo byte is first & crcHi byte is last
  return temp;
}

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  Serial.begin(9600);
  while (!Serial);

  listenTimer.setInterval(61865);

  driver.init();

  modbus.begin(38400);
  modbus.setInputRegisterLimits(1, 3);

  modbus.setInputRegisterValue(1, 0);
  modbus.setInputRegisterValue(2, 0);
  modbus.setInputRegisterValue(3, 0);
}

void loop()
{
  if (listenTimer.isReady()) {
    mode = !mode;

    if (mode) {
      modbus.setInputRegisterValue(2, 0);
      modbus.setInputRegisterValue(3, 0);
    }
  }

  if (mode) {
    digitalWrite(LED_BUILTIN, LOW);
    modbus.setInputRegisterValue(1, millis() / 60000);

    if (driver.recv(buf, &buflen)) {
      uint16_t address = buf[1];
      address <<= 8;
      address |= buf[0];

      uint32_t temperature = buf[3];
      temperature <<= 8;
      temperature |= buf[2];

      uint16_t crc = buf[5];
      crc <<= 8;
      crc |= buf[4];

      if (crc == calculateCRC(4)) {
        switch (address) {
          case 0x37:
            modbus.setInputRegisterValue(2, temperature+10000);
            break;
          case 0x28:
            modbus.setInputRegisterValue(3, temperature+10000);
            break;
          default:
            Serial.println("Invalid Address!");
            break;
        }
    
        Serial.print("Address: ");
        Serial.print(address, HEX);
        Serial.print(" Temperature: ");
        Serial.print(temperature);
        Serial.println();
      } else {
        Serial.println("Invalid CRC16!");
      }
    }
  } else {
    digitalWrite(LED_BUILTIN, HIGH);
    modbus.poll();
  }
}
