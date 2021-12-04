#include <Adafruit_BME280.h>
#include <RH_ASK.h>
#include <GyverPower.h>

#ifndef LED_BUILTIN
#define LED_BUILTIN 0
#endif

#define BMP_ENABLE
#define MCU_SLEEP
#define DELAY 1
//#define ADDRESS 0x28
#define ADDRESS 0x37
#define TEST_ADDRESS1 0x28
#define TEST_ADDRESS2 0x37
#define TEST_ADDRESS3 0x51
#define RF_PIN 2

RH_ASK driver(2000, 0, RF_PIN, 0);
bool bmxStatus = false;
uint8_t buf[6];

#ifdef BMP_ENABLE
Adafruit_BME280 bmx;
#endif

uint16_t getAddress() {
  #ifdef ADDRESS
  return ADDRESS;
  #else
  switch (random(100) % 3) {
    case 0:
      return TEST_ADDRESS1;
      break;
    case 1:
      return TEST_ADDRESS2;
      break;
    case 2:
      return TEST_ADDRESS3;
      break;
  }
  #endif
}

uint16_t calculateCRC(uint8_t u8length) {
  unsigned int temp, temp2, flag;
  temp = 0xFFFF;
  for (uint8_t i = 0; i < u8length; i++) {
    temp = temp ^ buf[i];
    for (uint8_t j = 1; j <= 8; j++) {
      flag = temp & 0x0001;
      temp >>=1;
      if (flag) {
        temp ^= 0xA001;
      }
    }
  }
  // Reverse byte order.
  temp2 = temp >> 8;
  temp = (temp << 8) | temp2;
  temp &= 0xFFFF;
  // the returned value is already swapped
  // crcLo byte is first & crcHi byte is last
  return temp;
}

void setup()
{
  #ifndef MCU_SLEEP
  Serial.begin(9600);
  while(!Serial);
  #endif
  
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  
  #ifdef BMP_ENABLE
  bmxStatus = bmx.begin(0x76);
  #else
  bmxStatus = true;
  #endif

  if (bmxStatus) {
    driver.init();

    #ifdef BMP_ENABLE
    bmx.setSampling(Adafruit_BME280::MODE_NORMAL,
                    Adafruit_BME280::SAMPLING_X2,
                    Adafruit_BME280::SAMPLING_NONE,
                    Adafruit_BME280::SAMPLING_NONE,
                    Adafruit_BME280::FILTER_OFF,
                    Adafruit_BME280::STANDBY_MS_1000);
    #endif
  } else {
    while (1) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
      digitalWrite(LED_BUILTIN, LOW);
      delay(100);
    }
  }

  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);

  #ifdef MCU_SLEEP
  //autoCalibrate();
  //setSystemPrescaler(PRESCALER_2);
  setSleepMode(POWERDOWN_SLEEP);
  bodInSleep(false);
  hardwareDisable(PWR_ADC | PWR_USB | PWR_UART0 | PWR_SPI | PWR_TIMER2);// | PWR_TIMER1);
  #endif
}

void loop()   
{
  if (bmxStatus) {
    uint16_t address = getAddress();

    buf[0] = 0;
    buf[0] |= address;
    buf[1] = 0;
    buf[1] |= address >> 8;

    uint32_t temperature = 0;
    #ifdef BMP_ENABLE
    temperature = (uint32_t) (bmx.readTemperature() * 100);
    #else
    temperature = (uint32_t) (millis() % 1000);
    #endif

    buf[2] = 0;
    buf[2] |= temperature;
    buf[3] = 0;
    buf[3] |= temperature >> 8;

    uint16_t crc = calculateCRC(4);
    buf[4] = 0;
    buf[4] |= crc;
    buf[5] = 0;
    buf[5] = crc >> 8;

    #ifndef MCU_SLEEP
    Serial.print(buf[0], HEX);
    Serial.print(" ");
    Serial.print(buf[1], HEX);
    Serial.print(" ");
    Serial.print(buf[2], HEX);
    Serial.print(" ");
    Serial.print(buf[3], HEX);
    Serial.print(" ");
    Serial.print(buf[4], HEX);
    Serial.print(" ");
    Serial.print(buf[5], HEX);
    Serial.println();
    #endif

    for (byte i=0;i<2;i++) {
      driver.send((const uint8_t *)&buf, 6);
      driver.waitPacketSent();
    }
  }

  #ifndef MCU_SLEEP
  digitalWrite(LED_BUILTIN, HIGH);
  delay(10);
  digitalWrite(LED_BUILTIN, LOW);
  #endif
  
  #ifdef MCU_SLEEP
  sleep(SLEEP_8192MS);
  sleep(SLEEP_8192MS);
  sleep(SLEEP_4096MS);
  sleep(SLEEP_2048MS);
  sleep(SLEEP_2048MS);
  //sleep(SLEEP_512MS);
  #else
  delay(2000);
  #endif
}
