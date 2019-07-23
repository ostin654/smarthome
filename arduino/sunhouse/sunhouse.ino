#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <LiquidCrystal_I2C.h>
#include <DS3231.h>
#include <GyverEncoder.h>
#include <GyverTimer.h>
#include <GyverFilters.h>
#include <ModbusKostin.h>
#include <SoftwareSerial.h>

#if defined(ARDUINO) && ARDUINO >= 100
#define printByte(args)  write(args);
#else
#define printByte(args)  print(args,BYTE);
#endif

#define SOIL_POWER 3
#define DOOR_PIN 8
#define VALVE_PIN 9

Adafruit_BMP280 bmp; // I2C
LiquidCrystal_I2C lcd(0x27,16,2);
Encoder enc1(7, 6, 5);
GTimer_ms analogTimer;
GTimer_ms backlightTimer;
GKalman soilFilter(500, 10, 0.1);
SoftwareSerial mySerial(A0, A1);
ModbusKostin modbus(0x0B, &mySerial, 2); // address, serial, expin
DS3231 Clock;

bool Century;
bool h12;
bool PM;

void setup() {
  analogReadResolution(10);
  if (!bmp.begin(0x76)) {
    while (1);
  }
  analogTimer.setInterval(1000);
  backlightTimer.setInterval(9000);
  enc1.setType(TYPE2);
  lcd.init();
  lcd.backlight();

  modbus.begin(38400);
  modbus.setRegisterLimits(1, 6);
  modbus.setRegisterValue(1, 0);
  modbus.setRegisterValue(2, 0);
  modbus.setRegisterValue(3, 0);
  modbus.setRegisterValue(4, 0);
  modbus.setRegisterValue(5, 0);
  modbus.setRegisterValue(6, 0);

  pinMode(VALVE_PIN, OUTPUT); // valve
  digitalWrite(VALVE_PIN, LOW);
  pinMode(DOOR_PIN, OUTPUT); // door
  digitalWrite(DOOR_PIN, LOW);
  pinMode(SOIL_POWER, OUTPUT); // soil power
  digitalWrite(SOIL_POWER, LOW);
}

void updateTime() {
  char myStr[10];

  sprintf(myStr, "%02d:%02d D%d", Clock.getHour(h12, PM), Clock.getMinute(), Clock.getDoW());
  lcd.setCursor(0,0);
  lcd.print(myStr);
}

void loop() {
  uint16_t pressureRaw;
  uint16_t soilRaw;
  uint16_t tempRaw;
  float tempPretty;
  float pressurePretty;
  char myStr[10];
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

    // valve
    uint8_t hour = Clock.getHour(h12, PM);
    uint8_t minute = Clock.getMinute();
    uint8_t dow = Clock.getDoW();
    if (hour == 21 && minute>=0 && minute<=4) {
      digitalWrite(VALVE_PIN, HIGH);
      relayState = 1;
    } else {
      digitalWrite(VALVE_PIN, LOW);
      relayState = 0;
    }

    // door
    tempRaw = 1000 * bmp.readTemperature();
    if (tempRaw > 30000) {
      digitalWrite(DOOR_PIN, HIGH);
      doorState = 1;
    }
    if (tempRaw < 25000) {
      digitalWrite(DOOR_PIN, LOW);
      doorState = 0;
    }
    tempPretty = tempRaw / 1000.0;
    dtostrf(tempPretty, 2, 1, myStr);
  
    lcd.setCursor(10,0);
    lcd.print(myStr);
    lcd.printByte(0xdf); // degree sign
    lcd.print("C"); // celsium

    digitalWrite(SOIL_POWER, HIGH);
    delay(10);
    soilRaw = soilFilter.filtered(analogRead(A3));
    digitalWrite(SOIL_POWER, LOW);
    sprintf(myStr, "%04d", soilRaw);
    lcd.setCursor(0,1);
    lcd.print("SO ");
    lcd.print(myStr);
  
    pressureRaw = analogRead(A2);
    pressurePretty = 0.0146628*pressureRaw-1.5;
    dtostrf(pressurePretty, 3, 1, myStr);
  
    lcd.setCursor(8,1);
    lcd.print("PR ");
    lcd.print(myStr);
    lcd.print("b");

    modbus.setRegisterValue(1, millis() / 60000);
    modbus.setRegisterValue(2, soilRaw);
    modbus.setRegisterValue(3, pressureRaw);
    modbus.setRegisterValue(4, tempRaw);
    modbus.setRegisterValue(5, relayState);
    modbus.setRegisterValue(6, doorState);
  }
}
