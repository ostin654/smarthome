
//#define DEBUG 1
#define __WAWGAT_NANO__

#if defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__) || defined(__AVR_ATtiny2313__) || defined(__AVR_ATtiny4313__) || defined(__AVR_ATtiny861__)
  #include <TinyBME280.h>
  TinyBME280 bme;
#else
  #include <Adafruit_BME280.h>
  Adafruit_BME280 bme;
#endif

#include <ModbusKostin.h>

#define BTN_PIN 0
#define DOOR_PIN 5
#define SENSOR_PIN 7
#define SOIL_PIN A2
#define PRESS_PIN A1
#define RELAY_PIN 10
#define MAX_DRY 400000
#define DRY_DELAY 172000000

#define ADDRESS 0x0B

#define ANALOG_DELAY 2000

#if defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__) || defined(__AVR_ATtiny2313__) || defined(__AVR_ATtiny4313__) || defined(__AVR_ATtiny861__)
  #define RX_PIN 1
  #define DIR_PIN 2
  #define TX_PIN 3
  SoftwareSerial mySerial(RX_PIN, TX_PIN);
  ModbusKostin modbus(ADDRESS, &mySerial, DIR_PIN);
#else
  #define DIR_PIN 4
  ModbusKostin modbus(ADDRESS, 1, DIR_PIN);
#endif

uint32_t lastAnalogRead = 0;

uint16_t soilHumidity = 0;
uint16_t waterPressure = 0;
uint16_t airTemperature = 0;
uint16_t relayState = 0;
uint16_t doorState = 0;

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  pinMode(SENSOR_PIN, OUTPUT);
  digitalWrite(SENSOR_PIN, LOW);
  pinMode(DOOR_PIN, OUTPUT);
  digitalWrite(DOOR_PIN, LOW);
  pinMode(BTN_PIN, INPUT_PULLUP);

  #ifdef DEBUG
  Serial.begin(38400);
  delay(1000);
  Serial.println("Begin");
  #endif

  modbus.begin(38400);
  modbus.setRegisterLimits(1, 6);
  modbus.setRegisterValue(1, 0);
  modbus.setRegisterValue(2, 0);
  modbus.setRegisterValue(3, 0);
  modbus.setRegisterValue(4, 0);
  modbus.setRegisterValue(5, 0);
  modbus.setRegisterValue(6, 0);
  
  if (!bme.begin(0x76)) {
    #ifdef DEBUG
    Serial.println("BME error");
    #endif
    while(1) {
      digitalWrite(DIR_PIN, HIGH);
      delay(500);
      digitalWrite(DIR_PIN, LOW);
      delay(500);
    }
  }
  
  #ifdef DEBUG
  Serial.println("OK");
  #endif
}
 
void loop() {
  modbus.poll();

  if (lastAnalogRead + ANALOG_DELAY < millis()) {
    digitalWrite(SENSOR_PIN, HIGH);
    delay(20);
    soilHumidity = analogRead(SOIL_PIN);
    digitalWrite(SENSOR_PIN, LOW);

    waterPressure = analogRead(PRESS_PIN);

    airTemperature = bme.readTemperature() * 100;

    lastAnalogRead = millis();

    if (millis() % DRY_DELAY < MAX_DRY) {
      relayState = 1;
    } else {
      relayState = 0;
    }

    if (airTemperature > 3100) {
      doorState = 1;
    }
    if (airTemperature < 2300) {
      doorState = 0;
    }

    // todo use btn!!!

    modbus.setRegisterValue(1, millis() / 60000);
    modbus.setRegisterValue(2, soilHumidity);
    modbus.setRegisterValue(3, waterPressure);
    modbus.setRegisterValue(4, airTemperature);
    modbus.setRegisterValue(5, relayState);
    modbus.setRegisterValue(6, doorState);

    digitalWrite(RELAY_PIN, relayState);
    digitalWrite(DOOR_PIN, doorState);
  }

  //Wraparound
  if (lastAnalogRead > millis() + 1000) {
    lastAnalogRead = millis();
  }
}
