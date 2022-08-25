
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

// include SPI, MP3 and SD libraries
#include <SPI.h>
#include <SD.h>
#include <Adafruit_VS1053.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif
#include <Adafruit_LSM6DSOX.h>
// #include <Adafruit_LSM6DSO32.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_I2CRegister.h>
#include <Adafruit_SPIDevice.h>
#include <Adafruit_BusIO_Register.h>

// DEFINES
// This section defines the constants needed for the lightsaber
//
#define NUM_LEDS        8             // Eight for the test strip; 60 for the full blade
#define CONTROL_PIN     12            // Pin used to control the NeoPixel strip - D12/GPIO12
#define BUTTON_PIN      25             // Pin used as input for the control button - D9/GPIO9
#define LED_PIN         11            // Pin used to power the LED on the button - D11/GPIO11
#define LED_BRIGHTNESS  85            // Brightness - in a range of 0-255, so 85 is about 1/3
#define BTN_TIMER       1000          // number of milliseconds to delay after a button is hit before interrupts are re-enabled.  
#define READY_DELAY     1000          // Number of milliseconds to delay when in Off/Ready state before a sync check.
#define POLL_DELAY      100           // Number of milliseconds to delay when in any state other than Off/Ready
#define LED_SPEED       100           // Time in ms between led's when extending or retracting
// I2C Section
#define GYRO_ADDR       0x6A          // Address of Accel/Gyro LSM6DSOX
// MusicMaker Section
#define VS1053_RESET    -1            // VS1053 reset pin (not used!)
#define VS1053_CS       8             // VS1053 chip select pin (output)
#define VS1053_DCS      10            // VS1053 Data/command select pin (output)
#define CARDCS          7             // Card chip select pin
#define VS1053_DREQ     9             // VS1053 Data request, ideally an Interrupt pin

// Here we set the critical section if we can.
#ifndef BLOCK_INTERRUPTS
    #define CRITICAL_START 
    #define CRITICAL_STOP
#else
    #define CRITICAL_START  noInterrupts(); // Disable interrupts for critical section - i.e. changing pixelse
    #define CRITICAL_STOP   interrupts();   // re-enable interrupts
#endif
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

// Sound volume
#define SOUND_VOL 2

// Defines the state of the lightsaber.  
//  Starts in Off. 
//  One button push moves to ready (enabling the button LED).
//  Second button push moves to powerup, which engages the lightsaber sound and extends the blade
//    Automatically moves to "on" when complete.
//  Clash is set when impact is felt.  Only triggers if it is in "on" state and returns to "on" when complete\
//  Next button push goes to powerdown, which engages the power off sound & blade controls, then goes to ready
//  Ideally, get timer going so that have to hold it for 5 seconds to power off.
// Button progression is only from off->powerdown.
enum saberState {
    off,  // 0
    coreboot, // 1
    procboot, // 2
    isready, // 3
    idle, // 4
    powerup, // 5
    on, // 6
    powerdown, // 7 
    blademove, // 8 Sublogic will determine speed, and whether it's spinning, which is a good trick.
    bladeclash, // 9
    clash // 10
};

enum soundState {
    ssoff,            // Default state - makes no sounds
    sspowerup,        // Plays poweron.wav - 2 seconds
    sshum,            // plays hum.wav - 30 seconds
    sspowerdown,      // Plays poweroff.wav - 2 seconds
    ssfswshhigh,      // plays hswing2.wav  - 5 seconds
    ssfswshlow,       // plays hswing1.wav  - 27 seconds
    sssswshhigh,      // plays lswing2.wav  - 5 seconds
    sssswshlow,       // Plays lswing1.wav  - 27 seconds
    ssclash,          // Plays clash1.wav - 2 seconds
    ssstab,           // Plays stab1.wav  - 3 seconds
    ssspin            // Plays spin1.wav.  Hey, it's a good trick.  2 seconds
};

/*
 * Global variable section.  This section holds hardware object variables and variables used
 * for inter-process communication
 */
// Blade object.  Controls the NeoPixels
Adafruit_NeoPixel Blade(NUM_LEDS, CONTROL_PIN, NEO_GRB + NEO_KHZ800);
// Sound object
Adafruit_VS1053_FilePlayer musicPlayer = 
    Adafruit_VS1053_FilePlayer(VS1053_RESET, VS1053_CS, VS1053_DCS, VS1053_DREQ, CARDCS);
