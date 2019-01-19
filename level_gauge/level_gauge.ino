#include <SPI.h>
#include <RF24.h>
#include <QuadDisplay2.h>

// создаём объект класса QuadDisplay и передаём номера пинов CS, DI и ⎍
QuadDisplay qd(10, 9, 8);

RF24 radio(1, 2);

int gaugePin = A0;

const uint32_t pipe_out = 555555550;
const uint32_t pipe_in = 555555555;

typedef byte request_packet;
typedef struct {
  float water_level;
} response_packet_well;

request_packet request = 0;
response_packet_well response_well;

void setup()   
{
  qd.begin();
  qd.displayClear();
  delay(100);
  qd.displayDigits(QD_L, QD_o, QD_A, QD_d); // Load

  radio.begin();

  delay(2000);

  radio.setDataRate(RF24_1MBPS);
  radio.setCRCLength(RF24_CRC_16);
  radio.setPALevel(RF24_PA_MAX);

  radio.setChannel(102);
  radio.setRetries(15, 15);

  radio.setAutoAck(true);
  radio.enableAckPayload();
  radio.enableDynamicPayloads();

  radio.powerUp();

  delay(2000);

  radio.openWritingPipe(pipe_out);
  radio.openReadingPipe(1,pipe_in);

  delay(1000);

  radio.startListening();
}

void loop()  
{
  // get level gauge data
  response_well.water_level = analogRead(gaugePin) * 7.0 / 1023.0;

  qd.displayFloat(response_well.water_level, 2);
  radio.writeAckPayload(1, &response_well, sizeof(response_packet_well));
  delay(100);

  if(radio.available()) {
    radio.read(&request, sizeof(request_packet));
    flash();
  }
}

void flash()
{
    digitalWrite(13, HIGH);
    delay(50);
    digitalWrite(13, LOW);
    delay(50);
    digitalWrite(13, HIGH);
    delay(50);
    digitalWrite(13, LOW);
}
