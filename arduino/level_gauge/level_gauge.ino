#define ANALOG_PIN A0
#define SERIAL_PIN 2
#define LED_PIN 13
//#define DEBUG

void setup()
{
  Serial1.begin(9600);
  while (!Serial1);

  #ifdef DEBUG
  Serial.begin(9600);
  while (!Serial);
  #endif
  
  pinMode(SERIAL_PIN, OUTPUT);
  digitalWrite(SERIAL_PIN, HIGH);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
}
 
void loop()
{
  Serial1.println(analogRead(ANALOG_PIN));
  #ifdef DEBUG
  Serial.println(level);
  #endif
  blinkOk();
  delay(300);
}

void blinkOk()
{
  digitalWrite(LED_PIN, HIGH);
  delay(200);
  digitalWrite(LED_PIN, LOW);
  delay(50);
  digitalWrite(LED_PIN, HIGH);
  delay(200);
  digitalWrite(LED_PIN, LOW);
  delay(50);
  digitalWrite(LED_PIN, HIGH);
  delay(200);
  digitalWrite(LED_PIN, LOW);
  delay(100);

  digitalWrite(LED_PIN, HIGH);
  delay(200);
  digitalWrite(LED_PIN, LOW);
  delay(50);
  digitalWrite(LED_PIN, HIGH);
  delay(80);
  digitalWrite(LED_PIN, LOW);
  delay(50);
  digitalWrite(LED_PIN, HIGH);
  delay(200);
  digitalWrite(LED_PIN, LOW);
}
