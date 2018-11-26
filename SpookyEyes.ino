//#include <EnableInterrupt.h>

#include <EEPROM.h>
#include <stdbool.h>

// Here are the pins:
//#if defined( __AVR_ATtinyX4__ )
// ATMEL ATTINY84 / ARDUINO
//
//                           +-\/-+
//                     VCC  1|    |14  GND
//             (D  0)  PB0  2|    |13  AREF (D 10)
//             (D  1)  PB1  3|    |12  PA1  (D  9)
//                     PB3  4|    |11  PA2  (D  8)
//  PWM  INT0  (D  2)  PB2  5|    |10  PA3  (D  7)
//  PWM        (D  3)  PA7  6|    |9   PA4  (D  6)
//  PWM        (D  4)  PA6  7|    |8   PA5  (D  5)        PWM
//                           +----+

//#ifdef EI_ATTINY24
#define LEFT_EYEBALL 4   // == 
#define RIGHT_EYEBALL 5  // == 
#define ON_OFF 10        // pin with light-sensitive resistor on it

static inline void left_eyeball(int level) {
    analogWrite(LEFT_EYEBALL, level);
}

static inline void right_eyeball(int level) {
    analogWrite(RIGHT_EYEBALL, level);
}

static inline void both_eyeballs(int level) {
    analogWrite(LEFT_EYEBALL, level);
    analogWrite(RIGHT_EYEBALL, level);
}

static inline void fadeInBothEyeballs(uint8_t top) {
  uint8_t step = 4, i=0, tmpstep;

  tmpstep = 1;
  for ( ; i < top - step; i+=tmpstep) {
    if (i > 80) tmpstep = step;
    both_eyeballs(i);
    delay(50);
  }
  delay(50);
  both_eyeballs(top);
}

static inline void fadeOutBothEyeballs(uint8_t start) {
  uint8_t step = 8, i=0, tmpstep;

  if (i < 80) tmpstep = 2; else tmpstep = step;
  for (uint8_t i=start; i > step; i-=tmpstep) {
    if (i < 80) tmpstep = 2;
    both_eyeballs(i);
    delay(50);
  }
  both_eyeballs(0);
}

// Make the eyes go up and down.
static inline void spookyEyeballs(uint8_t bottom, uint8_t top) {
  uint8_t step = 6, i=0;

  for (i=bottom ; i < top - step; i+=step) {
    both_eyeballs(i);
    delay(50);
  }
  both_eyeballs(top);
  delay(50);

  for (i=255; i >= bottom; i-=step) {
    both_eyeballs(i);
    delay(50);
  }
}

void flash(uint8_t times, int delay_time, uint8_t previous_level) {
    int i = 0;
    for (i=0; i < times; i++) {
      left_eyeball(0); right_eyeball(0);
      delay(delay_time);               // wait for a second
      left_eyeball(255); right_eyeball(255);
      delay(delay_time);               // wait for a second
    }
    left_eyeball(previous_level); right_eyeball(previous_level);
}

int array_size;
unsigned long currentMillis = 0;
unsigned long nowMillis = 0;

uint8_t eeprom_time = 0;
void setup() {
  if (EEPROM.read(0) == 0xFF) EEPROM.write(0, eeprom_time); // INITIAL SETUP ONLY
  uint8_t check_time = 0;
  pinMode(LEFT_EYEBALL, OUTPUT);
  pinMode(RIGHT_EYEBALL, OUTPUT);
  pinMode(ON_OFF, INPUT);
  uint8_t oldSREG = SREG;
  cli();
  CLKPR = 0x80; // Enables us to perform the following...
  // CLKPR = 0x00; // This is 8MHz. This uses 2.45 mA for the chip only.
  CLKPR = 0x03; // This is the default anyway. Here for tutorial purposes. 1MHz. Uses 0.83 mA for the chip only.
  SREG = oldSREG;
  check_time = CLKPR;
  switch (check_time) {
    case 0x00:
      flash (8, 200, 0);
      break;
    case 0xFF:
      flash (6, 200, 0);
      break;
    case 0x09:
      flash (1, 200, 0);
      EEPROM.write(0, 0xFF);
      break;
    default:
      flash (check_time, 500, 0);
      break;
  }
  delay(1000);
  check_time = EEPROM.read(0);
  switch (check_time) {
      case 0xFF:
        flash (6, 200, 0);
        break;
      default:
        flash (2, 200, 0);
        break;
  }
  delay(1000);
}

uint8_t OFF=0, ON=1;

