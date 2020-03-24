#include <QuadDisplay2.h>
#include <TroykaMQ.h>
#include <OneWire.h>
#include <GyverTimer.h>
#include <ArduinoJson.h>
#include <Encoder.h>
#include <EEPROM.h>
//#include <RH_ASK.h>

#define PIN_MQ5         A3
#define PIN_MQ5_HEATER  11
#define DS_PIN 3
#define RELAY_PIN A2
#define QD_PIN 10
#define GAS_FAILURE_PIN 8
#define GAS_FAILURE_LIMIT 100
#define ENCODER_PIN_A 2
#define ENCODER_PIN_B 7
//#define RF_PIN 6

//#define DEBUG
#define DEBUG_SERIAL1

#define EEPROM_ADDRESS 0

StaticJsonBuffer<200> jb;
MQ5 mq5(PIN_MQ5, PIN_MQ5_HEATER);
OneWire ds(DS_PIN);
QuadDisplay qd(QD_PIN);
Encoder myEnc(ENCODER_PIN_A, ENCODER_PIN_B);
GTimer_ms analogTimer(4000);
GTimer_ms displayTimer(1000);
//GTimer_ms nullTimer(50000);
GTimer_ms setTimer(5000);
GTimer_ms ds18b20Timer(4000);
//RH_ASK driver(2000, RF_PIN, 0, 0);

struct DataPacket {
  unsigned long my_uptime = 0;                 // 0x99
  unsigned long lpg = 0;                       // 0xa0
  unsigned long methane = 0;                   // 0xa1
  unsigned long current_floor_temperature = 0; // 0xc0
  unsigned long target_floor_temperature = 10; // 0xc1
  unsigned long current_floor_state = 0;       // 0xc2
  unsigned long target_floor_state = 0;        // 0xc3
  //long current_room_temperature = -1;        // 0xc4
};

DataPacket packet;

void setup()
{  
  pinMode(GAS_FAILURE_PIN, OUTPUT);
  digitalWrite(GAS_FAILURE_PIN, LOW);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  dsMesure();

  qd.begin();
  qd.displayInt(0);

  EEPROM.get(EEPROM_ADDRESS, packet.target_floor_temperature);
  EEPROM.get(EEPROM_ADDRESS + sizeof(packet.target_floor_temperature), packet.current_floor_state);

  if (packet.target_floor_temperature > 38) packet.target_floor_temperature = 38;
  if (packet.target_floor_temperature < 10) packet.target_floor_temperature = 10;
  packet.current_floor_state = packet.current_floor_state ? 1 : 0;

  myEnc.write(packet.target_floor_temperature * 4);
  setTimer.setMode(MANUAL);     // ручной режим

  #ifdef DEBUG
  Serial.begin(9600);
  while (!Serial);
  #endif

  #ifdef DEBUG_SERIAL1
  Serial1.begin(9600);
  while (!Serial1);
  Serial1.setTimeout(1000);
  #endif

  //driver.init();

  mq5.heaterPwrHigh();
  mq5.calibrate(0.5);

  packet.current_floor_temperature = dsGetTemperatureRaw();
}
 
