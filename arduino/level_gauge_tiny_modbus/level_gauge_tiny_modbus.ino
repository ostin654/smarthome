#include <GyverFilters.h>
#include <GyverTimer.h>
#include <ModbusKostin.h>

#define ENABLE_KEY
#define KEY_ON 500
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
#ifdef ENABLE_KEY
GTimer_ms keyTimer(971);
#endif
GKalman analogFilter(40, 0.5);
SoftwareSerial softSerial(SERIAL_RX, SERIAL_TX);
ModbusKostin modbus(ADDRESS, &softSerial, DIR_PIN);

void setup()
{
  modbus.begin(38400);
  modbus.setInputRegisterLimits(1, 2);
  #ifdef ENABLE_KEY
  pinMode(KEY_PIN, OUTPUT);
  digitalWrite(KEY_PIN, LOW);
  #endif

  delay(60000);
}

void loop() {
  modbus.poll();

  if (analogTimer.isReady()) {
    uint16_t analogValue = 0;

    analogValue = analogRead(GAUGE_PIN);
    analogValue = analogFilter.filtered(analogValue);

    modbus.setInputRegisterValue(1, millis() / 60000);
    modbus.setInputRegisterValue(2, analogValue);

    #ifdef ENABLE_KEY
    if (analogValue > KEY_ON && keyTimer.isReady()) {
      digitalWrite(KEY_PIN, HIGH);
      delay(10);
      digitalWrite(KEY_PIN, LOW);
    }
    #endif
  }
}
