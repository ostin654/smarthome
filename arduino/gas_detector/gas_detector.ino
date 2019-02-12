#include <Wire.h>
#include <TroykaMQ.h>

#define PIN_MQ5         A0
#define PIN_MQ5_HEATER  13

#define SERIAL_PIN 5

#define PRESSURE_PIN A2

#define GAS_FAILURE_PIN 9
#define GAS_FAILURE_LIMIT 300

#define SLAVE_ADDRESS 0x18

#define REGISTER_LPG 0xa0
#define REGISTER_METHANE 0xb1
#define REGISTER_PRESSURE 0xc2
#define REGISTER_WATER_LEVEL 0xd3

//#define DEBUG

MQ5 mq5(PIN_MQ5, PIN_MQ5_HEATER);

struct DataPacket {
  unsigned long lpg = 0; // ppm 0xa0
  unsigned long methane = 0; // ppm 0xb1
  unsigned long pressure = 0; // pascal 0xc2
  unsigned long water_level = 0; // meter 0xd3
};

DataPacket packet;

byte reg;
byte req = 0;
unsigned long to_send = 0;

void setup()
{
  pinMode(SERIAL_PIN, OUTPUT);
  digitalWrite(SERIAL_PIN, LOW);

  pinMode(GAS_FAILURE_PIN, OUTPUT);
  digitalWrite(GAS_FAILURE_PIN, LOW);

  Wire.begin(SLAVE_ADDRESS);
  delay(1000);
  Wire.onReceive(processMessage);
  Wire.onRequest(requestEvent);

  #ifdef DEBUG
  Serial.begin(9600);
  while (!Serial);
  #endif

  Serial1.begin(9600);
  while (!Serial1);

  mq5.heaterPwrHigh();
}
 
void loop()
{
  if (!mq5.isCalibrated() && mq5.heatingCompleted()) {
    mq5.calibrate();
  }

  if (mq5.isCalibrated() && mq5.heatingCompleted()) {
    packet.lpg = mq5.readLPG();
    packet.methane = mq5.readMethane();
  }

  if (packet.lpg > GAS_FAILURE_LIMIT || packet.methane > GAS_FAILURE_LIMIT) {
    digitalWrite(GAS_FAILURE_PIN, HIGH);
  }

  packet.pressure = analogRead(PRESSURE_PIN);

  if (Serial1.available() > 0) {
    packet.water_level = Serial1.parseInt();
  } else {
    #ifdef DEBUG
    Serial.println("No 485 data");
    #endif
  }

  #ifdef DEBUG
  Serial.print(packet.lpg);
  Serial.print(" ");
  Serial.print(packet.methane);
  Serial.print(" ");
  Serial.print(packet.pressure);
  Serial.print(" ");
  Serial.print(packet.water_level);
  Serial.println();
  #endif

  delay(100);
}

void processMessage(int n) {
  reg = Wire.read();
  if (reg != REGISTER_LPG && reg != REGISTER_METHANE && reg != REGISTER_PRESSURE && reg != REGISTER_WATER_LEVEL) {
    reg = REGISTER_LPG;
  }
  #ifdef DEBUG
  Serial.println("i2c process");
  #endif
}

void requestEvent() {
  #ifdef DEBUG
  Serial.print("i2c request ");
  Serial.print(reg);
  Serial.print(" ");
  Serial.print(req);
  #endif

  if (req == 0) {
    if (reg == REGISTER_LPG) {
      to_send = packet.lpg;
    }
    else if (reg == REGISTER_METHANE) {
      to_send = packet.methane;
    }
    else if (reg == REGISTER_PRESSURE) {
      to_send = packet.pressure;
    }
    else if (reg == REGISTER_WATER_LEVEL) {
      to_send = packet.water_level;
    }
  }

  byte b = 0;

  #ifdef DEBUG
  Serial.print(" ");
  Serial.println((byte)(to_send | b));
  #endif

  Wire.write((byte)(to_send | b));
  to_send = to_send >> 8;
  req = (req + 1) % 4;
}