// Gyro/Accelerometer object  
Adafruit_LSM6DSOX sox;

int led = LED_BUILTIN;


// saber State - indicates what state the blade is in
volatile enum saberState currentState = off;
volatile enum saberState priorState = off;
volatile enum saberState procZeroState = off;
volatile enum saberState procOneState = off;

volatile enum saberState buttonState = off;
volatile enum saberState nextState = off;
volatile int initComplete = 0;
volatile float old_vector = 0;

volatile enum soundState currSoundState = ssoff;  // Contains state of the soundsystem
  

/*
 * Configuration section.  This section holds global variables used to hold configuration information
 * that might be modifiable in the future
 */
// Blade color.
int bladeColor = LED_GREEN;

// Detects the push of a button and moves the state along.
void buttonPush() {
    Serial.println("Button Push Detected");
    switch (currentState)
    {
        case off:
            nextState = isready;
            Serial.println(" Next State = isready");
            break;
        case isready:
            nextState = idle;
            Serial.println(" Next State = idle");
            break;
        case idle:
            nextState = powerup;
            Serial.println(" Next State = powerup");
            break;
        case powerup:
            nextState = on;
            Serial.println(" Next State = on");
            break;
        case on:
            nextState = powerdown;
            Serial.println(" Next State = powerdown");
            break;
        case powerdown:
            nextState = idle;
            Serial.println(" Next State = off");
            break;
        default:
            nextState = currentState;
            Serial.println(" Next State = currentState");
            break;
    }
}

/*
 * This function will coordinate each thread's states and return once core1 is ready.  
 */
void syncState(int core, enum saberState syncState) {

    if (syncState == procZeroState) {
        // No need to sync - already sync'd
        return;
    }
    if (core == 0) {
        // Set core1 state to syncState
        procOneState = syncState;
        delay(POLL_DELAY);
        while(procZeroState != syncState) {
            delay(POLL_DELAY);
            Serial.print(".");
        }
        Serial.println(" ");
    }
    else { // This must be core 1 and it must be called at the END of the state function.
        procZeroState = syncState;
    }
    
    return;
}
// This function will accept the state that the blade is moving to, and control the blade
// and sound accordingly.
// Called in the Loop to keep the interrupt code as lightweight as possible.
//

enum saberState changeBladeState(enum saberState evalState) {
    enum saberState tmpState = evalState;
    if (evalState == currentState) {
        Serial.println("changeBladeState: Matching states.  Leaving");
        return(currentState); // If they pass in same state, just exit.
    }
    syncState(0, tmpState); // Make sure both threads are sync'd.
    Serial.print("changeBladeState: ");
    Serial.println(tmpState);
    switch (tmpState) {
        case off:      // 0
        case isready:  // 3 Both off and isready are really the same thing for now
            // Blade has already gone through powerdown and is now essentially at ready.
            // Shut off button led if it is on.
            digitalWrite(LED_PIN, LOW);
            CRITICAL_START
              Blade.clear();
              Blade.show();
            CRITICAL_STOP
            break;
        case idle:     // 4
            // Blade was in 'off'.  Enable the button LED
            digitalWrite(LED_PIN, HIGH);
            CRITICAL_START
              Blade.clear();
              Blade.show();
              delay(BTN_TIMER);
            CRITICAL_STOP
            break;
        case powerup: // 5
            // Extend blade  - pause between pixels for LED_SPEED ms
            CRITICAL_START
              for (int i = 0; i < NUM_LEDS; i++) {
                  Blade.setPixelColor(i, bladeColor);
                  Blade.show();
                  delay((int)(2000/NUM_LEDS));
              }
              delay(BTN_TIMER);
            CRITICAL_STOP
            //currentState = powerup;
            nextState = on; // Auto advance
            tmpState = changeBladeState(nextState);  // Recursive call
            break;
        case on:  // 6
            Serial.println("In ChangeState Section for ON");
            break;
        case powerdown:
            CRITICAL_START
              
              for (int i = NUM_LEDS-1; i >= 0; --i) {
                  Blade.setPixelColor(i, LED_OFF);
                  Blade.show();
                  delay((int)(2000/NUM_LEDS));
              }
              delay(BTN_TIMER);
            CRITICAL_STOP
            currentState = powerdown;
            nextState = off;
            tmpState = changeBladeState(nextState);  // Recursive call
          
            break;
        case blademove:
            break;
        case clash:
            break;
        default:
            break;
    }
    return tmpState;
}

