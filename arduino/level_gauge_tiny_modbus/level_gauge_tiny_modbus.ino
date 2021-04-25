#include <GyverFilters.h>
#include <GyverTimer.h>
#include <ModbusKostin.h>

#define ENABLE_KEY
#define KEY_ON 500
#define KEY_OFF 400
#define KEY_PIN 5

// for attiny84
#define GAUGE_PIN A4
#define SERIAL_RX 8
#define SERIAL_TX 10
#define DIR_PIN 9

//// for attiny85
//#define GAUGE_PIN A2
//#define SERIAL_RX 2
//#define SERIAL_TX 0
//#define DIR_PIN 1

//#define ADDRESS 0x0A
//#define ADDRESS 0x0B
//#define ADDRESS 0x0C
//#define ADDRESS 0x0D
//#define ADDRESS 0x0E
#define ADDRESS 0x0F

GTimer_ms analogTimer(1000);
GKalman analogFilter(40, 0.5);
SoftwareSerial softSerial(SERIAL_RX, SERIAL_TX);
ModbusKostin modbus(ADDRESS, &softSerial, DIR_PIN);

void setup()
{
  modbus.begin(38400);
  modbus.setRegisterLimits(1, 2);
  #ifdef ENABLE_KEY
  pinMode(KEY_PIN, OUTPUT);
  digitalWrite(KEY_PIN, LOW);
  #endif
}

void loop() {
  modbus.poll();

  if (analogTimer.isReady()) {
    uint16_t analogValue = 0;

    analogValue = analogRead(GAUGE_PIN);
    analogValue = analogFilter.filtered(analogValue);

    modbus.setRegisterValue(1, millis() / 60000);
    modbus.setRegisterValue(2, analogValue);

    #ifdef ENABLE_KEY
    if (analogValue > KEY_ON) {
      digitalWrite(KEY_PIN, HIGH);
    }
    if (analogValue < KEY_OFF) {
      digitalWrite(KEY_PIN, LOW);
    }
    #endif
  }
}
