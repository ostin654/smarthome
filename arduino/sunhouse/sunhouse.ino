//#define MODBUS_DEBUG

#include <Wire.h>
#include "Adafruit_BMP280.h"
#include <LiquidCrystal_I2C.h>
#include <DS3231.h>
#include <GyverEncoder.h>
#include <GyverTimer.h>
#include <ModbusKostin.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

#if defined(ARDUINO) && ARDUINO >= 100
#define printByte(args)  write(args);
#else
#define printByte(args)  print(args,BYTE);
#endif

#define DOOR_PIN 8
#define VALVE_PIN 9

#define EEPROM_ADDRESS_START_HOUR 0
#define EEPROM_ADDRESS_LIMIT_MINUTE 1
#define EEPROM_ADDRESS_CLOSE_TEMP 2
#define EEPROM_ADDRESS_TEMP_TRESH 3

Adafruit_BMP280 bmp; // I2C
LiquidCrystal_I2C lcd(0x27,16,2);
Encoder enc1(7, 6, 5);
GTimer_ms analogTimer;
GTimer_ms backlightTimer;
SoftwareSerial mySerial(A0, A1);
ModbusKostin modbus(0x0B, &mySerial, 2); // address, serial, expin
DS3231 Clock;

bool Century;
bool h12;
bool PM;

char myStr[20];

byte editMode = 0;
byte startHour = 0;
byte limitMinute = 0;
byte closeTemp = 0;
byte tempTresh = 0;

void setup() {
  if (!bmp.begin(0x76)) {
    while (1);
  }

  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                  Adafruit_BMP280::SAMPLING_X2,
                  Adafruit_BMP280::SAMPLING_NONE,
                  Adafruit_BMP280::FILTER_OFF,
                  Adafruit_BMP280::STANDBY_MS_1000);

  analogTimer.setInterval(1000);
  backlightTimer.setInterval(9000);
  enc1.setType(TYPE2);
  lcd.init();
  lcd.backlight();

  modbus.begin(38400);
  modbus.setInputRegisterLimits(1, 4);
  modbus.setOutputRegisterLimits(1, 7);
  modbus.setInputRegisterValue(1, 0);
  modbus.setInputRegisterValue(2, 0);
  modbus.setInputRegisterValue(3, 0);
  modbus.setInputRegisterValue(4, 0);
  modbus.setCallbackFunc(setDataFromModbus);

  pinMode(VALVE_PIN, OUTPUT); // valve
  digitalWrite(VALVE_PIN, LOW);
  pinMode(DOOR_PIN, OUTPUT); // door
  digitalWrite(DOOR_PIN, LOW);

  EEPROM.get(EEPROM_ADDRESS_START_HOUR, startHour);
  EEPROM.get(EEPROM_ADDRESS_LIMIT_MINUTE, limitMinute);
  EEPROM.get(EEPROM_ADDRESS_CLOSE_TEMP, closeTemp);
  EEPROM.get(EEPROM_ADDRESS_TEMP_TRESH, tempTresh);
}

void updateTime() {
  sprintf(myStr, "%02d:%02d:%02d", Clock.getHour(h12, PM), Clock.getMinute(), Clock.getSecond());
  lcd.setCursor(0,0);
  lcd.print(myStr);
}

void backLight() {
  lcd.backlight();
  backlightTimer.reset();
  backlightTimer.start();
}

void setupUpdate() {
  lcd.clear();
  backLight();

  switch (editMode) {
    case 1:
      sprintf(myStr, "%02d              ", Clock.getHour(h12, PM));
      lcd.setCursor(0,0);
      lcd.print(myStr);
      break;
    case 2:
      sprintf(myStr, "   %02d           ", Clock.getMinute());
      lcd.setCursor(0,0);
      lcd.print(myStr);
      break;
    case 3:
      sprintf(myStr, "      %02d        ", Clock.getSecond());
      lcd.setCursor(0,0);
      lcd.print(myStr);
      break;
    case 4:
      sprintf(myStr, "%02dh             ", startHour);
      lcd.setCursor(0,1);
      lcd.print(myStr);
      break;
    case 5:
      sprintf(myStr, "   %02dm          ", limitMinute);
      lcd.setCursor(0,1);
      lcd.print(myStr);
      break;
    case 6:
      sprintf(myStr, "          %02d/   ", closeTemp);
      lcd.setCursor(0,1);
      lcd.print(myStr);
      break;
    case 7:
      sprintf(myStr, "            /%02d ", tempTresh);
      lcd.setCursor(0,1);
      lcd.print(myStr);
      break;
  }
}

