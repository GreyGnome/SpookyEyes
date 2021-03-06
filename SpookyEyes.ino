//#include <EnableInterrupt.h>
// Using vim: :set ts=2 sts=2 sw=2 et ff=unix

#include <EEPROM.h>
#include <stdbool.h>


// This one is used in the Arduino IDE...
// From damellis' work:
// ATMEL ATTINY84 / ARDUINO
//
//                           +-\/-+
//                     VCC  1|    |14  GND
//             (D 10)  PB0  2|    |13  AREF (D  0)
//             (D  9)  PB1  3|    |12  PA1  (D  1) 
//                     PB3  4|    |11  PA2  (D  2) 
//  PWM  INT0  (D  8)  PB2  5|    |10  PA3  (D  3) 
//  PWM        (D  7)  PA7  6|    |9   PA4  (D  4) 
//  PWM        (D  6)  PA6  7|    |8   PA5  (D  5)        PWM
//                           +----+
//  USBasp connections:
//  MOSI = D6 (pin 7), MISO=D5 (pin 8), SCK=D4 (pin 9), RST=pin 4

//#ifdef EI_ATTINY24
#define LEFT_EYEBALL 6   // == 
#define RIGHT_EYEBALL 5  // == 
#define ON_OFF 0        // pin with light-sensitive resistor on it

// --- DEBUG DEBUG DEBUG DEBUG D--vvvv--UG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG
// --- DEBUG DEBUG DEBUG DEBUG D--vvvv--UG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG
#undef DEBUG
// --- DEBUG DEBUG DEBUG DEBUG D--^^^^--UG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG
// --- DEBUG DEBUG DEBUG DEBUG D--^^^^--UG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG

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

// Make the eyes go bright and dim.
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
      delay(delay_time);               // wait for a time
      left_eyeball(255); right_eyeball(255);
      delay(delay_time);               // wait for a time
    }
    left_eyeball(previous_level); right_eyeball(previous_level);
}

int array_size;
unsigned long currentMillis = 0;
unsigned long nowMillis = 0;

uint8_t eeprom_time = 0;
// During setup(), we flash the LEDs to indicate system state. There are 3 phases, with a delay of 1 second between each:
// 1. Turn both eyes on for 1 second.
// 2. Show the chip speed. Flash 8 times quickly for 8 MHz, 3 times quickly for 1 Mhz (CLKPR register == 3).
void setup() {
  if (EEPROM.read(0) == 0xFF) EEPROM.write(0, eeprom_time); // INITIAL SETUP ONLY; EEPROM's cells are reset to 255 when you upload the sketch.
  uint8_t i = 0;
  pinMode(LEFT_EYEBALL, OUTPUT);
  pinMode(RIGHT_EYEBALL, OUTPUT);
  pinMode(ON_OFF, INPUT);
  both_eyeballs(255); // Hello! :-)  #BK.Hello
  delay(1000);
  both_eyeballs(0);
  delay(1000);
  // How to change your chip's clock speed, using the internal RC oscillator, from the default (1 MHz) to 8 MHz?
  // Here you go. This is here for tutorial purposes. Remember, if you change this, you have to tell the Arduino IDE
  // that the new speed is 8 MHz. This way it will set proper values in, for example, the delay() function.
  // Beware that higher speed draws higher current (given below). Not good for battery operated stuff.
  // Slowing the chip down to less that 1 MHz is NOT RECOMMENDED! You may make it very difficult to program your chip!!
  // Using values of other than 0x00 (8 MHz), 0x01 (4 MHz), 0x02 (2 MHz), or 0x03 (1 MHz) could effectively brick your chip>
  uint8_t oldSREG = SREG; // save interrupt state (we don't want to assume they're already off ===>>> unknown assumptions == Bad Programmer.)
  cli();        // shut off interrupts
  CLKPR = 0x80; // You need to set this in order to change the chip frequency. From the docs: "Within 4 cycles, write the desired value to CLKPS..."
  // So now we write the desired value to CLKPS:
  // CLKPR = 0x00; // Sets it to 8MHz. This uses 2.45 mA for the chip only. Uncomment this and comment the next line, if you want.
  CLKPR = 0x03; // This is the default anyway. Uses 0.83 mA for the chip only. Happy. :-) This is low enough for battery operation.
  SREG = oldSREG;
  switch (CLKPR) {
    case 0x00:
      flash (8, 200, 0);          // Flash 8 times for 8 MHz
      break;
    default:
      flash (CLKPR, 500, 0); // Dy default, flashes 3 times #BK.check_time
      break;
  }
  delay(1000);
  switch (EEPROM.read(0)) {
      case 0xFF:
        flash (6, 200, 0);
        break;
      default:
        flash (2, 200, 0);
        break;
  }
  delay(1000);
#ifdef DEBUG
  currentMillis = millis(); // Since time is shortened, it could be confusing because we have
                            // already consumed a significant time in the setup()
#endif
}

