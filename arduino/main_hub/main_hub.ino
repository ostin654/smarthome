#include <QuadDisplay2.h>
#include <Wire.h>
#include <TroykaMQ.h>
#include <OneWire.h>

#define PIN_MQ5         A3
#define PIN_MQ5_HEATER  11

#define SERIAL_PIN 5
#define RS485_LED_PIN A5
#define DS_PIN 10
#define TEMP_UPDATE_TIME 1000

#define PRESSURE_PIN A2
#define PRESS_FAILURE_LED A4
#define RELAY_PIN 12

#define GAS_FAILURE_PIN 9
#define GAS_FAILURE_LIMIT 300

#define I2C_SLAVE_ADDRESS 0x18

#define QD_PIN 8

#define PING_PIN 7

#define REGISTER_LPG          0xa0
#define REGISTER_METHANE      0xa1
#define REGISTER_PRESSURE     0xa2
#define REGISTER_WATER_LEVEL  0xb0
#define REGISTER_CUR_TEMP     0xc0
#define REGISTER_TARGET_TEMP  0xc1
#define REGISTER_CUR_STATE    0xc2
#define REGISTER_TARGET_STATE 0xc3
#define REGISTER_STATE_OFF    0xd0
#define REGISTER_STATE_AUTO   0xd1

//#define DEBUG

MQ5 mq5(PIN_MQ5, PIN_MQ5_HEATER);
OneWire ds(DS_PIN);
QuadDisplay qd(QD_PIN);

struct DataPacket {
  unsigned long lpg = 0;                       // 0xa0
  unsigned long methane = 0;                   // 0xa1
  unsigned long pressure = 0;                  // 0xa2
  unsigned long water_level = 0;               // 0xb0
  unsigned long current_floor_temperature = 0; // 0xc0
  unsigned long target_floor_temperature = 10; // 0xc1
  unsigned long current_floor_state = 0;       // 0xc2
  unsigned long target_floor_state = 0;        // 0xc3
};

DataPacket packet;

byte reg;
byte req = 0;
unsigned long to_send = 0;

int median[3] = {0, 0, 0};   // массив для хранения трёх последних измерений
byte i = 0;

byte data[12];
byte addr[8];
long dsLastUpdateTime = 0;

void dsMesure()
{
  ds.reset();
  ds.write(0xCC);
  ds.write(0x44, 1);

  dsLastUpdateTime = millis();
}