void loop() {
  int16_t tempRaw;
  float tempPretty;
  static uint16_t relayState = 0;
  static uint16_t doorState = 0;

  enc1.tick();
  modbus.poll();

  if (enc1.isPress()) {
    backLight();
  }
  
  if (enc1.isClick()) {
    editMode = 0;
  }
  
  if (enc1.isHolded()) {
    editMode = (++editMode) % 8;
    setupUpdate();
  }

  switch (editMode) {
    case 1:
      if (enc1.isLeft()) {
        uint8_t hour = (23 + Clock.getHour(h12, PM)) % 24;
        Clock.setHour(hour);
        setupUpdate();
      }
      if (enc1.isRight()) {
        uint8_t hour = (Clock.getHour(h12, PM) + 1) % 24;
        Clock.setHour(hour);
        setupUpdate();
      }
      break;
    case 2:
      if (enc1.isLeft()) {
        uint8_t minute = (59 + Clock.getMinute()) % 60;
        Clock.setMinute(minute);
        setupUpdate();
      }
      if (enc1.isRight()) {
        uint8_t minute = (Clock.getMinute() + 1) % 60;
        Clock.setMinute(minute);
        setupUpdate();
      }
      break;
    case 3:
      if (enc1.isLeft()) {
        uint8_t second = (59 + Clock.getSecond()) % 60;
        Clock.setSecond(second);
        setupUpdate();
      }
      if (enc1.isRight()) {
        uint8_t second = (Clock.getSecond() + 1) % 60;
        Clock.setSecond(second);
        setupUpdate();
      }
      break;
    case 4:
      if (enc1.isLeft()) {
        startHour = (23 + startHour) % 24;
        EEPROM.update(EEPROM_ADDRESS_START_HOUR, startHour);
        setupUpdate();
      }
      if (enc1.isRight()) {
        startHour = (startHour + 1) % 24;
        EEPROM.update(EEPROM_ADDRESS_START_HOUR, startHour);
        setupUpdate();
      }
      break;
    case 5:
      if (enc1.isLeft()) {
        limitMinute = (9 + limitMinute) % 10;
        EEPROM.update(EEPROM_ADDRESS_LIMIT_MINUTE, limitMinute);
        setupUpdate();
      }
      if (enc1.isRight()) {
        limitMinute = (limitMinute + 1) % 10;
        EEPROM.update(EEPROM_ADDRESS_LIMIT_MINUTE, limitMinute);
        setupUpdate();
      }
      break;
    case 6:
      if (enc1.isLeft()) {
        closeTemp = (39 + closeTemp) % 40;
        EEPROM.update(EEPROM_ADDRESS_CLOSE_TEMP, closeTemp);
        setupUpdate();
      }
      if (enc1.isRight()) {
        closeTemp = (closeTemp + 1) % 40;
        EEPROM.update(EEPROM_ADDRESS_CLOSE_TEMP, closeTemp);
        setupUpdate();
      }
      break;
    case 7:
      if (enc1.isLeft()) {
        tempTresh = (9 + tempTresh) % 10;
        EEPROM.update(EEPROM_ADDRESS_TEMP_TRESH, tempTresh);
        setupUpdate();
      }
      if (enc1.isRight()) {
        tempTresh = (tempTresh + 1) % 10;
        EEPROM.update(EEPROM_ADDRESS_TEMP_TRESH, tempTresh);
        setupUpdate();
      }
      break;
  }
  
  if (backlightTimer.isReady() && editMode == 0) {
    lcd.noBacklight();
    backlightTimer.stop();
  }

  if (analogTimer.isReady() && editMode == 0) {
    updateTime();

    // time
    uint8_t hour = Clock.getHour(h12, PM);
    uint8_t minute = Clock.getMinute();
    uint8_t second = Clock.getSecond();

    // valve
    if (hour == startHour && minute<=limitMinute) {
      digitalWrite(VALVE_PIN, HIGH);
      relayState = 1;
    } else {
      digitalWrite(VALVE_PIN, LOW);
      relayState = 0;
    }

    // door
    tempRaw = 100 * bmp.readTemperature();
    if (tempRaw > 100 * (closeTemp + tempTresh)) {
      digitalWrite(DOOR_PIN, HIGH);
      doorState = 1;
    }
    if (tempRaw < 100 * closeTemp) {
      digitalWrite(DOOR_PIN, LOW);
      doorState = 0;
    }
    tempPretty = tempRaw / 100.0;
    dtostrf(tempPretty, 2, 1, myStr);
  
    lcd.setCursor(10,0);
    lcd.print(myStr);
    lcd.printByte(0xdf); // degree sign
    lcd.print("C"); // celsium

    uint16_t uptime = millis() / 60000;

    sprintf(myStr, "%02dh%02dm    %02d/%02d", startHour, limitMinute, closeTemp, tempTresh);
    lcd.setCursor(0,1);
    lcd.print(myStr);
    lcd.printByte(0xdf); // degree sign

    modbus.setInputRegisterValue(1, uptime);
    modbus.setInputRegisterValue(2, tempRaw);
    modbus.setInputRegisterValue(3, relayState);
    modbus.setInputRegisterValue(4, doorState);
  }
}

void setDataFromModbus(uint16_t registerNumber, uint16_t value) {
  if (registerNumber == 1) {
    Clock.setHour(value % 24);
  } else if (registerNumber == 2) {
    Clock.setMinute(value % 60);
  } else if (registerNumber == 3) {
    Clock.setSecond(value % 60);
  } else if (registerNumber == 4) {
    startHour = value % 24;
    EEPROM.update(EEPROM_ADDRESS_START_HOUR, startHour);
  } else if (registerNumber == 5) {
    limitMinute = value % 24;
    EEPROM.update(EEPROM_ADDRESS_LIMIT_MINUTE, limitMinute);
  } else if (registerNumber == 6) {
    closeTemp = value % 24;
    EEPROM.update(EEPROM_ADDRESS_CLOSE_TEMP, closeTemp);
  } else if (registerNumber == 7) {
    tempTresh = value % 24;
    EEPROM.update(EEPROM_ADDRESS_TEMP_TRESH, tempTresh);
  }
}
