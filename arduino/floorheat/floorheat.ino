#include <QuadDisplay2.h>
#include <Wire.h>
#include <TroykaMQ.h>
#include <OneWire.h>
#include <GyverTimer.h>

#define PIN_MQ5         A3
#define PIN_MQ5_HEATER  11
#define DS_PIN 10
#define RELAY_PIN 12
#define QD_PIN 8
#define GAS_FAILURE_PIN 9
#define GAS_FAILURE_LIMIT 100

#define I2C_SLAVE_ADDRESS 0x18

#define MY_UPTIME             0x99
#define REGISTER_LPG          0xa0
#define REGISTER_METHANE      0xa1
#define REGISTER_CUR_TEMP     0xc0
#define REGISTER_TARGET_TEMP  0xc1
#define REGISTER_CUR_STATE    0xc2
#define REGISTER_TARGET_STATE 0xc3
#define REGISTER_STATE_OFF    0xd0
#define REGISTER_STATE_AUTO   0xd1

//#define DEBUG
//#define DEBUG_I2C
//#define DEBUG_MB

#define MAX_BUFFER 64
#define T35 35

#define TELEGRAM_ADDR 0
#define TELEGRAM_FUNC 1
#define TELEGRAM_BYTE 2
#define TELEGRAM_ERRO 3
#define TELEGRAM_BYHI 4
#define TELEGRAM_BYLO 5
#define TELEGRAM_CRHI 6
#define TELEGRAM_CRLO 7

uint8_t au8Buffer[MAX_BUFFER];
uint8_t address = 0x00;
uint8_t function = 0x00;
uint8_t maxBytes = 0;
uint8_t errCode = 0;

MQ5 mq5(PIN_MQ5, PIN_MQ5_HEATER);
OneWire ds(DS_PIN);
QuadDisplay qd(QD_PIN);
GTimer_ms analogTimer(4000);
GTimer_ms displayTimer(1000);
GTimer_ms ds18b20Timer(4000);

struct DataPacket {
  unsigned long my_uptime = 0;                 // 0x99
  unsigned long lpg = 0;                       // 0xa0
  unsigned long methane = 0;                   // 0xa1
  unsigned long current_floor_temperature = 0; // 0xc0
  unsigned long target_floor_temperature = 10; // 0xc1
  unsigned long current_floor_state = 0;       // 0xc2
  unsigned long target_floor_state = 0;        // 0xc3
};

DataPacket packet;

// i2c
byte reg;
byte req = 0;
unsigned long to_send = 0;

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

void setup()
{  
  pinMode(GAS_FAILURE_PIN, OUTPUT);
  digitalWrite(GAS_FAILURE_PIN, LOW);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  dsMesure();

  qd.begin();
  qd.displayInt(0);

  Wire.begin(I2C_SLAVE_ADDRESS);
  delay(1000);
  Wire.onReceive(processMessage);
  Wire.onRequest(requestEvent);

  #ifdef DEBUG
  Serial.begin(9600);
  while (!Serial);
  #endif

  Serial1.begin(38400);
  while (!Serial1);

  mq5.heaterPwrHigh();
}
 
void loop()
{
  if (!mq5.isCalibrated() && mq5.heatingCompleted()) {
    mq5.calibrate();
  }

  if (analogTimer.isReady()) {
    if (mq5.isCalibrated() && mq5.heatingCompleted()) {
      packet.lpg = mq5.readLPG();
      packet.methane = mq5.readMethane();
    }
    packet.current_floor_temperature = dsGetTemperatureRaw();
  }

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

  if (displayTimer.isReady()) {
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
    Serial.print(" TT ");
    Serial.print(packet.target_floor_temperature);
    Serial.print(" ST ");
    Serial.print(packet.current_floor_state);
    Serial.print(" TS ");
    Serial.print(packet.target_floor_state);
    Serial.println();
    #endif
  }
}

void processMessage(int n) {
  byte data = 0;
  data = Wire.read();

  #ifdef DEBUG_I2C
  Serial.print("i2c process ");
  Serial.println(data);
  #endif

  switch (data) {
    case MY_UPTIME:             //0x99
    case REGISTER_LPG:          //0xa0
    case REGISTER_METHANE:      //0xa1
    case REGISTER_CUR_TEMP:     //0xc0
    case REGISTER_TARGET_TEMP:  //0xc1
    case REGISTER_CUR_STATE:    //0xc2
    case REGISTER_TARGET_STATE: //0xc3
      reg = data;
      #ifdef DEBUG_I2C
      Serial.print("Reg set ");
      Serial.println(reg);
      #endif
      break;
    case REGISTER_STATE_OFF:
      packet.target_floor_state = 0;
      #ifdef DEBUG_I2C
      Serial.println("State off");
      #endif
      break;
    case REGISTER_STATE_AUTO:
      packet.target_floor_state = 1;
      #ifdef DEBUG_I2C
      Serial.println("State on");
      #endif
      break;
    case 0xff: // exclude i2cdetect
      #ifdef DEBUG_I2C
      Serial.println("Skip");
      #endif
      break;
    default:
      if (10 <= data && data <= 38) {
        packet.target_floor_temperature = data;
      }
      #ifdef DEBUG_I2C
      Serial.print("Data ");
      Serial.println(data);
      Serial.print("Temp set ");
      Serial.println(packet.target_floor_temperature);
      #endif
      break;
  }

  req = 0;
}

void requestEvent() {
  #ifdef DEBUG_I2C
  Serial.print("i2c request ");
  Serial.print(reg);
  Serial.print(" ");
  Serial.print(req);
  #endif

  if (req == 0) {
    switch (reg) {
      case MY_UPTIME:
        to_send = packet.my_uptime;
        break;
      case REGISTER_LPG:
        to_send = packet.lpg;
        break;
      case REGISTER_METHANE:
        to_send = packet.methane;
        break;
      case REGISTER_CUR_TEMP:
        to_send = packet.current_floor_temperature;
        break;
      case REGISTER_TARGET_TEMP:
        to_send = packet.target_floor_temperature;
        break;
      case REGISTER_CUR_STATE:
        to_send = packet.current_floor_state;
        break;
      case REGISTER_TARGET_STATE:
        to_send = packet.target_floor_state;
        break;
      default:
        to_send = packet.my_uptime;
        break;
    }
  }

  byte b = 0;

  #ifdef DEBUG_I2C
  Serial.print(" ");
  Serial.println((byte)(to_send | b));
  #endif

  Wire.write((byte)(to_send | b));
  to_send = to_send >> 8;
  req = (req + 1) % 4;
}
