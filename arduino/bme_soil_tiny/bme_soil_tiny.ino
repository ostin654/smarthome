#include <SoftwareSerial.h>
#include <TinyBME280.h>

#define RELAY_PIN 0
#define SENSOR_PIN 1
#define TX_PIN 2
#define RX_PIN 8
#define SOIL_PIN A0
#define DIR_PIN 9
#define MAX_DRY 400000
#define DRY_DELAY 172000000

SoftwareSerial mySerial(RX_PIN, TX_PIN); // RX, TX
TinyBME280 bme; // I2C
unsigned int soilHumidity = 0;
bool relayState = LOW;

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  pinMode(SENSOR_PIN, OUTPUT);
  digitalWrite(SENSOR_PIN, LOW);
  pinMode(DIR_PIN, OUTPUT);
  digitalWrite(DIR_PIN, LOW);

  mySerial.begin(9600);
  
  if (!bme.begin(0x76)) {
    mySerial.println("Could not find a valid BME280 sensor, check wiring!");
    while(1);
  }
}
 
void loop() {
  digitalWrite(SENSOR_PIN, HIGH);
  delay(20);
  soilHumidity = analogRead(SOIL_PIN);
  digitalWrite(SENSOR_PIN, LOW);

  if (millis() % DRY_DELAY < MAX_DRY) {
    relayState = HIGH;
  } else {
    relayState = LOW;
  }

  mySerial.print("Soil: ");
  mySerial.print(soilHumidity);
  mySerial.print(" ; Millis: ");
  mySerial.print(millis());
  mySerial.print(" ; Relay: ");
  mySerial.print(relayState);
  mySerial.print(" ; Temperature: ");
  mySerial.println(bme.readTemperature());

  if (bme.readTemperature() > 31) {
    digitalWrite(DIR_PIN, HIGH);
  }
  if (bme.readTemperature() < 23) {
    digitalWrite(DIR_PIN, LOW);
  }

  digitalWrite(RELAY_PIN, relayState);

  delay(1000);
}
