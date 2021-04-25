//#define MODBUS_DEBUG

#include <Wire.h>
#include "Adafruit_BMP280.h"
#include <LiquidCrystal_I2C.h>
#include <DS3231.h>
#include <GyverEncoder.h>
#include <GyverTimer.h>
#include <ModbusKostin.h>
#include <SoftwareSerial.h>

#if defined(ARDUINO) && ARDUINO >= 100
#define printByte(args)  write(args);
#else
#define printByte(args)  print(args,BYTE);
#endif

#define DOOR_PIN 8
#define VALVE_PIN 9

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

void setup() {
  if (!bmp.begin(0x76)) {
    while (1);
  }

  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                  Adafruit_BMP280::SAMPLING_X2,
                  Adafruit_BMP280::SAMPLING_NONE,
                  Adafruit_BMP280::FILTER_X2,
                  Adafruit_BMP280::STANDBY_MS_1000);

  analogTimer.setInterval(1000);
  backlightTimer.setInterval(9000);
  enc1.setType(TYPE2);
  lcd.init();
  lcd.backlight();

  modbus.begin(38400);
  modbus.setInputRegisterLimits(1, 4);
  modbus.setOutputRegisterLimits(1, 3);
  modbus.setInputRegisterValue(1, 0);
  modbus.setInputRegisterValue(2, 0);
  modbus.setInputRegisterValue(3, 0);
  modbus.setInputRegisterValue(4, 0);
  modbus.setCallbackFunc(setTimeFromModbus);

  pinMode(VALVE_PIN, OUTPUT); // valve
  digitalWrite(VALVE_PIN, LOW);
  pinMode(DOOR_PIN, OUTPUT); // door
  digitalWrite(DOOR_PIN, LOW);
}

void updateTime() {
  sprintf(myStr, "%02d:%02d:%02d", Clock.getHour(h12, PM), Clock.getMinute(), Clock.getSecond());
  lcd.setCursor(0,0);
  lcd.print(myStr);
}

void loop() {
  int16_t tempRaw;
  float tempPretty;
  static uint16_t relayState = 0;
  static uint16_t doorState = 0;

  enc1.tick();
  modbus.poll();

  if (enc1.isPress()) {
    lcd.backlight();
    backlightTimer.reset();
    backlightTimer.start();
  }
  if (enc1.isRight()) {
    Clock.setHour((Clock.getHour(h12, PM)+1)%24);
    updateTime();
    lcd.backlight();
    backlightTimer.reset();
    backlightTimer.start();
  }
  if (enc1.isLeft()) {
    int8_t hour = Clock.getHour(h12, PM);
    if (hour == 0) hour = 23;
    else hour = (hour - 1)%24;
    Clock.setHour(hour);
    updateTime();
    lcd.backlight();
    backlightTimer.reset();
    backlightTimer.start();
  }
  if (enc1.isRightH()) {
    Clock.setMinute((Clock.getMinute()+1)%60);
    Clock.setSecond(0);
    updateTime();
    lcd.backlight();
    backlightTimer.reset();
    backlightTimer.start();
  }
  if (enc1.isLeftH()) {
    int8_t minute = Clock.getMinute();
    if (minute <= 0) minute = 59;
    else minute = (minute - 1)%60;
    Clock.setMinute(minute);
    Clock.setSecond(0);
    updateTime();
    lcd.backlight();
    backlightTimer.reset();
    backlightTimer.start();
  }
  if (backlightTimer.isReady()) {
    lcd.noBacklight();
    backlightTimer.stop();
  }

  if (analogTimer.isReady()) {
    updateTime();

    // time
    uint8_t hour = Clock.getHour(h12, PM);
    uint8_t minute = Clock.getMinute();
    uint8_t second = Clock.getSecond();

    // valve
    if (hour == 21 && minute<=4) {
      digitalWrite(VALVE_PIN, HIGH);
      relayState = 1;
    } else {
      digitalWrite(VALVE_PIN, LOW);
      relayState = 0;
    }

    // door
    tempRaw = 100 * bmp.readTemperature();
    if (tempRaw > 3000) {
      digitalWrite(DOOR_PIN, HIGH);
      doorState = 1;
    }
    if (tempRaw < 2500) {
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

    sprintf(myStr, "%02X V%dD%d UP%d", modbus.getAddress(), relayState, doorState, uptime);
    lcd.setCursor(0,1);
    lcd.print(myStr);
    
    modbus.setInputRegisterValue(1, uptime);
    modbus.setInputRegisterValue(2, tempRaw);
    modbus.setInputRegisterValue(3, relayState);
    modbus.setInputRegisterValue(4, doorState);
  }
}

void setTimeFromModbus(uint16_t registerNumber, uint16_t value) {
  if (registerNumber == 1) {
    Clock.setHour(value%24);
  } else if (registerNumber == 2) {
    Clock.setMinute(value%60);
  } else if (registerNumber == 3) {
    Clock.setSecond(value%60);
  }
}
