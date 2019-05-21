#include <SoftwareSerial.h>
#include <TinyBME280.h>

#define RELAY_PIN 0
#define SENSOR_PIN 1
#define TX_PIN 2
#define RX_PIN 8
#define SOIL_PIN A0
#define DIR_PIN 9
#define MAX_DRY 500
#define DRY_DELAY 80000
#define FIRST_DELAY 80000

SoftwareSerial mySerial(RX_PIN, TX_PIN); // RX, TX
TinyBME280 bme; // I2C
unsigned long lastDryStart = 0;
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
  delay(50);
  soilHumidity = analogRead(SOIL_PIN);
  digitalWrite(SENSOR_PIN, LOW);
  
  if (millis() > FIRST_DELAY * 1000) {
    if (soilHumidity < 350) {
      if (lastDryStart + DRY_DELAY * 1000 < millis()) {
        lastDryStart = millis();
      }
      if (lastDryStart + MAX_DRY * 1000 < millis()) {
        relayState = LOW;
      } else {
        relayState = HIGH;
      }
    }
    if (soilHumidity > 450) {
      relayState = LOW;
    }
  }

  mySerial.print("Soil: ");
  mySerial.print(soilHumidity);
  mySerial.print(" ; LastDry: ");
  mySerial.print(lastDryStart);
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
