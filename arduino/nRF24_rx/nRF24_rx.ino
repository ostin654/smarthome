//#include "printf.h"
#include <SPI.h>
#include <RF24.h>
#include <LiquidCrystal_I2C.h>

RF24 radio(6, 7);
LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

const uint8_t pipe_out[5] = {232,205,3,145,35};
const uint8_t pipe_in1[5] = {141,205,3,145,35};
const uint8_t pipe_in2[5] = {142,205,3,145,35};
const uint8_t pipe_in3[5] = {143,205,3,145,35};
const uint8_t pipe_in4[5] = {144,205,3,145,35};
const uint8_t pipe_in5[5] = {145,205,3,145,35};

uint8_t pipeNum;
unsigned long data = 0;

void setup()   
{
  pinMode(8, OUTPUT);
  digitalWrite(8, LOW);
  pinMode(9, OUTPUT);
  digitalWrite(9, LOW);
  pinMode(10, OUTPUT);
  digitalWrite(10, LOW);
  pinMode(11, OUTPUT);
  digitalWrite(11, LOW);
  pinMode(12, OUTPUT);
  digitalWrite(12, LOW);
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);

  radio.begin();
  radio.powerUp();               // включение или пониженное потребление powerDown - powerUp

  radio.setDataRate(RF24_250KBPS); // скорость обмена данными RF24_250KBPS, RF24_1MBPS или RF24_2MBPS
  radio.setCRCLength(RF24_CRC_16); // длинна контрольной суммы 8-bit or 16-bit
  radio.setPALevel(RF24_PA_MAX); // уровень питания усилителя RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH and RF24_PA_MAX
                                 // соответствует уровням:    -18dBm,      -12dBm,      -6dBM,           0dBm
  radio.setChannel(58);         // уствновка канала
  radio.setRetries(15, 15);
  radio.setAutoAck(true);

  radio.openWritingPipe(pipe_out);
  radio.openReadingPipe(1,pipe_in1);
  radio.openReadingPipe(2,pipe_in2);
  radio.openReadingPipe(3,pipe_in3);
  radio.openReadingPipe(4,pipe_in4);
  radio.openReadingPipe(5,pipe_in5);

  //radio.printDetails();
  radio.startListening(); // приём

  lcd.init();
  lcd.backlight();
  lcd.setCursor(1,0);
  lcd.print("1:----");
  lcd.setCursor(9,0);
  lcd.print("2:----");
  lcd.setCursor(1,1);
  lcd.print("3:----");
  lcd.setCursor(9,1);
  lcd.print("4:----");
}

void loop()  
{
  if (radio.available(&pipeNum)) {
    radio.read(&data, sizeof(data));

    if (pipeNum == 1) {
      digitalWrite(8, HIGH);
      lcd.setCursor(0,0);
      lcd.print("*");
      lcd.setCursor(8,0);
      lcd.print(" ");
      lcd.setCursor(0,1);
      lcd.print(" ");
      lcd.setCursor(8,1);
      lcd.print(" ");
      lcd.setCursor(3,0);
      lcd.print(data/1000);
    }
    if (pipeNum == 2) {
      digitalWrite(9, HIGH);
      lcd.setCursor(0,0);
      lcd.print(" ");
      lcd.setCursor(8,0);
      lcd.print("*");
      lcd.setCursor(0,1);
      lcd.print(" ");
      lcd.setCursor(8,1);
      lcd.print(" ");
      lcd.setCursor(3,0);
      lcd.setCursor(11,0);
      lcd.print(data/1000);
    }
    if (pipeNum == 3) {
      digitalWrite(10, HIGH);
      lcd.setCursor(0,0);
      lcd.print(" ");
      lcd.setCursor(8,0);
      lcd.print(" ");
      lcd.setCursor(0,1);
      lcd.print("*");
      lcd.setCursor(8,1);
      lcd.print(" ");
      lcd.setCursor(3,0);
      lcd.setCursor(3,1);
      lcd.print(data/1000);
    }
    if (pipeNum == 4) {
      digitalWrite(11, HIGH);
      lcd.setCursor(0,0);
      lcd.print(" ");
      lcd.setCursor(8,0);
      lcd.print(" ");
      lcd.setCursor(0,1);
      lcd.print(" ");
      lcd.setCursor(8,1);
      lcd.print("*");
      lcd.setCursor(3,0);
      lcd.setCursor(11,1);
      lcd.print(data/1000);
    }
    if (pipeNum == 5) {
      digitalWrite(12, HIGH);
    }
  } else {
    lcd.setCursor(0,0);
    lcd.print(" ");
    lcd.setCursor(8,0);
    lcd.print(" ");
    lcd.setCursor(0,1);
    lcd.print(" ");
    lcd.setCursor(8,1);
    lcd.print(" ");
    lcd.setCursor(3,0);

    digitalWrite(8, LOW);
    digitalWrite(9, LOW);
    digitalWrite(10, LOW);
    digitalWrite(11, LOW);
    digitalWrite(12, LOW);
    digitalWrite(13, HIGH);
    delay(30);
    digitalWrite(13, LOW);
  }
  delay(100);
}
