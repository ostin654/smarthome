#include <Adafruit_BME280.h>
#include <RH_ASK.h>
#include <GyverPower.h>

#ifndef LED_BUILTIN
#define LED_BUILTIN 0
#endif

#define BMP_ENABLE
#define MCU_SLEEP
#define DELAY 1
#define ADDRESS 0x37

RH_ASK driver(2000, 0, 2, 0);
bool bmxStatus = false;
uint8_t buf[4];

#ifdef BMP_ENABLE
Adafruit_BME280 bmx;
#endif


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
  delay(500);

  #ifdef MCU_SLEEP
  //autoCalibrate();
  //setSystemPrescaler(PRESCALER_2);
  setSleepMode(POWERDOWN_SLEEP);
  bodInSleep(false);
  hardwareDisable(PWR_ADC | PWR_USB | PWR_UART0 | PWR_SPI | PWR_TIMER2);// | PWR_TIMER1);
  #endif

  
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(50);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);

}

void loop()   
{
  if (bmxStatus) {
    uint32_t temperature = 0;
    #ifdef BMP_ENABLE
    temperature = (uint32_t) (bmx.readTemperature() * 100);
    #else
    temperature = (uint32_t) (millis() % 1000);
    #endif

    buf[0] = 0;
    buf[1] = ADDRESS;
    buf[2] = 0;
    buf[2] |= temperature;
    buf[3] = 0;
    buf[3] |= temperature >> 8;

    #ifndef MCU_SLEEP
    Serial.print(buf[0], HEX);
    Serial.print(" ");
    Serial.print(buf[1], HEX);
    Serial.print(" ");
    Serial.print(buf[2], HEX);
    Serial.print(" ");
    Serial.println(buf[3], HEX);
    #endif
    
    for (byte i=0;i<5;i++) {
      driver.send((const uint8_t *)&buf, 4);
      driver.waitPacketSent();
    }
  }

  digitalWrite(LED_BUILTIN, HIGH);
  delay(50);
  digitalWrite(LED_BUILTIN, LOW);
  
  #ifdef MCU_SLEEP
  sleep(SLEEP_8192MS);
  sleep(SLEEP_8192MS);
  sleep(SLEEP_4096MS);
  sleep(SLEEP_2048MS);
  sleep(SLEEP_512MS);
  #else
  delay(2000);
  #endif
}