uint8_t OFF=0, ON=1;

unsigned long delta_time = 0;

uint8_t is_light = false, light_ran_tonight = false;
int looper = 0;
#ifndef DEBUG
const unsigned long HOUR_millis = 3600000;
const uint8_t TOTAL_RUN_HOURS   = 40;     // Then it shuts off (until you reprogram it)! Notice it must be less than 255.
#else
// --- DEBUG DEBUG DEBUG DEBUG D--vvvv--UG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG
const unsigned long HOUR_millis = 10000; // DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG
const uint8_t TOTAL_RUN_HOURS   = 100;   // Then it shuts off (until you reprogram it)!
// --- DEBUG DEBUG DEBUG DEBUG D--^^^^--UG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG
#endif
// If we run 4 hours max and our total run hours is 40... that's 10 days!
const uint8_t MAX_RUNTIME = 4; // hours daily, assuming HOUR_millis == 3600000.
uint8_t latch = OFF;
uint8_t latch_time_on = 0;
uint8_t latch_time_off = MAX_RUNTIME + 1; // We start by having been off for a long time.

void loop() {   // #BK.loop
  uint8_t levels[] = {5, 20, 100, 180};
  uint8_t i=0;

  nowMillis = millis();
  delta_time = nowMillis - currentMillis; // currentMillis always starts at 0.
  //
  // Make sure we only run for TOTAL_RUN_HOURS
  //
  if (eeprom_time > TOTAL_RUN_HOURS) {
    // Comment this out if you want it to be completely silent after reaching TOTAL_RUN_HOURS.
    flash(3, 200, 0);
    delay(10000);
    return;
  }
  //
  // Time management: After HOUR_millis time, perform time management.
  //
  // Count the hours we've been on with "latch_time_on", or the hours we've been off with "latch_time_off".
  // The problem is: At dusk, the light may be variable. In one moment, it may be dark enough to turn
  // on but the next moment light enough to think it should be off. So: Once it gets dark enough to
  // turn on, stay on for some hours. Once it gets light enough to turn off, stay off for some hours.
  //
  if (delta_time >= HOUR_millis) { // An hour has passed
    currentMillis = nowMillis - (delta_time - HOUR_millis);
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
    flash(3, 200, 0);      // #BK.time_management
    if (latch) latch_time_on++; // increment every hour
    else latch_time_off++;
  }
  if (latch) {                      // it's running.
    if (latch_time_on > MAX_RUNTIME) {   // if we've run long enough, turn off
      latch = OFF;                       // #BK.turn_spookiness_off
      latch_time_off = 0;
    } else {                        // we've not been on long, continue running
    }
  }
  else {                            // it's not running
    if (latch_time_off > 1 ) {  // We've been off for over 1 hour.
      // #BK.check_the_light
      is_light = ! digitalRead(ON_OFF); // digitalRead == 0 ---->>> light outside.
      if ((! is_light) && (latch_time_on == 0)) {  // if it's dark and it had been light, turn
        latch = ON;                              // on the latch
      } else {                                   // it's light outside
        latch_time_on = 0;                       // flag that says we haven't been on yet today
      }
    } else {                        //  we've not been off long, do nothing
    }
  }
  if (latch) {
    //
    // HERE'S THE SPOOKY STUFF
    //
    if (latch_time_on <= MAX_RUNTIME) {
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
        spookyEyeballs(20, 255); // go up and down for a while
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
    flash(EEPROM.read(0), 100, 0); // #BK.indicate_duration
    delay(100);
    /* both_eyeballs(0); */
    // delay(5000);
#endif
  }
#ifdef DEBUG
  delay(2000);
#else
  delay(20000);
#endif
}

//////////// DEPRECATED //////////////////////////////////////////
// Old pins: (where did I get this???)
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
