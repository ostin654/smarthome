#include <GyverTimer.h>
#include <SoftwareSerial.h>
#include <QuadDisplay2.h>

#define SERIAL_PIN 5
#define RX A2
#define TX A3
#define QD_PIN 10

//#define DEBUG
#define DEBUG_SERIAL
//#define DEBUG_MB
//#define QD_TEST

#define DEVICE_COUNT 4
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
int device = 0;

GTimer_ms modbusTimer(3468);
GTimer_ms displayTimer(2379);
GTimer_ms wellNullTimer(30000);
GTimer_ms greenhouseNullTimer(30000);
GTimer_ms gastankNullTimer(30000);
GTimer_ms roomNullTimer(90000);
SoftwareSerial softSerial(RX, TX);
QuadDisplay qd(QD_PIN);

const static uint8_t numerals[] = { QD_0, QD_1, QD_2, QD_3, QD_4, QD_5, QD_6, QD_7, QD_8, QD_9 };

struct DataPacket {
  long my_uptime = -1;                 // 0x99
  long well_uptime = -1;               // 0xb9
  long water_level = -1;               // 0xb0
  long greenhouse_uptime = -1;         // 0xe9
  long greenhouse_soil_hum = -1;       // 0xe0
  long greenhouse_water_press = -1;    // 0xe1
  long greenhouse_air_temp = -100000;  // 0xe2
  long greenhouse_relay_state = -1;    // 0xe3
  long greenhouse_door_state = -1;     // 0xe4
  long gastank_uptime = -1;            // 0xd9
  long gastank_level = -1;             // 0xd0
  long rx_433_uptime = -1;             // 0xf9
  long room_temperature = -100000;     // 0xf0
};

DataPacket packet;

void setup()
{  
  pinMode(SERIAL_PIN, OUTPUT);
  digitalWrite(SERIAL_PIN, LOW);

  delay(1000);

  qd.begin();
  qd.displayInt(0);

  #if defined(DEBUG) || defined(DEBUG_MB)
  Serial.begin(9600);
  while (!Serial);
  #endif

  #ifdef DEBUG_SERIAL
  softSerial.begin(9600);
  #endif

  Serial1.begin(38400);
  while (!Serial1);
}
 
