#include <GyverTimer.h>
#include <GyverFilters.h>
#include <SoftwareSerial.h>

#define PRESSURE_PIN A1
#define RX_PIN 4
#define TX_PIN 6

GTimer_ms displayTimer;
GKalman myFilter(200, 10, 0.1);
SoftwareSerial softSerial(RX_PIN, TX_PIN);

struct DataPacket {
  uint32_t my_uptime = 0; // 0x99
  uint32_t pressure = 0;  // 0xa2
};

DataPacket packet;

void setup()
{
  softSerial.begin(9600);
  analogReference(EXTERNAL);
  displayTimer.setInterval(1000);
}
 
void loop()
{
  if (displayTimer.isReady()) {
    packet.my_uptime = millis() / 60000;
    packet.pressure = myFilter.filtered(analogRead(PRESSURE_PIN));

    softSerial.print("{ \"uptime\": ");
    softSerial.print(packet.my_uptime);
    softSerial.print(", \"pressure\": ");
    softSerial.print(packet.pressure);
    softSerial.println(" }");
  }
}
