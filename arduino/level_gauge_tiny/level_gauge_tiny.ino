#include <SoftwareSerial.h>

#define GAUGE_PIN A2
#define SERIAL_RX 2
#define SERIAL_TX 0
#define DIR_PIN 1

#define MAX_BUFFER 64
#define T35 10

#define TELEGRAM_ADDR 0
#define TELEGRAM_FUNC 1
#define TELEGRAM_REHI 2
#define TELEGRAM_RELO 3
#define TELEGRAM_NUHI 4
#define TELEGRAM_NULO 5
#define TELEGRAM_CRHI 6
#define TELEGRAM_CRLO 7

#define ADDRESS 0x0A

#define ANALOG_DELAY 500

SoftwareSerial softSerial(SERIAL_RX, SERIAL_TX); // RX, TX

uint8_t au8Buffer[MAX_BUFFER];
uint8_t address = 0x00;
uint8_t function = 0x00;
uint16_t firstRegister = 0;
uint16_t registerNumber = 0;

uint16_t analogValue = 0;
uint32_t lastAnalogRead = 0;

uint16_t dist_3[3] = {0, 0, 0};   // массив для хранения трёх последних измерений
uint16_t middle, dist;

void setup()
{
  softSerial.begin(9600);
  pinMode(DIR_PIN, OUTPUT);
  digitalWrite(DIR_PIN, LOW);
}


void loop() {
  uint8_t len = 0;
  uint8_t j = 0;
  float k;
  static byte i;
  byte delta;

  if (len = ReceiveTelegram()) {
    len = PrepareResponse();
    digitalWrite(DIR_PIN, HIGH);
    delay(20);
    softSerial.write(au8Buffer, len);
    delay(20);
    digitalWrite(DIR_PIN, LOW);
  }

  if (lastAnalogRead + ANALOG_DELAY < millis()) {
    // счётчик от 0 до 2
    // каждую итерацию таймера i последовательно принимает значения 0, 1, 2, и так по кругу
    if (i > 1) i = 0;
    else i++;

    dist_3[i] = analogRead(GAUGE_PIN);                   // получить сигнал в текущую ячейку массива
    dist = middle_of_3(dist_3[0], dist_3[1], dist_3[2]); // фильтровать медианным фильтром из 3ёх последних измерений

    delta = abs(analogValue - dist);                     // расчёт изменения с предыдущим
    if (delta > 20) k = 0.7;                             // если большое - резкий коэффициент
    else k = 0.1;                                        // если маленькое - плавный коэффициент

    analogValue = dist * k + analogValue * (1 - k);      // фильтр "бегущее среднее"

    lastAnalogRead = millis();
  }

  //Wraparound
  if (lastAnalogRead > millis() + 1000) {
    lastAnalogRead = millis();
  }
}

// медианный фильтр из 3ёх значений
float middle_of_3(int a, int b, int c) {
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

uint8_t PrepareResponse() {
  uint8_t i = 0;
  if (function != 0x04) {
    au8Buffer[i++] = ADDRESS; // device address
    au8Buffer[i++] = function + 0x80; // function code
    au8Buffer[i++] = 0x01;
  } else if (firstRegister < 1 || firstRegister + registerNumber > 3) {
    au8Buffer[i++] = ADDRESS; // device address
    au8Buffer[i++] = function + 0x80; // function code
    au8Buffer[i++] = 0x02;
  } else {
    uint16_t len = 2;
    uint16_t upTime = millis() / 60000;

    au8Buffer[i++] = ADDRESS; // device address
    au8Buffer[i++] = 0x04; // function code
    if (firstRegister == 2) {
      len = 2;
      au8Buffer[i++] = len;
      au8Buffer[i++] = highByte(analogValue);
      au8Buffer[i++] = lowByte(analogValue);
    } else {
      if (registerNumber == 1) {
        len = 2;
        au8Buffer[i++] = len;
        au8Buffer[i++] = highByte(upTime);
        au8Buffer[i++] = lowByte(upTime);
      } else {
        len = 4;
        au8Buffer[i++] = len;
        au8Buffer[i++] = highByte(upTime);
        au8Buffer[i++] = lowByte(upTime);
        au8Buffer[i++] = highByte(analogValue);
        au8Buffer[i++] = lowByte(analogValue);
      }
    }
  }
  
  // append CRC to message
  uint16_t crc = CalculateCRC(i);
  au8Buffer[i++] = crc >> 8;
  au8Buffer[i++] = crc & 0x00ff;

  return i;
}

uint8_t ReceiveTelegram() {
  static uint8_t telegramStep = 0;
  static uint8_t i = 0;
  static uint32_t telegramStart = 0;

  if (telegramStart + T35 < millis()) {
    telegramStep = TELEGRAM_ADDR;
    i = 0;
  }

  if (softSerial.available()) {
    byte c = softSerial.read();
    telegramStart = millis();
    switch(telegramStep) {
      case TELEGRAM_ADDR:
        address = au8Buffer[i++] = c;
        telegramStep = TELEGRAM_FUNC;
        return 0;
        break;
      case TELEGRAM_FUNC:
        function = au8Buffer[i++] = c;
        telegramStep = TELEGRAM_REHI;
        return 0;
        break;
      case TELEGRAM_REHI:
        au8Buffer[i++] = c;
        telegramStep = TELEGRAM_RELO;
        return 0;
        break;
      case TELEGRAM_RELO:
        au8Buffer[i++] = c;
        firstRegister = word(au8Buffer[i-2], au8Buffer[i-1]);
        telegramStep = TELEGRAM_NUHI;
        return 0;
        break;
      case TELEGRAM_NUHI:
        au8Buffer[i++] = c;
        telegramStep = TELEGRAM_NULO;
        return 0;
        break;
      case TELEGRAM_NULO:
        au8Buffer[i++] = c;
        registerNumber = word(au8Buffer[i-2], au8Buffer[i-1]);
        telegramStep = TELEGRAM_CRHI;
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
          i = 0;
          if (ADDRESS != address) {
            return 0;
          }
          if (word(au8Buffer[len+1], au8Buffer[len+2]) != CalculateCRC(len)) {
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
  for (unsigned char i = 0; i < u8length; i++) {
    temp = temp ^ au8Buffer[i];
    for (unsigned char j = 1; j <= 8; j++) {
      flag = temp & 0x0001;
      temp >>=1;
      if (flag) {
        temp ^= 0xA001;
      }
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
