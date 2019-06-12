#include <GyverFilters.h>
#include <GyverTimer.h>
#include <ModbusKostin.h>

#define GAUGE_PIN A2
#define SERIAL_RX 2
#define SERIAL_TX 0
#define DIR_PIN 1

#define ADDRESS 0x0A

GTimer_ms analogTimer(1000);
GKalman analogFilter(40, 0.5);
SoftwareSerial softSerial(SERIAL_RX, SERIAL_TX);
ModbusKostin modbus(ADDRESS, &softSerial, DIR_PIN);

void setup()
{
  modbus.begin(38400);
  modbus.setRegisterLimits(1, 2);
}

void loop() {
  modbus.poll();

  if (analogTimer.isReady()) {
    uint16_t analogValue = 0;

    analogValue = analogRead(GAUGE_PIN);
    analogValue = analogFilter.filtered(analogValue);

    modbus.setRegisterValue(1, millis() / 60000);
    modbus.setRegisterValue(2, analogValue);
  }
}
