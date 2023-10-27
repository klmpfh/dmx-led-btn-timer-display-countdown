#include <DMXSerial.h>


#include <FastLED.h>

FASTLED_USING_NAMESPACE

#define DATA_PIN    12
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS    60
CRGB leds[NUM_LEDS];


#include <Arduino.h>
#include <TM1637TinyDisplay.h>

// Define Digital Pins
#define CLK 6
#define DIO 7

// https://github.com/jasonacox/TM1637TinyDisplay/blob/master/TM1637TinyDisplay.cpp
// https://github.com/jasonacox/TM1637TinyDisplay/blob/master/examples/TM1637-Countdown/TM1637-Countdown.ino

// Instantiate TM1637TinyDisplay Class
TM1637TinyDisplay display(CLK, DIO);

uint8_t dots = 0b01000000; // Add dots or colons (depends on display module)

const byte displayBrightness = 2;





// program states
byte current_state = 0;
unsigned long state_since = 0;
unsigned long state_for = 0;



// btn stuff
const int btn_pin = A0;
bool btn_was = false;
bool btn_is = false;

// mistake blink
unsigned long lastMistake = 0;

// strobos
const int dmx_start = 1;
const byte dmx[2][5] = {
  {0, 0, 0, 0, 0},
  {255, 255, 255, 255, 230}
};


void setup() {

  pinMode(btn_pin, INPUT_PULLUP);

  DMXSerial.init(DMXController);


  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 1000);
  // Initialize Display
  display.begin();
  display.setBrightness(displayBrightness, true);

  display.showString("load");

  // start with booting
  current_state = 0;
  state_since = millis();
  state_for = 3000;

  dmx_low();
}

int count_btns = 0;

void refreshAllStuff() {

  // LEDs
  FastLED.show();

  // read btn
  btn_is = digitalRead(btn_pin) == HIGH ? true : false;

  if ( btn_is != btn_was) {

    btn_was = btn_is;

    if (btn_is) {
      // btn pushed
      count_btns++;

      // what ist todo
      switch (current_state) {
        case 1:
          state_for = 1; // change on next loop
          break;
        default:
          lastMistake = millis();
      }
    }
  }

  delay(2);
}

void nextStateControll() {

  /*
    0 - booting - 5000
    1 - ready - unending
    2 - countdown - 5 * 1000
    3 - flash - 3 * 1000
    4 - cooldown - 3 * 60 * 1000
  */

  // set next state
  if (state_for > 0 && state_since + state_for < millis() ) {
    switch (current_state) {
      case 0: // from booting
      case 4: // from cooldown
        current_state = 1; // to ready
        state_since = millis();
        state_for = 0UL;
        break;
      case 1: // from reday
        current_state = 2; // to countdown
        state_since = millis();
        state_for = 5UL * 1000UL;
        break;
      case 2: // from countdown
        current_state = 3; // to flash
        state_since = millis();
        state_for = 3UL * 1000UL;
        break;
      case 3: // from flash
        current_state = 4; // to cooldown
        state_since = millis();
        state_for = 1UL * 60UL * 1000UL; // should 3 mins
        break;
    }
  }
}

void dmx_low() {
  DMXSerial.write(1, 0);
  DMXSerial.write(2, 0);
  DMXSerial.write(3, 0);
  DMXSerial.write(4, 0);
  
  DMXSerial.write(5, 0);
  DMXSerial.write(6, 0);
  DMXSerial.write(7, 0);
  DMXSerial.write(8, 0);
}
void dmx_high() {
  DMXSerial.write(1, 255);
  DMXSerial.write(2, 95);
  DMXSerial.write(3, 255);
  DMXSerial.write(4, 255);
  DMXSerial.write(5, 255);
  DMXSerial.write(6, 120);
  DMXSerial.write(7, 255);
  DMXSerial.write(8, 0);
}

void handlDMX() {

  switch (current_state) {
    case 0:
    case 3: return dmx_high();
    default: return dmx_low();
  }

}

void runBoot() { // state 0

  // fill_solid(leds, map(millis(), state_since, state_since + state_for, 0, NUM_LEDS), CRGB(255, 255, 255));
  for (byte l = 0; l < 60; l++) leds[l].fadeToBlackBy( 2 );
  leds[map(current_millis() % 1000, 0, 1000, 0, NUM_LEDS)] = 0xFFFFFF;

  // display.showString("boot");
  //display.showNumber(current_state);


}

void runReady() {
  display.showString("push");
  for (byte l = 0; l < 60; l++) {
    leds[l].fadeToBlackBy( 8 );
  }
  leds[map(current_millis() % 2000, 0, 2000, 0, NUM_LEDS)].setHue(map(millis() % 3333, 0, 3333, 0, 255));
  addGlitter(38, CRGB(255, 255, 255));

}

void runCountdown() {
  display_time();
  if ((current_millis() / 100) % 10 == 0) return fill_solid(leds, NUM_LEDS, CRGB(255, 255, 255));
  for (byte l = 0; l < 60; l++) leds[l].fadeToBlackBy( 64 );
}

void runFlash() {
  display_time();
  fill_solid(leds, NUM_LEDS, CRGB(0, 0, 0));
}

void runCooldown() {

  display_time();
  // fill_rainbow_circular(leds, NUM_LEDS, (millis() / 20) % 255, false);
  for (byte l = 0; l < 60; l++) leds[l].fadeToBlackBy( 8 );


  fill_solid(leds,map(millis(), current_start() , current_end(), 60,0),CRGB(16,0,0));
  
  //addGlitter(32, CRGB(32, 0, 0));

}

void runMistake() {

  display.showString("wait");
  fill_solid(leds, NUM_LEDS, CRGB(16, 0, 0));

}

unsigned long current_start() {
  return state_since;
}
unsigned long current_end() {
  return state_since + state_for;
}
unsigned long current_millis() {
  return current_start() - millis();
}
unsigned long current_millis_left() {
  return current_end() - millis();
}

void addGlitter( fract8 chanceOfGlitter, CRGB color) {
  if ( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += color;
  }
}

void display_time() {
  if (current_millis_left() < 0) return;
  uint8_t dot = (current_millis_left() / 500) % 2 == 0 ? dots : 0;
  unsigned long humanSecoundsLeft = (current_millis_left() / 1000) + 1;
  display.showNumberDec(humanSecoundsLeft / 60, dot, true, 2, 0);
  display.showNumberDec((humanSecoundsLeft % 60), dot, true, 2, 2);
}

void loop() {

  refreshAllStuff();

  nextStateControll();

  handlDMX();

  if (lastMistake > millis() - 500) return runMistake();

  switch (current_state) {
    case 0: return runBoot();
    case 1: return runReady();
    case 2: return runCountdown();
    case 3: return runFlash();
    case 4: return runCooldown();
    default: return fill_solid(leds, NUM_LEDS, CRGB(255, 255, 255));
  }



}
