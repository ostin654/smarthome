#include <SPI.h>
#include <RF24.h>
#include <Adafruit_BME280.h>
#include <Adafruit_BMP280.h>

#define BMP_ENABLE

RF24 radio(6, 7);
#ifdef BMP_ENABLE
Adafruit_BMP280 bmx;
#endif

const uint8_t pipe_in[5] = {232,205,3,145,35};
const uint8_t pipe_out[5] = {141,205,3,145,35};

bool bmxStatus = false;
unsigned long data = 0;

void setup()
{
  Serial.begin(9600);
  #ifdef BMP_ENABLE
  bmxStatus = bmx.begin(0x76);
  #else
  bmxStatus = true;
  #endif

  if (bmxStatus) {
    radio.begin();
    radio.powerUp();

    radio.setDataRate(RF24_250KBPS); // скорость обмена данными RF24_250KBPS, RF24_1MBPS или RF24_2MBPS
    radio.setCRCLength(RF24_CRC_16); // длинна контрольной суммы 8-bit or 16-bit
    radio.setPALevel(RF24_PA_MAX); // уровень питания усилителя RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH and RF24_PA_MAX
                                 // соответствует уровням:    -18dBm,      -12dBm,      -6dBM,           0dBm
    radio.setChannel(90);         // установка канала
    radio.setRetries(15, 15);
    radio.setAutoAck(true);

    radio.openWritingPipe(pipe_out);   // открыть канал на отправку
    radio.openReadingPipe(1,pipe_in);
  } else {
    while (1) {
      Serial.println("BME Not working");
      delay(300);
    }
  }
}

void loop()   
{
  if (bmxStatus) {
    #ifdef BMP_ENABLE
    data = (unsigned long) (bmx.readTemperature() * 1000);
    #else
    data = (unsigned long) (millis() % 10000);
    #endif
    if (radio.write(&data, sizeof(data))) {
    } else {
      Serial.println("Not sent");
    }
  }
  delay(50);
}
