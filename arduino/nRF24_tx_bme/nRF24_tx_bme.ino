#include <SPI.h>
#include <RF24.h>
#include <Adafruit_BME280.h>
#include <Adafruit_BMP280.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

//#define BMP_ENABLE
//#define MCU_SLEEP
#define RADIO_SLEEP
#define DELAY 2

RF24 radio(9, 10);
#ifdef BMP_ENABLE
Adafruit_BMP280 bmx;
#endif

const uint8_t pipe_in[5] = {232,205,3,145,35};
const uint8_t pipe_out[5] = {141,205,3,145,35};

bool bmxStatus = false;
unsigned long data = 0;


void setup()
{
  delay(1000);

  Serial.begin(9600);
  #ifdef BMP_ENABLE
  bmxStatus = bmx.begin(0x76);
  #else
  bmxStatus = true;
  #endif

  if (bmxStatus) {
    radio.begin();

    radio.setDataRate(RF24_250KBPS); // скорость обмена данными RF24_250KBPS, RF24_1MBPS или RF24_2MBPS
    radio.setCRCLength(RF24_CRC_16); // длинна контрольной суммы 8-bit or 16-bit
    radio.setPALevel(RF24_PA_MAX); // уровень питания усилителя RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH and RF24_PA_MAX
                                 // соответствует уровням:    -18dBm,      -12dBm,      -6dBM,           0dBm
    radio.setChannel(90);         // установка канала
    radio.setRetries(15, 15);
    radio.setAutoAck(true);

    radio.openWritingPipe(pipe_out);   // открыть канал на отправку
    radio.stopListening();
    //radio.openReadingPipe(1,pipe_in);

    #ifdef BMP_ENABLE
    bmx.setSampling(Adafruit_BMP280::MODE_NORMAL,
                    Adafruit_BMP280::SAMPLING_X2,
                    Adafruit_BMP280::SAMPLING_NONE,
                    Adafruit_BMP280::FILTER_OFF,
                    Adafruit_BMP280::STANDBY_MS_2000);
    #endif
  } else {
    while (1) {
      Serial.println("BME Not working");
      delay(300);
    }
  }
}

void loop()   
{
  static byte iteration = 0;
  iteration = (iteration + 1) % DELAY;
  if (bmxStatus && iteration==0) {
    #ifdef BMP_ENABLE
    data = (unsigned long) (bmx.readTemperature() * 1000);
    #else
    data = (unsigned long) (millis() % 10000);
    #endif

    #ifdef RADIO_SLEEP
    radio.powerUp();
    delay(20);
    #endif
    if (radio.write(&data, sizeof(data))) {
    } else {
      Serial.println("Not sent");
    }
    #ifdef RADIO_SLEEP
    radio.powerDown();
    delay(20);
    #endif
  }
  #ifdef MCU_SLEEP
  goToSleep();
  #else
  delay(300);
  #endif
}

// watchdog interrupt
ISR (WDT_vect)
{
  wdt_disable(); // disable watchdog
}
 
#ifdef MCU_SLEEP
void goToSleep ()
{
  // disable ADC
  ADCSRA = 0;
  // clear various "reset" flags
  MCUSR = 0;
  // allow changes, disable reset
  WDTCSR = bit (WDCE) | bit (WDE);
  // set interrupt mode and an interval
  WDTCSR = bit (WDIE) | bit (WDP3) | bit (WDP0); // set WDIE, and 8 seconds delay
  wdt_reset(); // reset the watchdog
  set_sleep_mode (SLEEP_MODE_PWR_DOWN);
  //power_adc_disable();
  //power_usart0_disable();
  //power_timer0_disable();
  //power_timer1_disable();
  //power_timer2_disable();
  noInterrupts (); // timed sequence follows
  sleep_enable();
  // turn off brown‐out enable in software
  MCUCR = bit (BODS) | bit (BODSE);
  MCUCR = bit (BODS);
  interrupts (); // guarantees next instruction executed
  sleep_cpu ();
  // cancel sleep as a precaution
  sleep_disable();
}
#endif
