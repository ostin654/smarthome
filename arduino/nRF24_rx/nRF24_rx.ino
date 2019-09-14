//#include "printf.h"
#include <SPI.h>
#include <RF24.h>
#include <LiquidCrystal_I2C.h>
#include <GyverTimer.h>

//#define LCD_ENABLE

RF24 radio(6, 7);
#ifdef LCD_ENABLE
LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display
#endif
GTimer_ms ledTimer1;
GTimer_ms ledTimer2;
GTimer_ms ledTimer3;
GTimer_ms ledTimer4;
#ifdef LCD_ENABLE
GTimer_ms lсdTimer1;
GTimer_ms lсdTimer2;
GTimer_ms lсdTimer3;
GTimer_ms lсdTimer4;
#endif;

const uint8_t pipe_out[5] = {232,205,3,145,35};
const uint8_t pipe_in1[5] = {141,205,3,145,35};
const uint8_t pipe_in2[5] = {142,205,3,145,35};
const uint8_t pipe_in3[5] = {143,205,3,145,35};
const uint8_t pipe_in4[5] = {144,205,3,145,35};

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

  radio.begin();
  radio.powerUp();               // включение или пониженное потребление powerDown - powerUp

  radio.setDataRate(RF24_250KBPS); // скорость обмена данными RF24_250KBPS, RF24_1MBPS или RF24_2MBPS
  radio.setCRCLength(RF24_CRC_16); // длинна контрольной суммы 8-bit or 16-bit
  radio.setPALevel(RF24_PA_MAX); // уровень питания усилителя RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH and RF24_PA_MAX
                                 // соответствует уровням:    -18dBm,      -12dBm,      -6dBM,           0dBm
  radio.setChannel(90);         // уствновка канала
  radio.setRetries(15, 15);
  radio.setAutoAck(true);

  radio.openWritingPipe(pipe_out);
  radio.openReadingPipe(1,pipe_in1);
  radio.openReadingPipe(2,pipe_in2);
  radio.openReadingPipe(3,pipe_in3);
  radio.openReadingPipe(4,pipe_in4);

  //radio.printDetails();
  radio.startListening(); // приём

  #ifdef LCD_ENABLE
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
  #endif

  ledTimer1.setInterval(2000);
  ledTimer1.setMode(MANUAL);
  ledTimer1.reset();
  ledTimer2.setInterval(2000);
  ledTimer2.setMode(MANUAL);
  ledTimer2.reset();
  ledTimer3.setInterval(2000);
  ledTimer3.setMode(MANUAL);
  ledTimer3.reset();
  ledTimer4.setInterval(2000);
  ledTimer4.setMode(MANUAL);
  ledTimer4.reset();

  #ifdef LCD_ENABLE
  lcdTimer1.setInterval(60000);
  lcdTimer1.setMode(MANUAL);
  lcdTimer1.reset();
  lcdTimer2.setInterval(60000);
  lcdTimer2.setMode(MANUAL);
  lcdTimer2.reset();
  lcdTimer3.setInterval(60000);
  lcdTimer3.setMode(MANUAL);
  lcdTimer3.reset();
  lcdTimer4.setInterval(60000);
  lcdTimer4.setMode(MANUAL);
  lcdTimer4.reset();
  #endif
}

void loop()  
{
  if (ledTimer1.isReady()) {
    digitalWrite(8, LOW);
    #ifdef LCD_ENABLE
    lcd.setCursor(0,0);
    lcd.print(" ");
    #endif
  }

  #ifdef LCD_ENABLE
  if (lcdTimer1.isReady()) {
    lcd.setCursor(1,0);
    lcd.print("1:----");
  }
  #endif

  if (ledTimer2.isReady()) {
    digitalWrite(9, LOW);
    #ifdef LCD_ENABLE
    lcd.setCursor(8,0);
    lcd.print(" ");
    #endif
  }

  #ifdef LCD_ENABLE
  if (lcdTimer2.isReady()) {
    lcd.setCursor(9,0);
    lcd.print("2:----");
  }
  #endif

  if (ledTimer3.isReady()) {
    digitalWrite(10, LOW);
    #ifdef LCD_ENABLE
    lcd.setCursor(0,1);
    lcd.print(" ");
    #endif
  }

  #ifdef LCD_ENABLE
  if (lcdTimer3.isReady()) {
    lcd.setCursor(1,1);
    lcd.print("3:----");
  }
  #endif

  if (ledTimer4.isReady()) {
    digitalWrite(11, LOW);
    #ifdef LCD_ENABLE
    lcd.setCursor(8,1);
    lcd.print(" ");
    #endif
  }

  #ifdef LCD_ENABLE
  if (lcdTimer4.isReady()) {
    lcd.setCursor(9,1);
    lcd.print("4:----");
  }
  #endif

  if (radio.available(&pipeNum)) {
    radio.read(&data, sizeof(data));

    if (pipeNum == 1) {
      digitalWrite(8, HIGH);
      ledTimer1.reset();
      
      #ifdef LCD_ENABLE
      lcdTimer1.reset();
      
      lcd.setCursor(0,0);
      lcd.print("*");
      lcd.setCursor(3,0);
      lcd.print(data/1000);
      #endif
    }
    if (pipeNum == 2) {
      digitalWrite(9, HIGH);
      ledTimer2.reset();
      
      #ifdef LCD_ENABLE
      lcdTimer2.reset();
      
      lcd.setCursor(8,0);
      lcd.print("*");
      lcd.setCursor(11,0);
      lcd.print(data/1000);
      #endif
    }
    if (pipeNum == 3) {
      digitalWrite(10, HIGH);
      ledTimer3.reset();
      
      #ifdef LCD_ENABLE
      lcdTimer3.reset();
      
      lcd.setCursor(0,1);
      lcd.print("*");
      lcd.setCursor(3,1);
      lcd.print(data/1000);
      #endif
    }
    if (pipeNum == 4) {
      digitalWrite(11, HIGH);
      ledTimer4.reset();
      
      #ifdef LCD_ENABLE
      lcdTimer4.reset();
      
      lcd.setCursor(8,1);
      lcd.print("*");
      lcd.setCursor(11,1);
      lcd.print(data/1000);
      #endif
    }
  }
}