unsigned long d_time = 0;

// --- DEBUG DEBUG DEBUG DEBUG D--vvvv--UG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG
// --- DEBUG DEBUG DEBUG DEBUG D--vvvv--UG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG
#undef DEBUG
// --- DEBUG DEBUG DEBUG DEBUG D--vvvv--UG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG
// --- DEBUG DEBUG DEBUG DEBUG D--vvvv--UG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG

uint8_t is_light = false, light_ran_tonight = false;
int looper = 0;
#ifndef DEBUG
const unsigned long HOUR_millis = 3600000;
const uint8_t TOTAL_RUN_HOURS = 4; // Then it shuts off!!!!
#else
// --- DEBUG DEBUG DEBUG DEBUG D--vvvv--UG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG
const unsigned long HOUR_millis = 10000; // DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG
const uint8_t TOTAL_RUN_HOURS = 100; // Then it shuts off!!!!
// --- DEBUG DEBUG DEBUG DEBUG D--^^^^--UG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG
#endif
const uint8_t MAX_RUNTIME = 4; // hours, assuming HOUR_millis == 3600000
uint8_t latch = OFF;
uint8_t latch_on = 0;
uint8_t latch_off = MAX_RUNTIME + 1; // We start by having been off for a long time.

void loop() {
  uint8_t levels[] = {5, 20, 100, 180};
  uint8_t i=0;

  nowMillis = millis();
  d_time = nowMillis - currentMillis;
  // Comment this out if you want it to be completely silent after reaching
  // TOTAL_RUN_HOURS.
  if (eeprom_time > TOTAL_RUN_HOURS) {
    flash(3, 200, 0);
    delay(10000);
    return;
  }
  if (d_time >= HOUR_millis) {
    currentMillis = nowMillis - (d_time - HOUR_millis);
    eeprom_time = EEPROM.read(0); // This is the number of hours we've been running.
    eeprom_time++;
    EEPROM.write(0, eeprom_time);
    // hysteresis latch works like this:
    // latch starts "off"
    // check to see if it's dark AND it WAS light.
    // if it is, run for 4 hours without checking the light.
    //     then turn off the lights and stay off
    // check the light.
    // reset the latching mechanism once it goes light.
    flash(3, 200, 0);
    if (latch) latch_on++; // increment every hour
    else latch_off++;
  }
  if (latch) {                      // it's running
    if (latch_on > MAX_RUNTIME) {   // if we've run long enough, turn off
      latch = OFF;
      latch_off = 0;
    } else {                        // we've not been on long, continue running
    }
  }
  else {                            // it's not running
    if (latch_off > 1 ) {  // We've been off for only 1 hour.
      is_light = ! digitalRead(ON_OFF); // digitalRead == 0 ---->>> light outside.
      if (! is_light) {             // if it's dark, turn on the latch
        latch = ON;
        latch_on = 0;
      }
    } else {                        //  we've not been off long, do nothing
    }
  }
  if (latch) {
    // Need code here to run for 4 hours.
    if (latch_on <= MAX_RUNTIME) {
      delay(2000);
      left_eyeball(255); delay(1500); left_eyeball(0); delay(3000);
      left_eyeball(255); delay(1000); left_eyeball(0); delay(4000);
      fadeInBothEyeballs(255); delay(5000);
      fadeOutBothEyeballs(255);
#ifndef DEBUG
      delay(3000);
      left_eyeball(10); delay(1000);
      left_eyeball(20); delay(1000);
      left_eyeball(40); delay(1000);
      right_eyeball(80); delay(4000);
      both_eyeballs(255);
      delay(3000);
      both_eyeballs(0);
      delay(2500);
      for (i=0; i < 10; i++) {
        spookyEyeballs(60, 255); // go up and down for a while
        delay(500);
      }
      while (looper < array_size) {
        both_eyeballs(levels[looper]);
        looper ++;
        delay(1000);               // wait for a second
      }
      looper=0;
      delay(1000);
      both_eyeballs(255);
      delay(5000);
      for (i=0; i < 10; i++) {
        spookyEyeballs(60, 255); // go up and down for a while
        delay(500);
      }
      both_eyeballs(0);
      delay(1000);
#endif
      // ############## END OF "ON" LOOP ################
    }
  }
  else {
#ifdef DEBUG
    flash(EEPROM.read(0), 100, 0);
    /* both_eyeballs(0); */
    // delay(5000);
#endif
  }
#ifdef DEBUG
  delay(1000);
#else
  delay(20000);
#endif
}
