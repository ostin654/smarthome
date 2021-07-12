#include <QuadDisplay2.h>
#include <OneWire.h>
#include <GyverTimer.h>
#include <ModbusKostin.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

//#define TROYKA_ENC

#ifdef TROYKA_ENC
#include <Encoder.h>
#else
#include <GyverEncoder.h>
#endif

#define DIR_PIN 2
#define DS_PIN A0
#define RELAY_PIN A4
#define QD_PIN 10

#ifdef TROYKA_ENC
#define ENCODER_PIN_A 8
#define ENCODER_PIN_B 7
#else
#define ENCODER_PIN_A 8
#define ENCODER_PIN_B 7
#define ENCODER_PIN_C 6
#endif

#define ADDRESS 0x10

#define EEPROM_ADDRESS_CURRENT_TEMP 0

OneWire ds(DS_PIN);
QuadDisplay qd(QD_PIN);

#ifdef TROYKA_ENC
Encoder myEnc(ENCODER_PIN_A, ENCODER_PIN_B);
#else
Encoder enc1(ENCODER_PIN_A, ENCODER_PIN_B, ENCODER_PIN_C);
#endif

GTimer_ms analogTimer;
GTimer_ms setTimer;
GTimer_ms displayTimer;
SoftwareSerial mySerial(0, 1);
ModbusKostin modbus(ADDRESS, &mySerial, DIR_PIN); // address, serial, expin

uint16_t myUptime;
uint16_t currentFloorTemperature;
uint16_t targetFloorTemperature;
uint8_t displayMode = 0;

// DS18B20
uint16_t dsGetTemperatureRaw()
{
  ds.reset();
  ds.write(0xCC);
  ds.write(0xBE);

  byte data[2];
  data[0] = ds.read();
  data[1] = ds.read();
  uint16_t temperature = (data[1] << 8) | data[0];

  ds.reset();
  ds.write(0xCC);
  ds.write(0x44, 1);

  return temperature;
}

void setup()
{  
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  qd.begin();
  qd.displayInt(0);

  analogTimer.setInterval(1137);
  displayTimer.setInterval(2089);
  setTimer.setInterval(4000);
  setTimer.setMode(MANUAL);

  #ifndef TROYKA_ENC
  enc1.setType(TYPE2);
  #endif

  currentFloorTemperature = dsGetTemperatureRaw();

  modbus.begin(38400);
  modbus.setInputRegisterLimits(1, 3);
  modbus.setOutputRegisterLimits(1, 1);
  modbus.setInputRegisterValue(1, 0);
  modbus.setInputRegisterValue(2, 0);
  modbus.setInputRegisterValue(3, 0);
  modbus.setCallbackFunc(setDataFromModbus);

  targetFloorTemperature = EEPROM.read(EEPROM_ADDRESS_CURRENT_TEMP);
  setDataFromModbus(1, targetFloorTemperature);
}
 
void loop()
{
  modbus.poll();
  
  #ifdef TROYKA_ENC
  static long oldPosition = 0;
  long newPosition = myEnc.read();
  
  if (newPosition != oldPosition) {
    oldPosition = newPosition;
    
    if (newPosition % 4 == 0) {
      targetFloorTemperature = newPosition / 4;
      setDataFromModbus(1, targetFloorTemperature);
      qd.displayInt(targetFloorTemperature);
      setTimer.start();
    }
  }
  #else
  enc1.tick();

  if (enc1.isLeft()) {
    setDataFromModbus(1, targetFloorTemperature-1);
    qd.displayInt(targetFloorTemperature);
    setTimer.start();
  }
  if (enc1.isRight()) {
    setDataFromModbus(1, targetFloorTemperature+1);
    qd.displayInt(targetFloorTemperature);
    setTimer.start();
  }
  #endif
  
  if (analogTimer.isReady()) {
    currentFloorTemperature = dsGetTemperatureRaw();
    myUptime = millis() / 60000;

    modbus.setInputRegisterValue(1, myUptime);
    modbus.setInputRegisterValue(2, currentFloorTemperature);
    modbus.setInputRegisterValue(3, targetFloorTemperature);

    if (millis() > 10000) {
      if (currentFloorTemperature * 0.0625 < targetFloorTemperature) {
        digitalWrite(RELAY_PIN, HIGH);
      }
      if (currentFloorTemperature * 0.0625 > targetFloorTemperature + 3) {
        digitalWrite(RELAY_PIN, LOW);
      }
    }
  }

  if (displayTimer.isReady() && setTimer.isReady()) {
    if (displayMode == 0) {
      qd.displayFloat((float)(currentFloorTemperature * 0.0625), 1);
    } else {
      qd.displayInt(targetFloorTemperature);
    }

    displayMode = (displayMode + 1) % 2;
  }
}

void setDataFromModbus(uint16_t registerNumber, uint16_t value) {
  if (registerNumber == 1) {
    targetFloorTemperature = value;
    if (targetFloorTemperature > 38) targetFloorTemperature = 38;
    if (targetFloorTemperature < 5) targetFloorTemperature = 5;
    EEPROM.put(EEPROM_ADDRESS_CURRENT_TEMP, (uint8_t)targetFloorTemperature);

    #ifdef TROYKA_ENC
    myEnc.write(targetFloorTemperature * 4);
    #endif
  }
}