unsigned long dsGetTemperatureRaw()
{
  static unsigned long temperature = 0;
  if (millis() - dsLastUpdateTime > TEMP_UPDATE_TIME)
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

// медианный фильтр из 3ёх значений
float middle_of_3(int a, int b, int c) {
  int middle;
  if ((a <= b) && (a <= c)) {
    middle = (b <= c) ? b : c;
  }
  else {
    if ((b <= a) && (b <= c)) {
      middle = (a <= c) ? a : c;
    }
    else {
      middle = (a <= b) ? a : b;
    }
  }
  return middle;
}

void(* resetFunc) (void) = 0; // объявляем функцию reset

void setup()
{  
  pinMode(SERIAL_PIN, OUTPUT);
  digitalWrite(SERIAL_PIN, LOW);

  pinMode(RS485_LED_PIN, OUTPUT);
  digitalWrite(RS485_LED_PIN, LOW);

  pinMode(PRESS_FAILURE_LED, OUTPUT);
  digitalWrite(PRESS_FAILURE_LED, LOW);

  pinMode(GAS_FAILURE_PIN, OUTPUT);
  digitalWrite(GAS_FAILURE_PIN, LOW);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  pinMode(PING_PIN, OUTPUT);
  digitalWrite(PING_PIN, LOW);

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
  } else {
    digitalWrite(GAS_FAILURE_PIN, LOW);
  }

  packet.pressure = analogRead(PRESSURE_PIN);
  if (packet.pressure < 150) {
    digitalWrite(PRESS_FAILURE_LED, HIGH);
  } else {
    digitalWrite(PRESS_FAILURE_LED, LOW);
  }

  if (Serial1.available() > 0) {
    int serialData = Serial1.parseInt();
    if (serialData >= 0 && serialData <= 1023) {
      if (i > 1) i = 0;
      else i++;
      median[i] = serialData;
      packet.water_level = middle_of_3(median[0], median[1], median[2]);;
      digitalWrite(RS485_LED_PIN, HIGH);
    } else {
      digitalWrite(RS485_LED_PIN, LOW);
      #ifdef DEBUG
      Serial.println("No 485 data");
      #endif
    }
    delay(30);
  } else {
    digitalWrite(RS485_LED_PIN, LOW);
    #ifdef DEBUG
    Serial.println("No 485 data");
    #endif
    delay(1000);
  }

  packet.current_floor_temperature = dsGetTemperatureRaw();
  if (millis() % 4000 < 2000) {
     qd.displayFloat((float)(packet.current_floor_temperature * 0.0625), 1);
  } else {
    if (packet.target_floor_state == 0) {
      qd.displayDigits(QD_NONE, QD_o, QD_f, QD_f); 
    } else {
      qd.displayInt(packet.target_floor_temperature);
    }
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

  #ifdef DEBUG
  Serial.print(packet.lpg);
  Serial.print(" ");
  Serial.print(packet.methane);
  Serial.print(" ");
  Serial.print(packet.pressure);
  Serial.print(" ");
  Serial.print(packet.water_level);
  Serial.print(" ");
  Serial.print(packet.current_floor_temperature);
  Serial.print(" ");
  Serial.print(packet.target_floor_temperature);
  Serial.print(" ");
  Serial.print(packet.current_floor_state);
  Serial.print(" ");
  Serial.print(packet.target_floor_state);
  Serial.println();
  #endif

  digitalWrite(PING_PIN, HIGH);
  delay(20);
  digitalWrite(PING_PIN, LOW);

  //if (millis() > 300000) resetFunc(); //вызываем reset
}

void processMessage(int n) {
  byte data = 0;
  data = Wire.read();

  #ifdef DEBUG
  Serial.print("i2c process ");
  Serial.println(data);
  #endif

  switch (data) {
    case REGISTER_LPG:
    case REGISTER_METHANE:
    case REGISTER_PRESSURE:
    case REGISTER_WATER_LEVEL:
    case REGISTER_CUR_TEMP:
    case REGISTER_TARGET_TEMP:
    case REGISTER_CUR_STATE:
    case REGISTER_TARGET_STATE:
      reg = data;
      #ifdef DEBUG
      Serial.print("Reg set ");
      Serial.println(reg);
      #endif
      break;
    case REGISTER_STATE_OFF:
      packet.target_floor_state = 0;
      #ifdef DEBUG
      Serial.println("State off");
      #endif
      break;
    case REGISTER_STATE_AUTO:
      packet.target_floor_state = 1;
      #ifdef DEBUG
      Serial.println("State on");
      #endif
      break;
    case 0xff: // exclude i2cdetect
      #ifdef DEBUG
      Serial.println("Skip");
      #endif
      break;
    default:
      if (10 <= data && data <= 33) {
        packet.target_floor_temperature = data;
      }
      #ifdef DEBUG
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
  #ifdef DEBUG
  Serial.print("i2c request ");
  Serial.print(reg);
  Serial.print(" ");
  Serial.print(req);
  #endif

  if (req == 0) {
    switch (reg) {
      case REGISTER_LPG:
        to_send = packet.lpg;
        break;
      case REGISTER_METHANE:
        to_send = packet.methane;
        break;
      case REGISTER_PRESSURE:
        to_send = packet.pressure;
        break;
      case REGISTER_WATER_LEVEL:
        to_send = packet.water_level;
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
        to_send = packet.lpg;
        break;
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