void loop()
{
  #ifdef DEBUG_SERIAL1
  if (Serial1.available()) {
    String request = Serial1.readString();

    #ifdef DEBUG
    Serial.println(request);
    #endif

    JsonObject& obj = jb.parseObject(request);
    if (obj.success()) {
      packet.target_floor_state = obj.get<int>("target_state");
      if (packet.target_floor_state > 0) {
        packet.target_floor_state = 1;
      } else {
        packet.target_floor_state = 0;
      }
      packet.target_floor_temperature = obj.get<int>("target_temp");
      if (packet.target_floor_temperature > 38) {
        packet.target_floor_temperature = 38;
      }
      if (packet.target_floor_temperature < 10) {
        packet.target_floor_temperature = 10;
      }

      myEnc.write(packet.target_floor_temperature * 4);
      setTimer.reset();
      
      qd.displayDigits(QD_NONE, QD_S, QD_E, QD_t);
      #ifdef DEBUG
      Serial.println("JSON Ok!");
      #endif
      delay(1000);
    } else {
      qd.displayDigits(QD_NONE, QD_E, QD_r, QD_r);
      delay(1000);
      #ifdef DEBUG
      Serial.println("Problem JSON...");
      #endif
    }
    jb.clear();
  }
  #endif

  static long oldPosition = 0;
  long newPosition = myEnc.read();
  if (newPosition != oldPosition) {
    oldPosition = newPosition;
    if (newPosition % 4 == 0) {
      packet.target_floor_temperature = newPosition / 4;
      packet.target_floor_state = 1;
      qd.displayInt(packet.target_floor_temperature);
      setTimer.reset();
    }
  }
  if (newPosition > 38 * 4) {
    myEnc.write(38 * 4);
    newPosition = 38 * 4;
  }
  if (newPosition < 10 * 4) {
    myEnc.write(10 * 4);
    newPosition = 10 * 4;
  }

  /*
  uint8_t buf[4];
  uint8_t buflen = sizeof(buf);
  uint8_t address = 0;
  uint32_t temperature = 0;
  if (driver.recv(buf, 4)) {
    #ifdef DEBUG
    Serial.print(buf[1], HEX);
    Serial.print(" ");
    Serial.print(buf[2], HEX);
    Serial.print(" ");
    Serial.println(buf[3], HEX);
    #endif
    
    address = buf[1];
    if (address == 0x4D) {
      temperature = buf[3];
      temperature <<=8;
      temperature |= buf[2];

      packet.current_room_temperature = temperature;
      nullTimer.reset();
    }
  }

  if (nullTimer.isReady()) {
    packet.current_room_temperature = -1;
  }
  /**/

  if (analogTimer.isReady()) {
    if (mq5.isCalibrated() && mq5.heatingCompleted()) {
      packet.lpg = mq5.readLPG();
      packet.methane = mq5.readMethane();
    }
    packet.current_floor_temperature = dsGetTemperatureRaw();
    packet.my_uptime = millis() / 60000;

    if (packet.lpg > GAS_FAILURE_LIMIT || packet.methane > GAS_FAILURE_LIMIT) {
      digitalWrite(GAS_FAILURE_PIN, HIGH);
    } else {
      digitalWrite(GAS_FAILURE_PIN, LOW);
    }
  
    if (packet.target_floor_state == 0) {
      digitalWrite(RELAY_PIN, LOW);
      packet.current_floor_state = 0;
    } else {
      if (packet.current_floor_temperature * 0.0625 < packet.target_floor_temperature) {
        digitalWrite(RELAY_PIN, HIGH);
        packet.current_floor_state = 1;
      }
      if (packet.current_floor_temperature * 0.0625 > packet.target_floor_temperature + 3) {
        digitalWrite(RELAY_PIN, LOW);
        packet.current_floor_state = 0;
      }
    }
    EEPROM.put(EEPROM_ADDRESS, packet.target_floor_temperature);
    EEPROM.put(EEPROM_ADDRESS + sizeof(packet.target_floor_temperature), packet.current_floor_state);
  }

  if (displayTimer.isReady() && setTimer.isReady()) {
    if (millis() % 4000 < 2000) {
       qd.displayFloat((float)(packet.current_floor_temperature * 0.0625), 1);
    } else {
      if (packet.target_floor_state == 0) {
        qd.displayDigits(QD_NONE, QD_o, QD_f, QD_f); 
      } else {
        qd.displayInt(packet.target_floor_temperature);
      }
    }

    #ifdef DEBUG
    Serial.print("UP ");
    Serial.print(packet.my_uptime);
    Serial.print(" LP ");
    Serial.print(packet.lpg);
    Serial.print(" ME ");
    Serial.print(packet.methane);
    Serial.print(" TE ");
    Serial.print(packet.current_floor_temperature);
    //Serial.print(" RT ");
    //Serial.print(packet.current_room_temperature);
    Serial.print(" TT ");
    Serial.print(packet.target_floor_temperature);
    Serial.print(" ST ");
    Serial.print(packet.current_floor_state);
    Serial.print(" TS ");
    Serial.print(packet.target_floor_state);
    Serial.print(" RO ");
    Serial.print(mq5.getRo());
    Serial.println();
    #endif

    #ifdef DEBUG_SERIAL1
    Serial1.print("{ \"uptime\": ");
    Serial1.print(packet.my_uptime);
    Serial1.print(", \"lpg\": ");
    Serial1.print(packet.lpg);
    Serial1.print(", \"methane\": ");
    Serial1.print(packet.methane);
    Serial1.print(", \"current_temp\": ");
    Serial1.print(packet.current_floor_temperature);
    Serial1.print(", \"target_temp\": ");
    Serial1.print(packet.target_floor_temperature);
    Serial1.print(", \"current_state\": ");
    Serial1.print(packet.current_floor_state);
    Serial1.print(", \"target_state\": ");
    Serial1.print(packet.target_floor_state);
    //Serial1.print(", \"room_temp\": ");
    //Serial1.print(packet.current_room_temperature);
    Serial1.println(" }");
    #endif
  }
}

// DS18B20
void dsMesure()
{
  ds.reset();
  ds.write(0xCC);
  ds.write(0x44, 1);
}

unsigned long dsGetTemperatureRaw()
{
  static unsigned long temperature = 0;
  if (ds18b20Timer.isReady())
  {
    ds.reset();
    ds.write(0xCC);
    ds.write(0xBE);

    byte data[2];
    data[0] = ds.read();
    data[1] = ds.read();
    temperature = (data[1] << 8) | data[0];

    dsMesure();
  }

  return temperature;
}
