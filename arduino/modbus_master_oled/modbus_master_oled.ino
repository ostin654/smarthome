#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <GyverTimer.h>
#include <SoftwareSerial.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define SERIAL_PIN 7
#define RX 9
#define TX 8

#define DEBUG_SERIAL
//#define DEBUG_MB
//#define OLED_TEST

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
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

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

  Serial.begin(38400);

  #ifdef DEBUG_SERIAL
  softSerial.begin(9600);
  #endif

  delay(1000);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
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

  #ifdef OLED_TEST
  packet.water_level = 287;
  packet.greenhouse_soil_hum = 698;
  packet.greenhouse_water_press = 255;
  packet.greenhouse_air_temp = 3100;
  packet.greenhouse_relay_state = 1;
  packet.greenhouse_door_state = 0;
  packet.gastank_level = 466;
  packet.room_temperature = 2378;
  #else
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
  }
  #endif

  if (displayTimer.isReady()) {
    display.clearDisplay();
    display.setTextSize(1);      // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE); // Draw white text
    display.setCursor(0, 0);     // Start at top-left corner
    display.cp437(true);         // Use full 256 char 'Code Page 437' font

    display.print("Water: ");
    if (packet.water_level >= 0) {
      display.print((float)(packet.water_level * 7.0 / 1023.0));
    } else {
      display.print("-.--");
    }
    display.print("m ");

    display.print("Gas: ");
    if (packet.gastank_level >= 0) {
      display.print(packet.gastank_level * 100 / 1023);
    } else {
      display.print("--");
    }
    display.print("%");
    display.println("");

    display.print("Room: ");
    if (packet.room_temperature > -10000) {
      display.print(packet.room_temperature / 100);
    } else {
      display.print("--");
    }
    display.write(248);
    display.print("C");
    display.println("");

    display.print("Sunhouse: ");
    if (packet.greenhouse_air_temp > -10000) {
      display.print(packet.greenhouse_air_temp / 100);
    } else {
      display.print("--");
    }
    display.write(248);
    display.print("C ");

    if (packet.greenhouse_soil_hum >= 0) {
      display.print(packet.greenhouse_soil_hum);
    } else {
      display.print("----");
    }

    display.println("");

    if (packet.greenhouse_soil_hum >= 103) {
      display.print((float)(packet.greenhouse_water_press * 1.4662757 - 150));
    } else {
      display.print("-.--");
    }
    display.print("b ");

    if (packet.greenhouse_door_state == 0) {
      display.print("D0");
    } else if (packet.greenhouse_door_state == 1) {
      display.print("D1");
    } else {
      display.print("D-");
    }
    if (packet.greenhouse_relay_state == 0) {
      display.print("V0");
    } else if (packet.greenhouse_relay_state == 1) {
      display.print("V1");
    } else {
      display.print("V-");
    }

    display.println("");
    display.display();

    packet.my_uptime = millis()/60000;

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
  digitalWrite(LED_BUILTIN, HIGH);
  delay(20);
  #ifdef DEBUG_MB
  for (uint8_t j=0; j<len;j++) {
    softSerial.print(au8Buffer[j], HEX);
    softSerial.print(" ");
  }
  softSerial.println();
  #endif;
  Serial.write(au8Buffer, len);
  delay(20);
}

bool WaitRespose() {
  uint8_t len;
  digitalWrite(SERIAL_PIN, LOW);
  digitalWrite(LED_BUILTIN, LOW);
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

  if (Serial.available()) {
    byte c = Serial.read();
    telegramStart = millis();
    #ifdef DEBUG_MB
    softSerial.print(c, HEX);
    softSerial.print(" ");
    softSerial.print(byteCnt);
    softSerial.print(" ");
    softSerial.println(telegramStep);
    #endif
    switch(telegramStep) {
      case TELEGRAM_ADDR:
        address = au8Buffer[i++] = c;
        telegramStep = TELEGRAM_FUNC;
        #ifdef DEBUG_MB
        softSerial.print("TELEGRAM_ADDR ");
        softSerial.println(c, HEX);
        #endif
        return 0;
        break;
      case TELEGRAM_FUNC:
        function = au8Buffer[i++] = c;
        telegramStep = TELEGRAM_BYTE;
        #ifdef DEBUG_MB
        softSerial.print("TELEGRAM_FUNC ");
        softSerial.println(c, HEX);
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
        softSerial.print("TELEGRAM_ERRO ");
        softSerial.println(c, HEX);
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
        softSerial.print("TELEGRAM_BYTE ");
        softSerial.println(c, HEX);
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
        softSerial.print("TELEGRAM_BYHI ");
        softSerial.println(c, HEX);
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
        softSerial.print("TELEGRAM_BYLO ");
        softSerial.println(c, HEX);
        #endif
        return 0;
        break;
      case TELEGRAM_CRHI:
        au8Buffer[i+1] = c;
        telegramStep = TELEGRAM_CRLO;
        #ifdef DEBUG_MB
        softSerial.print("TELEGRAM_CRHI ");
        softSerial.println(c, HEX);
        #endif
        return 0;
        break;
      case TELEGRAM_CRLO:
        {
          au8Buffer[i+2] = c;
          telegramStep = TELEGRAM_ADDR;
          #ifdef DEBUG_MB
          softSerial.print("TELEGRAM_CRLO ");
          softSerial.println(c, HEX);
          #endif

          uint8_t len = i;
          uint16_t crc = 0;
          i = 0;

          crc = au8Buffer[len+1] << 8;
          crc |= au8Buffer[len+2];
          if (crc != CalculateCRC(len)) {
            #ifdef DEBUG_MB
            softSerial.print(" WRONG CRC ");
            softSerial.print(word(au8Buffer[len+1], au8Buffer[len+2]), DEC);
            softSerial.print(CalculateCRC(len), DEC);
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
