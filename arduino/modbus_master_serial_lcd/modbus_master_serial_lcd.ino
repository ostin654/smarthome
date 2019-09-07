#include <Wire.h>
#include <GyverTimer.h>
#include <LiquidCrystal_I2C.h>
#include <AltSoftSerial.h>

#define SERIAL_PIN 5
#define WELL_LED A5
#define GREENHOUSE_LED 7

//#define DEBUG
#define DEBUG_SERIAL
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

#if defined(ARDUINO) && ARDUINO >= 100
#define printByte(args)  write(args);
#else
#define printByte(args)  print(args,BYTE);
#endif

uint8_t au8Buffer[MAX_BUFFER];
uint8_t address = 0x00;
uint8_t function = 0x00;
uint8_t maxBytes = 0;
uint8_t errCode = 0;

GTimer_ms modbusTimer(2000);
GTimer_ms displayTimer(1000);
GTimer_ms wellNullTimer(10000);
GTimer_ms greenhouseNullTimer(10000);
AltSoftSerial softSerial;
LiquidCrystal_I2C lcd(0x27,20,4);

struct DataPacket {
  unsigned long my_uptime = 0;                 // 0x99
  unsigned long well_uptime = 0;               // 0xb9
  unsigned long water_level = 0;               // 0xb0
  unsigned long greenhouse_uptime = 0;         // 0xe9
  unsigned long greenhouse_soil_hum = 0;       // 0xe0
  unsigned long greenhouse_water_press = 0;    // 0xe1
  unsigned long greenhouse_air_temp = 0;       // 0xe2
  unsigned long greenhouse_relay_state = 0;    // 0xe3
  unsigned long greenhouse_door_state = 0;     // 0xe4
};

DataPacket packet;

void setup()
{  
  pinMode(SERIAL_PIN, OUTPUT);
  digitalWrite(SERIAL_PIN, LOW);
  pinMode(WELL_LED, OUTPUT);
  digitalWrite(WELL_LED, LOW);
  pinMode(GREENHOUSE_LED, OUTPUT);
  digitalWrite(GREENHOUSE_LED, LOW);

  #ifdef DEBUG
  Serial.begin(9600);
  while (!Serial);
  #endif

  #ifdef DEBUG_SERIAL
  softSerial.begin(9600);
  #endif

  Serial1.begin(38400);
  while (!Serial1);

  lcd.init();
  lcd.backlight();
  lcd.clear();
}
 
void loop()
{
  static bool device = false;
  char myStr[10];
  float tempPretty;
  float pressurePretty;
  float waterPretty;

  // modbus send
  if (modbusTimer.isReady()) {
    uint8_t len = 0;
    if (device) {
      len = ReadInputRegisters(0x0A, 1, 2);
    } else {
      len = ReadInputRegisters(0x0B, 1, 6);
    }
    SendBuffer(len);
    device = !device;
  }

  if (wellNullTimer.isReady()) {
    packet.well_uptime = 0;
    packet.water_level = 0;
  }
  if (greenhouseNullTimer.isReady()) {
    packet.greenhouse_uptime = 0;
    packet.greenhouse_soil_hum = 0;
    packet.greenhouse_water_press = 0;
    packet.greenhouse_air_temp = 0;
    packet.greenhouse_relay_state = 0;
    packet.greenhouse_door_state = 0;
  }

  // modbus receive
  if (WaitRespose()) {
    if (address == 0x0A) {
      wellNullTimer.reset();
      packet.well_uptime = word(au8Buffer[3], au8Buffer[4]);
      packet.water_level = word(au8Buffer[5], au8Buffer[6]);

      waterPretty = 7.0 * packet.water_level / 1023.0;
      dtostrf(waterPretty, 2, 1, myStr);
      lcd.setCursor(0,0);
      lcd.print("Water: ");
      lcd.print(myStr);
      lcd.print("m");
    } if (address == 0x0B) {
      greenhouseNullTimer.reset();
      packet.greenhouse_uptime = word(au8Buffer[3], au8Buffer[4]);
      packet.greenhouse_soil_hum = word(au8Buffer[5], au8Buffer[6]);
      packet.greenhouse_water_press = word(au8Buffer[7], au8Buffer[8]);
      packet.greenhouse_air_temp = word(au8Buffer[9], au8Buffer[10]);
      packet.greenhouse_relay_state = word(au8Buffer[11], au8Buffer[12]);
      packet.greenhouse_door_state = word(au8Buffer[13], au8Buffer[14]);

      dtostrf(packet.greenhouse_air_temp, 2, 1, myStr);
      lcd.setCursor(0,1);
      lcd.print(myStr);
      lcd.printByte(0xdf); // degree sign
      lcd.print("C"); // celsium

      sprintf(myStr, "%04d", packet.greenhouse_soil_hum);
      lcd.print("SO ");
      lcd.print(myStr);

      pressurePretty = 0.0146628*packet.greenhouse_water_press-1.5;
      dtostrf(pressurePretty, 3, 1, myStr);  
      lcd.setCursor(0,2);
      lcd.print("Pressure: ");
      lcd.print(myStr);
      lcd.print("b");

      lcd.setCursor(0,3);
      lcd.print("DO");
      lcd.print(packet.greenhouse_door_state);
      lcd.print("VA");
      lcd.print(packet.greenhouse_relay_state);
    }
  }


  if (displayTimer.isReady()) {
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
    softSerial.println(" }");
    #endif
  }
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
    Serial.print(au8Buffer[j]);
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
    Serial.print(c);
    Serial.print(" ");
    Serial.print(byteCnt);
    Serial.print(" ");
    Serial.println(telegramStep);
    #endif
    switch(telegramStep) {
      case TELEGRAM_ADDR:
        address = au8Buffer[i++] = c;
        telegramStep = TELEGRAM_FUNC;
        return 0;
        break;
      case TELEGRAM_FUNC:
        function = au8Buffer[i++] = c;
        telegramStep = TELEGRAM_BYTE;
        if (function >= 0x80) {
          telegramStep = TELEGRAM_ERRO;
        }
        return 0;
        break;
      case TELEGRAM_ERRO:
        errCode = au8Buffer[i++] = c;
        telegramStep = TELEGRAM_CRHI;
        return 0;
        break;
      case TELEGRAM_BYTE:
        maxBytes = au8Buffer[i++] = c;
        if (byteCnt >= maxBytes) {
          telegramStep = TELEGRAM_CRHI;
        } else {
          telegramStep = TELEGRAM_BYHI;
        }
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
        return 0;
        break;
      case TELEGRAM_CRHI:
        au8Buffer[i+1] = c;
        telegramStep = TELEGRAM_CRLO;
        return 0;
        break;
      case TELEGRAM_CRLO:
        {
          au8Buffer[i+2] = c;
          telegramStep = TELEGRAM_ADDR;
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
