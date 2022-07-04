// Lightsaber
//
// Core logic for the EROYAN.tech Lightsaber
// Copyright 2022 EROYAN.tech
// Licensed under MIT license
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
// to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
// IN THE SOFTWARE.
//

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

// DEFINES
// This section defines the constants needed for the lightsaber
//
#define NUM_LEDS        8             // Eight for the test strip; 60 for the full blade
#define CONTROL_PIN     12            // Pin used to control the NeoPixel strip - D12/GPIO12
#define BUTTON_PIN      13            // Pin used as input for the control button - D13/GPIO13
#define LED_PIN         11            // Pin used to power the LED on the button - D11/GPIO11
#define LED_BRIGHTNESS  85            // Brightness - in a range of 0-255, so 85 is about 1/3
#define BTN_TIMER       1000          // number of milliseconds to delay after a button is hit before interrupts are re-enabled.  
#define LED_SPEED       100           // Time in ms between led's when extending or retracting

#define CRITICAL_START  noInterrupts(); // Disable interrupts for critical section - i.e. changing pixelse
#define CRITICAL_STOP   interrupts();   // re-enable interrupts
/*
 * Colors are 32-bit number defined in four bytes - 0xWWRRGGBB  Code uses bit-shifting to determine it.
 * Examples are:
 * 0x00FF0000 = RED
 * 0x0000FF00 = GREEN
 * 0x000000FF = BLUE
 * 0x00FFF600 = Cadmium Yellow
 * 0x00FFFFFF = WHITE
 * 0x0087CEEB = Sky Blue
 * 0x0000FFFF = Cyan Blue
 */
#define LED_RED   0x00FF0000
#define LED_GREEN 0x0000FF00
#define LED_BLUE  0x000000FF
#define LED_YELW  0x00FFF600
#define LED_WHITE 0x00FFFFFF
#define LED_SKBLU 0x0087CEEB
#define LED_CYAN  0x0000FFFF
#define LED_OFF   0x00000000

// Defines the state of the lightsaber.  
//  Starts in Off. 
//  One button push moves to Idle (enabling the button LED).
//  Second button push moves to powerup, which engages the lightsaber sound and extends the blade
//    Automatically moves to "on" when complete.
//  Clash is set when impact is felt.  Only triggers if it is in "on" state and returns to "on" when complete\
//  Next button push goes to powerdown, which engages the power off sound & blade controls, then goes to Idle
//  Ideally, get timer going so that have to hold it for 5 seconds to power off.
// Button progression is only from off->powerdown.
enum saberState {
  off,
  idle,
  powerup,
  on,
  powerdown,
  blademove,
  clash
};

// Blade object.  Controls the NeoPixels
Adafruit_NeoPixel Blade(NUM_LEDS, CONTROL_PIN, NEO_GRB + NEO_KHZ800);

// saber State - indicates what state the blade is in
volatile enum saberState buttonState = off;
volatile enum saberState nextState = off;
// TEST CODE
volatile int toggle = 0;
void togglebutton() {
    if (toggle == 0) {
        toggle = 1;
        digitalWrite(LED_PIN, HIGH);
    }
    else {
        toggle = 0;
        digitalWrite(LED_PIN, LOW);
    }
}
// END TEST CODE

// Blade color.
int bladeColor = LED_GREEN;

// Detects the push of a button and moves the state along.
void buttonPush() {
  switch (buttonState)
  {
    case off:
      nextState = idle;
      break;
    case idle:
      nextState = powerup;
      break;
    case powerup:
      nextState = on;
      break;
    case on:
      nextState = powerdown;
      break;
    case powerdown:
      nextState = off;
      break;
    default:
      break;
  }
}

// This function will accept the state that the blade is moving to, and control the blade
// and sound accordingly.
// Called in the Loop to keep the interrupt code as lightweight as possible.
//
enum saberState changeState(enum saberState nextState) {
  buttonState = nextState;
  switch (nextState) {
    case off:
      // Blade has already gone through powerdown and is now essentially at idle.
      // Shut off button led if it is on.
      digitalWrite(LED_PIN, LOW);
      CRITICAL_START
      Blade.clear();
      Blade.show();
      delay(BTN_TIMER);
      CRITICAL_STOP
      break;
    case idle:
      // Blade was in 'off'.  Enable the button LED
      digitalWrite(LED_PIN, HIGH);
      CRITICAL_START
      Blade.clear();
      Blade.show();
      delay(BTN_TIMER);
      CRITICAL_STOP
      break;
    case powerup:
      // Extend blade  - pause between pixels for LED_SPEED ms
      CRITICAL_START
      for (int i = 0; i < NUM_LEDS; i++) {
        Blade.setPixelColor(i, bladeColor);
        Blade.show();
        delay(LED_SPEED);
      }
      delay(BTN_TIMER);
      CRITICAL_STOP
      nextState = on; // Auto advance
      break;
    case on:
      break;
    case powerdown:
      CRITICAL_START
      for (int i = NUM_LEDS-1; i >= 0; --i) {
        Blade.setPixelColor(i, LED_OFF);
        Blade.show();
        delay(LED_SPEED);
        nextState = off; // Auto advance
      }
      delay(BTN_TIMER);
      CRITICAL_STOP
      break;
    case blademove:
      break;
    case clash:
      break;
    default:
      break;
  }
  return nextState;
}

// TEST CODE
int led = LED_BUILTIN;
// END TEST CODE

// Initial setup.  Initialize blade, configure pins, set up interrupt and set the brightness to manage power usage.
void setup() {
  Blade.begin();  // Initialize neopixel library
  pinMode(BUTTON_PIN, INPUT_PULLUP); // Button controller - this is the interrupt pin
  pinMode(LED_PIN, OUTPUT);
  //attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonPush, RISING);
  // TEST CODE
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), togglebutton, RISING);
  // END TEST CODE
  //changeState(off);
  // TEST CODE
  analogWrite(led, 240);
  // END TEST CODE
  Blade.setBrightness(LED_BRIGHTNESS);

}

// Main loop - based on mode, do something here.
void loop() {
  // put your main code here, to run repeatedly:
  //if (nextState != buttonState) {
    //nextState = changeState(nextState);
  //}
  analogWrite(led, 240);
  for (int i = 0; i < NUM_LEDS; i++) {
    Blade.setPixelColor(i, bladeColor);
    Blade.show();
    delay(LED_SPEED);
  }
  delay(500);
  analogWrite(led, 0);
        Blade.clear();
      Blade.show();
  delay(500);


}