void loop()
{
  if (roomNullTimer.isReady()) {
    packet.rx_433_uptime = -1;
    packet.room_temperature = -100000;
  }
  if (wellNullTimer.isReady()) {
    packet.well_uptime = -1;
    packet.water_level = -1;
  }
  if (gastankNullTimer.isReady()) {
    packet.gastank_uptime = -1;
    packet.gastank_level = -1;
  }
  if (greenhouseNullTimer.isReady()) {
    packet.greenhouse_uptime = -1;
    packet.greenhouse_soil_hum = -1;
    packet.greenhouse_water_press = -1;
    packet.greenhouse_air_temp = -100000;
    packet.greenhouse_relay_state = -1;
    packet.greenhouse_door_state = -1;
  }

  // modbus send
  if (modbusTimer.isReady()) {
    uint8_t len = 0;
    switch(device % DEVICE_COUNT) {
      case 0:
        len = ReadInputRegisters(0x0A, 1, 2);
        break;
      case 1:
        len = ReadInputRegisters(0x0B, 1, 6);
        break;
      case 2:
        len = ReadInputRegisters(0x0C, 1, 2);
        break;
      case 3:
        len = ReadInputRegisters(0x0D, 1, 2);
        break;
    }
    SendBuffer(len);
    device++;
    device = device % DEVICE_COUNT;
  }

  // modbus receive
  if (WaitRespose()) {
    if (address == 0x0A) {
      wellNullTimer.reset();
      packet.well_uptime = word(au8Buffer[3], au8Buffer[4]);
      packet.water_level = word(au8Buffer[5], au8Buffer[6]);
    }
    else if (address == 0x0B) {
      greenhouseNullTimer.reset();
      packet.greenhouse_uptime = word(au8Buffer[3], au8Buffer[4]);
      packet.greenhouse_soil_hum = word(au8Buffer[5], au8Buffer[6]);
      packet.greenhouse_water_press = word(au8Buffer[7], au8Buffer[8]);
      packet.greenhouse_air_temp = word(au8Buffer[9], au8Buffer[10]);
      packet.greenhouse_relay_state = word(au8Buffer[11], au8Buffer[12]);
      packet.greenhouse_door_state = word(au8Buffer[13], au8Buffer[14]);
    }
    else if (address == 0x0C) {
      gastankNullTimer.reset();
      packet.gastank_uptime = word(au8Buffer[3], au8Buffer[4]);
      packet.gastank_level = word(au8Buffer[5], au8Buffer[6]);
    }
    else if (address == 0x0D) {
      roomNullTimer.reset();
      packet.rx_433_uptime = word(au8Buffer[3], au8Buffer[4]);
      packet.room_temperature = word(au8Buffer[5], au8Buffer[6]);
    }
    else {
      Serial.println(address, HEX);
    }
  } else {
    #ifdef QD_TEST
    packet.water_level = 287;
    packet.greenhouse_soil_hum = 698;
    packet.greenhouse_water_press = 255;
    packet.greenhouse_air_temp = 31000;
    packet.greenhouse_relay_state = 1;
    packet.greenhouse_door_state = 0;
    packet.gastank_level = 466;
    #endif
  }

  if (displayTimer.isReady()) {

    if (millis() % 14274 < 2379) {
      if (packet.water_level >= 0) {
        qd.displayFloat((float)(packet.water_level * 7.0 / 1023.0), 2);
      } else {
        qd.displayDigits(QD_NONE, QD_MINUS, QD_MINUS & QD_DOT, QD_MINUS);
      }
    } else if (millis() % 14274 < 2379*2) {
      if (packet.greenhouse_air_temp > -100000) {
        qd.displayTemperatureC((packet.greenhouse_air_temp / 1000), true);
      } else {
        qd.displayDigits(QD_MINUS, QD_MINUS, QD_DEGREE, QD_C);
      }
    } else if (millis() % 14274 < 2379*3) {
      if (packet.greenhouse_soil_hum >= 0) {
        qd.displayInt(packet.greenhouse_soil_hum, true);
      } else {
        qd.displayDigits(QD_MINUS, QD_MINUS, QD_MINUS, QD_MINUS);
      }
    } else if (millis() % 14274 < 2379*4) {
      if (packet.greenhouse_water_press >= 0) {
        int pressure = packet.greenhouse_water_press * 1.4662757 - 150;
        #ifdef DEBUG
        Serial.println(pressure);
        #endif
        uint8_t digits[3] = { 0xff, 0xff, 0xff };
        digits[2] = pressure % 10;
        pressure /= 10;
        #ifdef DEBUG
        Serial.println(pressure);
        #endif
        digits[1] = pressure % 10;
        pressure /= 10;
        #ifdef DEBUG
        Serial.println(pressure);
        #endif
        digits[0] = pressure % 10;
        
        #ifdef DEBUG
        Serial.println(digits[0]);
        Serial.println(digits[1]);
        Serial.println(digits[2]);
        #endif

        qd.displayDigits(numerals[digits[0]], numerals[digits[1]] & QD_DOT, numerals[digits[2]], QD_b);
      } else {
        qd.displayDigits(QD_MINUS, QD_MINUS & QD_DOT, QD_MINUS, QD_b);
      }
    } else if (millis() % 14274 < 2379*5) {
      uint8_t door = QD_MINUS;
      if (packet.greenhouse_door_state == 0) {
        door = QD_0;
      }
      if (packet.greenhouse_door_state > 0) {
        door = QD_1;
      }
      uint8_t valve = QD_MINUS;
      if (packet.greenhouse_relay_state == 0) {
        valve = QD_0;
      }
      if (packet.greenhouse_relay_state > 0) {
        valve = QD_1;
      }
      qd.displayDigits(QD_d, door , QD_U, valve); 
    } else {
      if (packet.gastank_level >= 0) {
        qd.displayHumidity(packet.gastank_level * 100 / 1023);
      } else {
        qd.displayDigits(QD_MINUS, QD_MINUS, QD_DEGREE, QD_UNDER_DEGREE);
      }
    }

    packet.my_uptime = millis()/60000;

    #ifdef DEBUG
    Serial.print("UP ");
    Serial.print(packet.my_uptime);
    Serial.print(" WU ");
    Serial.print(packet.well_uptime);
    Serial.print(" WA ");
    Serial.print(packet.water_level);
    Serial.print(" UP ");
    Serial.print(packet.greenhouse_uptime);
    Serial.print(" SO ");
    Serial.print(packet.greenhouse_soil_hum);
    Serial.print(" PR ");
    Serial.print(packet.greenhouse_water_press);
    Serial.print(" TE ");
    Serial.print(packet.greenhouse_air_temp);
    Serial.print(" RE ");
    Serial.print(packet.greenhouse_relay_state);
    Serial.print(" DO ");
    Serial.print(packet.greenhouse_door_state);
    Serial.print(" GA ");
    Serial.print(packet.gastank_level);
    Serial.print(" RT ");
    Serial.print(packet.room_temperature);
    Serial.println();
    #endif

    #ifdef DEBUG_SERIAL
    softSerial.print("{ \"uptime\": ");
    softSerial.print(packet.my_uptime);
    softSerial.print(", \"well_uptime\": ");
    softSerial.print(packet.well_uptime);
    softSerial.print(", \"water_level\": ");
    softSerial.print(packet.water_level);
    softSerial.print(", \"greenhouse_uptime\": ");
    softSerial.print(packet.greenhouse_uptime);
    softSerial.print(", \"greenhouse_soil_hum\": ");
    softSerial.print(packet.greenhouse_soil_hum);
    softSerial.print(", \"greenhouse_water_press\": ");
    softSerial.print(packet.greenhouse_water_press);
    softSerial.print(", \"greenhouse_air_temp\": ");
    softSerial.print(packet.greenhouse_air_temp);
    softSerial.print(", \"greenhouse_relay_state\": ");
    softSerial.print(packet.greenhouse_relay_state);
    softSerial.print(", \"greenhouse_door_state\": ");
    softSerial.print(packet.greenhouse_door_state);
    softSerial.print(", \"gastank_uptime\": ");
    softSerial.print(packet.gastank_uptime);
    softSerial.print(", \"gastank_level\": ");
    softSerial.print(packet.gastank_level);
    softSerial.print(", \"room_temp\": ");
    softSerial.print(packet.room_temperature);
    softSerial.println(" }");
    #endif
  }
  /**/
}


