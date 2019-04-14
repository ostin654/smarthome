#include <SoftwareSerial.h>

#define GAUGE_PIN A2
//#define LED_PIN 2
#define SERIAL_RX 1
#define SERIAL_TX 2
#define DELAY 100

SoftwareSerial softSerial(SERIAL_RX, SERIAL_TX); // RX, TX
int dist_3[3] = {0, 0, 0};   // массив для хранения трёх последних измерений
int middle, dist, dist_filtered;
float k;
byte i, delta;
unsigned long sensTimer;

void setup()
{
  softSerial.begin(9600);
  //while (!softSerial);
  Serial.begin(9600);
  while (!Serial);
  delay(1000);
  //pinMode(LED_PIN, OUTPUT);
}
 
void loop()
{
  int raw = 0;

  //digitalWrite(LED_PIN, LOW);
  //delay(10);

  if (millis() - sensTimer > DELAY) {
    // счётчик от 0 до 2
    // каждую итерацию таймера i последовательно принимает значения 0, 1, 2, и так по кругу
    if (i > 1) i = 0;
    else i++;

    raw = dist_3[i] = analogRead(GAUGE_PIN);                      // получить сигнал в текущую ячейку массива
    dist = middle_of_3(dist_3[0], dist_3[1], dist_3[2]);    // фильтровать медианным фильтром из 3ёх последних измерений

    delta = abs(dist_filtered - dist);                      // расчёт изменения с предыдущим
    if (delta > 20) k = 0.7;                                // если большое - резкий коэффициент
    else k = 0.1;                                           // если маленькое - плавный коэффициент

    dist_filtered = dist * k + dist_filtered * (1 - k);     // фильтр "бегущее среднее"
    softSerial.println(dist_filtered);                      // вывести
    //Serial.println(dist_filtered);                        // вывести
    //Serial.println(raw);                                  // вывести

    sensTimer = millis();                                   // сбросить таймер
  }

  //digitalWrite(LED_PIN, HIGH);
  //delay(10);
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
