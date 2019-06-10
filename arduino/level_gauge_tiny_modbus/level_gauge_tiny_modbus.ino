#include <ModbusKostin.h>

#define GAUGE_PIN A2
#define SERIAL_RX 2
#define SERIAL_TX 0
#define DIR_PIN 1

#define ADDRESS 0x0A

#define ANALOG_DELAY 1000

SoftwareSerial softSerial(SERIAL_RX, SERIAL_TX); // RX, TX
ModbusKostin modbus(ADDRESS, &softSerial, DIR_PIN);

uint16_t analogValue = 0;
uint32_t lastAnalogRead = 0;
uint16_t dist_3[3] = {0, 0, 0};   // массив для хранения трёх последних измерений
uint16_t middle, dist;

void setup()
{
  modbus.begin(38400);
  modbus.setRegisterLimits(1, 2);
}

void loop() {
  uint8_t len = 0;
  float k;
  static byte i;
  byte delta;

  modbus.poll();

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

    modbus.setRegisterValue(1, millis() / 60000);
    modbus.setRegisterValue(2, analogValue);
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