uint8_t ReadInputRegisters(uint8_t address, uint16_t firstReg, uint16_t regNumber) {
  uint8_t i = 0;
  uint16_t crc = 0;

  au8Buffer[i++] = address; // device address
  au8Buffer[i++] = 0x04; // function code
  au8Buffer[i++] = highByte(firstReg);
  au8Buffer[i++] = lowByte(firstReg);
  au8Buffer[i++] = highByte(regNumber);
  au8Buffer[i++] = lowByte(regNumber);

  // append CRC to message
  crc = CalculateCRC(i);
  au8Buffer[i++] = crc >> 8;
  au8Buffer[i++] = crc & 0x00ff;

  return i;
}

void SendBuffer(uint8_t len) {
  delay(20);
  digitalWrite(SERIAL_PIN, HIGH);
  delay(20);
  #ifdef DEBUG_MB
  for (uint8_t j=0; j<len;j++) {
    Serial.print(au8Buffer[j], HEX);
    Serial.print(" ");
  }
  Serial.println();
  #endif;
  Serial1.write(au8Buffer, len);
  delay(20);
}

bool WaitRespose() {
  uint8_t len;
  digitalWrite(SERIAL_PIN, LOW);
  len = ReceiveTelegram();
  if (len > 0 && errCode == 0) {
    return true;
  } else {
    return false;
  }
}