/*
 * changeSoundState
 * 
 * This function will change the state of the sound based on the blade state.
 * Currently, only supports the core blade states.  In future, additional states will be added.
 */
void changeSoundState(enum saberState evalState) {
    if (evalState != currentState) {
        switch(evalState) {
            // TAG TODO
            // Here is where we put in the needed sound playing.
            default:
                break;   
        }
    }
    syncState(1, procOneState); 
    return;
}
/*
 * This function manages the sound of the lightsaber, playing the new sound upon demand
 * Future enhancement would be to randomize the sound between 1-4 and play that
 * specific wave file.
 */
void playSound(enum soundState sound) {
    Serial.print("Playing sound ");
    Serial.println(sound);
    switch(sound) {
        case ssoff:
            if (!musicPlayer.stopped()) {
                musicPlayer.stopPlaying();  // Should already be off, but just in case
                Serial.println("Stopped playing");
            }
            else {
                Serial.println("Not playing, so doing nothing");
            }
            break;
        case sspowerup:
            musicPlayer.startPlayingFile("/poweron.wav");
            break;
        case sshum:
            musicPlayer.startPlayingFile("/hum.wav");
            break;
        case sspowerdown:
            musicPlayer.stopPlaying();
            musicPlayer.startPlayingFile("/poweroff.wav");
            break;
        case ssfswshhigh:
            //musicPlayer.startPlayingFile("");
            break;
        case ssfswshlow:
            //musicPlayer.startPlayingFile("");
            break;
        case sssswshhigh:
            //musicPlayer.startPlayingFile("");
            break;
        case sssswshlow:
            //musicPlayer.startPlayingFile("");
            break;
        case ssclash:
            //musicPlayer.startPlayingFile("");
            break;
        case ssstab:
            //musicPlayer.startPlayingFile("");
            break;
        case ssspin:
            //musicPlayer.startPlayingFile("");
            break;
        default:
            if (!musicPlayer.stopped()) {
                musicPlayer.stopPlaying();  
                Serial.println("Stopped playing");
            }
            break;
        currSoundState = sound;
    }
}

/* 
 *  Core 0 setup
 *  Core 0 is the master.  It starts with current State = off, procZeroState = off, procOneState = off.
 *  Pseudocode
 *  Set procZeroState to coreboot
 *  Initialize serial port
 *  Set procZeroState to procboot
 *  Initialize neopixels
 *  Set procZeroState to Ready
 *  Loop until ProcOneState is Ready
 *  Set currentState to ready
 *  Initial setup.  Initialize blade, configure pins, set up interrupt and set the brightness to manage power usage.
 *  
 */
void setup() {
  
    priorState == off;
    procZeroState = coreboot;
    Serial.begin(115200);
    while (!Serial) {
        delay(10); // will pause Zero, Leonardo, etc until serial console opens
    }

    procZeroState = procboot;
    Serial.println("Core 0 Initialization Starting");
    Serial.println("Core 0: Initializing neopixel library");
    Blade.begin();  // Initialize neopixel library
    pinMode(BUTTON_PIN, INPUT_PULLUP); // Button controller - this is the interrupt pin
    pinMode(LED_PIN, OUTPUT);
    
    Blade.setBrightness(LED_BRIGHTNESS);
    Serial.println("Core 0: Initializing interrupt handler");
    // Add interrupt handler
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonPush, RISING);
  
    Serial.println("Core 0: Waiting for core 1");
    
    while(procOneState != isready)
    {
      delay(100);
    }
    procZeroState = isready;
    // Now that procOne is ready, set overall state to ready.
    Serial.println("Core 0:  Setup complete.  Ready to Rock and Roll.");
    currentState = isready;

}

/*
 * Core 1 Initialization
 * Pseudocode
 * Wait for Core 0 to go to procboot state
 * Initialize Gyro
 * Initialize Sound
 * Flag Core 1 as Ready
 */
