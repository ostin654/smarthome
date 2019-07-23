#include <TinyWire.h>
#include <GyverTimer.h>
#include <GyverFilters.h>

#define PRESSURE_PIN A1

#define I2C_SLAVE_ADDRESS 0x19

#define MY_UPTIME             0x99
#define REGISTER_PRESSURE     0xa2

GTimer_ms readTimer;
GKalman myFilter(200, 10, 0.1);

struct DataPacket {
  uint32_t my_uptime = 0; // 0x99
  uint32_t pressure = 0;  // 0xa2
};

DataPacket packet;

// i2c
uint8_t reg;
uint8_t req = 0;
uint32_t to_send = 0;

void setup()
{  
  analogReference(EXTERNAL);

  TinyWire.begin(I2C_SLAVE_ADDRESS);
  delay(1000);
  TinyWire.onReceive(processMessage);
  TinyWire.onRequest(requestEvent);

  readTimer.setInterval(1000);
}
 
void loop()
{
  if (readTimer.isReady()) {
    packet.my_uptime = millis() / 60000;
    packet.pressure = myFilter.filtered(analogRead(PRESSURE_PIN));
    //packet.pressure = analogRead(PRESSURE_PIN);
  }
}

void processMessage(int n) {
  uint8_t data = 0;
  data = TinyWire.read();

  switch (data) {
    case MY_UPTIME:             //0x99
    case REGISTER_PRESSURE:     //0xa2
      reg = data;
      break;
    case 0xff: // exclude i2cdetect
      break;
  }

  req = 0;
}

void requestEvent() {
  if (req == 0) {
    switch (reg) {
      case MY_UPTIME:
        to_send = packet.my_uptime;
        break;
      case REGISTER_PRESSURE:
        to_send = packet.pressure;
        break;
    }
  }

  uint8_t b = 0;

  TinyWire.write((uint8_t)(to_send | b));
  to_send = to_send >> 8;
  req = (req + 1) % 4;
}