uint8_t ReceiveTelegram() {
  static uint8_t telegramStep = 0;
  static uint8_t i = 0;
  static uint32_t telegramStart = 0;
  static uint8_t byteCnt = 0;

  if (millis() - telegramStart > T35) {
    telegramStep = TELEGRAM_ADDR;
    byteCnt = i = 0;
    errCode = 0;
  }

  if (Serial1.available()) {
    byte c = Serial1.read();
    telegramStart = millis();
    #ifdef DEBUG_MB
    Serial.print(c, HEX);
    Serial.print(" ");
    Serial.print(byteCnt);
    Serial.print(" ");
    Serial.println(telegramStep);
    #endif
    switch(telegramStep) {
      case TELEGRAM_ADDR:
        address = au8Buffer[i++] = c;
        telegramStep = TELEGRAM_FUNC;
        #ifdef DEBUG_MB
        Serial.print("TELEGRAM_ADDR ");
        Serial.println(c, HEX);
        #endif
        return 0;
        break;
      case TELEGRAM_FUNC:
        function = au8Buffer[i++] = c;
        telegramStep = TELEGRAM_BYTE;
        #ifdef DEBUG_MB
        Serial.print("TELEGRAM_FUNC ");
        Serial.println(c, HEX);
        #endif
        if (function >= 0x80) {
          telegramStep = TELEGRAM_ERRO;
        }
        return 0;
        break;
      case TELEGRAM_ERRO:
        errCode = au8Buffer[i++] = c;
        telegramStep = TELEGRAM_CRHI;
        #ifdef DEBUG_MB
        Serial.print("TELEGRAM_ERRO ");
        Serial.println(c, HEX);
        #endif
        return 0;
        break;
      case TELEGRAM_BYTE:
        maxBytes = au8Buffer[i++] = c;
        if (byteCnt >= maxBytes) {
          telegramStep = TELEGRAM_CRHI;
        } else {
          telegramStep = TELEGRAM_BYHI;
        }
        #ifdef DEBUG_MB
        Serial.print("TELEGRAM_BYTE ");
        Serial.println(c, HEX);
        #endif
        return 0;
        break;
      case TELEGRAM_BYHI:
        au8Buffer[i++] = c;
        byteCnt++;
        if (byteCnt >= maxBytes) {
          telegramStep = TELEGRAM_CRHI;
        } else {
          telegramStep = TELEGRAM_BYLO;
        }
        #ifdef DEBUG_MB
        Serial.print("TELEGRAM_BYHI ");
        Serial.println(c, HEX);
        #endif
        return 0;
        break;
      case TELEGRAM_BYLO:
        au8Buffer[i++] = c;
        byteCnt++;
        if (byteCnt >= maxBytes) {
          telegramStep = TELEGRAM_CRHI;
        } else {
          telegramStep = TELEGRAM_BYHI;
        }
        #ifdef DEBUG_MB
        Serial.print("TELEGRAM_BYLO ");
        Serial.println(c, HEX);
        #endif
        return 0;
        break;
      case TELEGRAM_CRHI:
        au8Buffer[i+1] = c;
        telegramStep = TELEGRAM_CRLO;
        #ifdef DEBUG_MB
        Serial.print("TELEGRAM_CRHI ");
        Serial.println(c, HEX);
        #endif
        return 0;
        break;
      case TELEGRAM_CRLO:
        {
          au8Buffer[i+2] = c;
          telegramStep = TELEGRAM_ADDR;
          #ifdef DEBUG_MB
          Serial.print("TELEGRAM_CRLO ");
          Serial.println(c, HEX);
          #endif

          uint8_t len = i;
          uint16_t crc = 0;
          i = 0;

          crc = au8Buffer[len+1] << 8;
          crc |= au8Buffer[len+2];
          if (crc != CalculateCRC(len)) {
            #ifdef DEBUG_MB
            Serial.print(" WRONG CRC ");
            Serial.print(word(au8Buffer[len+1], au8Buffer[len+2]), DEC);
            Serial.print(CalculateCRC(len), DEC);
            #endif;
            return 0;
          }
          return len;
        }
        break;
      default:
        telegramStep = TELEGRAM_ADDR;
        i = 0;
        return 0;
        break;
    }
  } else {
    return 0;
  }
}

uint16_t CalculateCRC(uint8_t u8length) {
    unsigned int temp, temp2, flag;
    temp = 0xFFFF;
    for (unsigned char i = 0; i < u8length; i++)
    {
        temp = temp ^ au8Buffer[i];
        for (unsigned char j = 1; j <= 8; j++)
        {
            flag = temp & 0x0001;
            temp >>=1;
            if (flag)
                temp ^= 0xA001;
        }
    }
    // Reverse byte order.
    temp2 = temp >> 8;
    temp = (temp << 8) | temp2;
    temp &= 0xFFFF;
    // the returned value is already swapped
    // crcLo byte is first & crcHi byte is last
    return temp;
}
