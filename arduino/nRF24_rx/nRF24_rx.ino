//#include "printf.h"
#include <SPI.h>
#include <RF24.h>

RF24 radio(6, 7);

const uint8_t pipe_out[5] = {232,205,3,145,35};
const uint8_t pipe_in[5] = {141,205,3,145,35};

uint8_t pipeNum;
unsigned long data = 0;

void setup()   
{
  //Serial.begin(9600);
  //while (!Serial);
  //printf_begin();
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
  radio.openReadingPipe(1,pipe_in);

  //radio.printDetails();
  radio.startListening(); // приём
}

void loop()  
{
  if (radio.available(&pipeNum)) {
    radio.read(&data, sizeof(data));
    if (pipeNum == 1) {
      digitalWrite(8, HIGH);
    }
    //Serial.print(data);
    //Serial.print(" => ");
    //Serial.println(pipeNum);
  } else {
    digitalWrite(8, LOW);
    digitalWrite(9, LOW);
    digitalWrite(10, LOW);
    digitalWrite(11, LOW);
    digitalWrite(12, LOW);
    digitalWrite(13, HIGH);
    delay(30);
    digitalWrite(13, LOW);
    //Serial.println("No data");
  }
  delay(100);
}
