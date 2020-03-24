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
uint8_t buf[4];
uint8_t buflen = 4;
uint8_t address = 0;
uint32_t temperature = 0;
uint8_t tries = 0;

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  Serial.begin(9600);
  while (!Serial);

  listenTimer.setInterval(60000);
  listenTimer.setMode(MANUAL);

  driver.init();

  modbus.begin(38400);
  modbus.setRegisterLimits(1, 2);

  modbus.setRegisterValue(1, 0);
  modbus.setRegisterValue(2, 0);
}

void loop()
{
  if (listenTimer.isReady()) {
    digitalWrite(LED_BUILTIN, LOW);
    if (driver.recv(buf, &buflen)) {
      address = buf[1];
  
      temperature = buf[3];
      temperature <<=8;
      temperature |= buf[2];
  
      modbus.setRegisterValue(1, millis() / 60000);
      modbus.setRegisterValue(2, temperature);
  
      Serial.print("Address: ");
      Serial.print(address, HEX);
      Serial.print(" Temperature: ");
      Serial.println(temperature);
      
      digitalWrite(LED_BUILTIN, HIGH);
      delay(30);
      digitalWrite(LED_BUILTIN, LOW);
  
      tries++;
  
      if (tries>3) {
        tries = 0;
        listenTimer.reset();
      }
    }
  } else {
    digitalWrite(LED_BUILTIN, HIGH);
    modbus.poll();
  }
}