void setup1() {
    
    while (procZeroState != procboot) {
        delay(100); // will pause until core 0 starts its actual setup
    }
    Serial.println("Core 1 Initializing Starting");
    procOneState = procboot;
    Serial.println("Core 1: Initializing Gyro");
    analogWrite(led, 240);
    // Now try to initialize gyro
    if (!sox.begin_I2C(GYRO_ADDR, &Wire1, 0)) {
        delay(50);
    }
    Serial.println("Core 1: Testing Gyro");
    Serial.print("Accelerometer range set to: ");
    switch (sox.getAccelRange()) {
        case LSM6DS_ACCEL_RANGE_2_G:
            Serial.println("+-2G");
            break;
        case LSM6DS_ACCEL_RANGE_4_G:
            Serial.println("+-4G");
            break;
        case LSM6DS_ACCEL_RANGE_8_G:
            Serial.println("+-8G");
            break;
        case LSM6DS_ACCEL_RANGE_16_G:
            Serial.println("+-16G");
            break;
        default:
            Serial.println("Error");
    }
    sox.setGyroRange(LSM6DS_GYRO_RANGE_250_DPS );
    Serial.print("Gyro range set to: ");
    switch (sox.getGyroRange()) {
        case LSM6DS_GYRO_RANGE_125_DPS:
            Serial.println("125 degrees/s");
            break;
        case LSM6DS_GYRO_RANGE_250_DPS:
            Serial.println("250 degrees/s");
            break;
        case LSM6DS_GYRO_RANGE_500_DPS:
            Serial.println("500 degrees/s");
            break;
        case LSM6DS_GYRO_RANGE_1000_DPS:
            Serial.println("1000 degrees/s");
            break;
        case LSM6DS_GYRO_RANGE_2000_DPS:
            Serial.println("2000 degrees/s");
            break;
        case ISM330DHCX_GYRO_RANGE_4000_DPS:
            break; // unsupported range for the DSOX
        default:
            Serial.println("Error");        
      }
    Serial.println("Core 1: Initializing Sound");
    // Initialize sound
    if (! musicPlayer.begin()) { // initialise the music player
      Serial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
      while (1);
    }
    Serial.println("Initializing Soundsystem");
    musicPlayer.setVolume(SOUND_VOL,SOUND_VOL);
    musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);
    Serial.println("Core 1: Initializing SD Card");
    if (!SD.begin(CARDCS)) {
        Serial.println(F("SD failed, or not present"));
        while (1);  // don't do anything more
    }
    Serial.println("Core 1: SD Card Initialization Complete");
    Serial.println("Core 1: Testing Sound");
    musicPlayer.sineTest(0x44, 500);    // Make a tone to indicate VS1053 is working
    // Make sure blade is in state OFF
    Serial.println(" Core 1: Initialization complete");
    procOneState = isready;
}

/*
 * Core 0 Loop
 * 
 * Core 0 is the master, and manages the neopixel blade (only).
 * It monitors for state change, then does a sync, then intializes the sync state.
 * Currently operates on defined delay period defined by the following DEFINES
 * READY_DELAY     - Number of milliseconds to delay when in Off/Ready state before a sync check.
 * POLL_DELAY      - Number of milliseconds to delay when in any state other than Off/Ready.
 * 
 * Pseudocode
 * If currentState == Off Delay for READY_DELAY
 * if currentState == Ready and prev state == Off, turn on button LED
 * 
 */
void loop() {
    // put your main code here, to run repeatedly:

    while(1) {
        // If the priorState is OFF, the only valid options are off and isReady.  Everything else is invalid and will result in a call to POWEROFF
        currentState = changeBladeState(nextState);
        if (currentState == off) {
            delay(READY_DELAY);
        }
        else {
            delay(POLL_DELAY);
        }
    }

}


/*
 * This loop will manage the sound aspect of the lightsaber.  
 */
void loop1() {

   while(1) {
        // If the priorState is OFF, the only valid options are off and isReady.  Everything else is invalid and will result in a call to POWEROFF
        changeSoundState(nextState);
        if (currentState == off) {
            delay(READY_DELAY);
        }
        else {
            delay(POLL_DELAY);
        }
    }
    
}
/*
*/
