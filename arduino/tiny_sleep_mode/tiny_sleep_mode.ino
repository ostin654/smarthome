#include <avr/sleep.h>
#include <avr/wdt.h>

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

#define CYCLE 0
#define DELAY 2000
#define TRIGPIN 0

volatile boolean f_wdt = 1;
unsigned int iteration = 0;

void setup() 
  {
    pinMode(TRIGPIN, OUTPUT);
    setup_watchdog(8);// 0=16ms, 1=32ms,2=64ms,3=128ms,4=250ms,5=500ms
                      // 6=1sec, 7=2 sec, 8=4 sec, 9= 8sec
  }

void loop()
  {
    if (f_wdt == 1)
    {
      f_wdt = 0;

      iteration++;

      if (iteration > CYCLE) {
        iteration = 0;

        digitalWrite(TRIGPIN, HIGH);
        delay(DELAY);
        digitalWrite(TRIGPIN, LOW);
      }
      
      system_sleep();
    }
  }


  void system_sleep()
  {
    cbi(ADCSRA, ADEN);
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    sleep_mode();
    sleep_disable();
    sbi(ADCSRA, ADEN);
  }


  void setup_watchdog(int ii)
  {
    byte bb;
    int ww;
    if (ii > 9 ) ii = 9;
    bb = ii & 7;
    if (ii > 7) bb |= (1 << 5);
    bb |= (1 << WDCE);
    ww = bb;
    MCUSR &= ~(1 << WDRF);
    WDTCR |= (1 << WDCE) | (1 << WDE);
    WDTCR = bb;
    WDTCR |= _BV(WDIE);
  }
  ISR(WDT_vect)
  {
    f_wdt = 1;
  }
  
