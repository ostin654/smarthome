#include <SPI.h>
#include <RF24.h>
#include <Adafruit_BME280.h>

RF24 radio(6, 7);
Adafruit_BME280 bme;

const uint8_t pipe_in[5] = {232,205,3,145,35};
const uint8_t pipe_out[5] = {141,205,3,145,35};

bool bmeStatus = false;
unsigned long data = 0;

void setup()
{
  bmeStatus = true; //bme.begin();

  if (bmeStatus) {
    radio.begin();
    radio.powerUp();

    radio.setDataRate(RF24_250KBPS); // скорость обмена данными RF24_250KBPS, RF24_1MBPS или RF24_2MBPS
    radio.setCRCLength(RF24_CRC_16); // длинна контрольной суммы 8-bit or 16-bit
    radio.setPALevel(RF24_PA_MAX); // уровень питания усилителя RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH and RF24_PA_MAX
                                 // соответствует уровням:    -18dBm,      -12dBm,      -6dBM,           0dBm
    radio.setChannel(58);         // установка канала
    radio.setRetries(15, 15);
    radio.setAutoAck(true);

    radio.openWritingPipe(pipe_out);   // открыть канал на отправку
    radio.openReadingPipe(1,pipe_in);
  } else {
    pinMode(13, OUTPUT);
    while (1) {
      digitalWrite(13, HIGH);
      delay(300);
      digitalWrite(13, LOW);
      delay(300);
    }
  }
}

void loop()   
{
  if (bmeStatus) {
    data = (unsigned long) 20; //(bme.readTemperature() * 1000);
    if (radio.write(&data, sizeof(data))) {
    } else {
    }
  }
  delay(100);
}
