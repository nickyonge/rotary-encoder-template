// basic test script for using a rotary encoder with an ATtiny84

// TODO: attach interrupt pin to enc switch and confirm sleep

#include <Arduino.h>
#include <RotaryEncoder.h>
#include <EnableInterrupt.h>

#include <avr/sleep.h>

// #define MODE_LED_BLINK   // blink on/off LED mode
#define MODE_LED_BOOLEAN // Boolean counting LED mode
// #define MODE_LED_TIMED   // timed LED mode (useful for debugging)

#define PIN_ENC_SWITCH 8  // INT0, PCINT10 (add 10F cap to debounce)
#define PIN_ENC_INPUT_1 7 // A7, PCINT7
#define PIN_ENC_INPUT_2 6 //

// FOUR3 - preferred, inc/dec pos by 1, reverse direction applies inc/dec as expected
// TWO03 - inc/dec pos by 2, reverse direction applies inc/dec as expected
// RotaryEncoder encoder(PIN_ENC_INPUT_1, PIN_ENC_INPUT_2);// default FOUR0
// RotaryEncoder encoder(PIN_ENC_INPUT_1, PIN_ENC_INPUT_2, RotaryEncoder::LatchMode::TWO03);
// RotaryEncoder encoder(PIN_ENC_INPUT_1, PIN_ENC_INPUT_2, RotaryEncoder::LatchMode::FOUR0);
RotaryEncoder encoder(PIN_ENC_INPUT_1, PIN_ENC_INPUT_2, RotaryEncoder::LatchMode::FOUR3);
// FOUR0 - default, inc/dec pos by 1, reverse direction results in 0

static int encPos = 0;         // encoder position
static bool encSwitch = false; // is enc switch currently pressed? read at beginning of loop()

// #define SWITCH_SETS_LEDS          // if defined, holding switch sets LEDs to the given state
#define SWITCH_SET_LED_STATE HIGH // if SWITCH_SETS_LEDS defined, set them to this state

void onInterrupt();                               // called on switch interrupt
void digitalWritePin(uint8_t pin, uint8_t state); // digitalWrite that accommodates pin state
volatile bool interrupted = false;

void sleep();
// #define SLEEP_LOOP_TIMEOUT 5000
int sleepTimeout = 0;

// ----------------------------- LED BLINK MODE SETUP
#ifdef MODE_LED_BLINK
#define PIN_LED_BLINK 5

#define LED_BLINK_MIN 100
#define LED_BLINK_MAX 2000

#define ENCODER_BLINK_WRAP // comment out to lock lerp at 0-100, uncomment to wrap

#define LED_BLINK_LERP_INCREMENT 1

int blinkTimer = 0;    // time value, constantly incrementing in ms
int blinkLerp = 0;     // lerp value, 0-100, to map between LED_BLINK_MIN and LED_BLINK_MAX
bool blinkLED = false; // is the LED currently on via blinking
#endif                 // MODE_LED_BLINK

// ----------------------------- LED BOOLEAN MODE SETUP
#ifdef MODE_LED_BOOLEAN

#define PIN_LED_BOOL_A 5
#define PIN_LED_BOOL_B 4
#define PIN_LED_BOOL_C 3
#define PIN_LED_BOOL_D 2

// #define ENCODER_BOOLEAN_LOCK_INCREMENT // lock bool increment to +/- 1 per pulse
#define LED_BOOL_ZERODELTA_JUMP // if delta == 0 and inc locked, offset bool by 8

// value, from 0-15, to set boolean LEDs to
int booleanValue = 0;

// update boolean LEDs to current booleanValue
void setBooleanLEDs();
// set boolean LED values based on an input int (value must be 0-15), autoupdates booleanValue
void setBooleanLEDs(int value);
// set boolean LED values directly
void setBooleanLEDs(bool a, bool b, bool c, bool d);

// inverts boolean LED values (useful for testing)
void invertBooleanLEDs();

#endif // MODE_LED_BOOLEAN

#ifdef MODE_LED_TIMED

#define PIN_LED_TIMED 5
int ledTimedValue = 0;
void timedLED();
#else
void timedLED(); // just to prevent errors for usage throughout code
#endif // MODE_LED_TIMED


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas" // needed so that #pragma region doesn't make an IDE warning =_=
#pragma region PINMAPPING_CCW_DEFINITION
// Some basic warnings to ensure ATtinyX4's Arduino pinmapping is defined correctly intended
// 
// Uncomment one of the two lines below to select the desired pinmapping `
#define USE_CW_PINMAPPING
// #define USE_CCW_PINMAPPING
// 
#ifdef USE_CW_PINMAPPING
#ifdef USE_CCW_PINMAPPING
#error "Both CW and CCW pinmappings are specified - only one may be selected!"
#else
#ifdef PINMAPPING_CCW
#error "Project uses CW pin mapping, but CCW pinmapping is defined! Ensure ini board_build.variant is tinyX4_reverse (optional if board is attiny84_cw_pinmap), or omitted (if board is attiny84)"
#else
#ifndef PINMAPPING_CW
#error "Project uses CW pin mapping, but PINMAPPING_CW is undefined! Ensure ini board_build.variant is omitted (if board is attiny84), or that it's tinyX4 (optional, or if board is attiny84_cw_pinmap)"
#endif
#endif
#endif
#else
#ifndef USE_CCW_PINMAPPING
#error "Neither CW or CCW pinmappings are specified - one must be selected!"
#else
#ifdef PINMAPPING_CW
#error "Project uses CCW pin mapping, but CW pinmapping is defined! Ensure ini board_build.variant is omitted (if board is attiny84), or that it's tinyX4 (optional, or if board is attiny84_cw_pinmap)"
#else
#ifndef PINMAPPING_CCW
#error "Project uses CCW pin mapping, but PINMAPPING_CCW is undefined! Ensure ini board_build.variant is tinyX4_reverse (optional if board is attiny84_cw_pinmap), or omitted (if board is attiny84)"
#endif
#endif
#endif
#endif
#pragma endregion PINMAPPING_CCW_DEFINITION
#pragma GCC diagnostic pop // re-enable unknown pragma errors 

void setup()
{

    // put your setup code here, to run once:

    pinMode(PIN_ENC_SWITCH, INPUT_PULLUP);

#ifdef MODE_LED_BLINK
    pinMode(PIN_LED_BLINK, OUTPUT);
#endif

#ifdef MODE_LED_BOOLEAN
    pinMode(PIN_LED_BOOL_A, OUTPUT);
    pinMode(PIN_LED_BOOL_B, OUTPUT);
    pinMode(PIN_LED_BOOL_C, OUTPUT);
    pinMode(PIN_LED_BOOL_D, OUTPUT);
    setBooleanLEDs(0);
#endif

#ifdef MODE_LED_TIMED
    pinMode(PIN_LED_TIMED, OUTPUT);
#endif

    // init interrupt
    enableInterrupt(PIN_ENC_SWITCH, onInterrupt, FALLING);

    // test onInterruput reset
    // onInterrupt();
    // interrupted = false;

    // cli(); // disable interrupts
    // sei(); // enable interrupts

    // init sleep mode
    // set_sleep_mode(SLEEP_MODE_IDLE);
    // set_sleep_mode(SLEEP_MODE_STANDBY);
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
}

void loop()
{

    // get switch input
    bool lastSw = encSwitch;                  // switch state on last loop
    encSwitch = !digitalRead(PIN_ENC_SWITCH); // if encSwitch true, force LED on

    // get encoder input
    encoder.tick();
    int newPos = encoder.getPosition();

    // determine if interrupted
    if (interrupted)
    {
        interrupted = false;

#ifdef MODE_LED_BOOLEAN
        // invert boolean value
        invertBooleanLEDs();
#endif
    }

    // update encoder on change
    if (encPos != newPos)
    {
        int delta = newPos - encPos;

#ifdef MODE_LED_BLINK
        // blink mode
        blinkLerp += delta * LED_BLINK_LERP_INCREMENT;
#ifdef ENCODER_BLINK_WRAP
        // wrap around at 100
        while (blinkLerp < 0)
        {
            blinkLerp += 100;
        }
        while (blinkLerp > 100)
        {
            blinkLerp -= 100;
        }
#else
        // cap to 100
        if (blinkLerp < 0)
        {
            blinkLerp = 0;
        }
        else if (blinkLerp > 100)
        {
            blinkLerp = 100;
        }
#endif
#endif

#ifdef MODE_LED_BOOLEAN
        // boolean mode
#ifdef ENCODER_BOOLEAN_LOCK_INCREMENT
        if (delta > 0)
        {
            booleanValue++;
        }
        else if (delta < 0)
        {
            booleanValue--;
        }
#ifdef LED_BOOL_ZERODELTA_JUMP
        else if (delta == 0)
        {
            booleanValue += 8;
        }
#endif
#else
        booleanValue += delta;
#endif
        while (booleanValue < 0)
        {
            booleanValue += 16;
        }
        while (booleanValue >= 16)
        {
            booleanValue -= 16;
        }
        setBooleanLEDs();
#endif

#ifdef MODE_LED_TIMED
        // pulse in timed mode
#endif

        // finish pulse method
        encPos = newPos;
        encoder.setPosition(encPos);
    }

#ifdef MODE_LED_BLINK
    // update blink timer
    blinkTimer++;
    if (blinkTimer > map(blinkLerp, 0, 100, LED_BLINK_MIN, LED_BLINK_MAX))
    {
        blinkLED = !blinkLED;
        blinkTimer = 0;
    }

    // put your main code here, to run repeatedly:
    digitalWritePin(PIN_LED_BLINK, blinkLED ? HIGH : LOW);
#endif

#ifdef MODE_LED_BOOLEAN
    // boolean mode
#endif

#ifdef MODE_LED_TIMED

    // timedLED trigger on press switch
    // if (encSwitch && encSwitch != lastSw)
    // {
    //     timedLED();
    // }

    if (ledTimedValue > 0)
    {
        ledTimedValue--;
        digitalWritePin(PIN_LED_TIMED, ledTimedValue > 0 ? HIGH : LOW);
#ifdef MODE_LED_BOOLEAN
        // if bool mode and timed value done, reset bool LEDs
        if (ledTimedValue == 0)
        {
            setBooleanLEDs();
        }
#endif
    }
#endif

    if (encSwitch != lastSw)
    {
// switch state changed, update as needed
#ifdef MODE_LED_BOOLEAN
        setBooleanLEDs();
#endif
    }

// update sleep timer
#ifdef SLEEP_LOOP_TIMEOUT
    if (SLEEP_LOOP_TIMEOUT > 0)
    {
        sleepTimeout++;
        if (sleepTimeout > SLEEP_LOOP_TIMEOUT)
        {
            sleepTimeout = 0;
            sleep();
            // invertBooleanLEDs();
        }
    }
#endif

    // end loop
    delay(1);
}

void onInterrupt()
{
    interrupted = true;
}

#ifdef MODE_LED_BOOLEAN

void setBooleanLEDs(int booleanValueTo)
{
    booleanValue = booleanValueTo;
    setBooleanLEDs();
}
void setBooleanLEDs()
{
    while (booleanValue < 0)
    {
        booleanValue += 16;
    }
    while (booleanValue >= 16)
    {
        booleanValue -= 16;
    }
    switch (booleanValue)
    {
    case 0:
        setBooleanLEDs(false, false, false, false);
        break;
    case 1:
        setBooleanLEDs(true, false, false, false);
        break;
    case 2:
        setBooleanLEDs(false, true, false, false);
        break;
    case 3:
        setBooleanLEDs(true, true, false, false);
        break;
    case 4:
        setBooleanLEDs(false, false, true, false);
        break;
    case 5:
        setBooleanLEDs(true, false, true, false);
        break;
    case 6:
        setBooleanLEDs(false, true, true, false);
        break;
    case 7:
        setBooleanLEDs(true, true, true, false);
        break;
    case 8:
        setBooleanLEDs(false, false, false, true);
        break;
    case 9:
        setBooleanLEDs(true, false, false, true);
        break;
    case 10:
        setBooleanLEDs(false, true, false, true);
        break;
    case 11:
        setBooleanLEDs(true, true, false, true);
        break;
    case 12:
        setBooleanLEDs(false, false, true, true);
        break;
    case 13:
        setBooleanLEDs(true, false, true, true);
        break;
    case 14:
        setBooleanLEDs(false, true, true, true);
        break;
    case 15:
        setBooleanLEDs(true, true, true, true);
        break;
    }
}
void setBooleanLEDs(bool a, bool b, bool c, bool d)
{
    digitalWritePin(PIN_LED_BOOL_A, a ? HIGH : LOW);
    digitalWritePin(PIN_LED_BOOL_B, b ? HIGH : LOW);
    digitalWritePin(PIN_LED_BOOL_C, c ? HIGH : LOW);
    digitalWritePin(PIN_LED_BOOL_D, d ? HIGH : LOW);
}

void invertBooleanLEDs()
{
    booleanValue = 15 - booleanValue;
    setBooleanLEDs();
}

#endif

#ifdef MODE_LED_TIMED

void timedLED()
{
    if (ledTimedValue < 500)
    {
        ledTimedValue = 500;
    }
}
#else
void timedLED() {}
#endif

void sleep()
{
    sleep_enable();      // enable sleep bit
    sleep_bod_disable(); // disable brownout detection
    sei();               // ensure interrupts are active
    sleep_cpu();         // begin sleep mode
    // device will automatically re-enable on receiving an interrupt
    sleep_disable(); // disable sleep bit
    sei();           // re-enable interrupts again
}

void digitalWritePin(uint8_t pin, uint8_t state)
{
#ifdef SWITCH_SETS_LEDS
    if (encSwitch)
    {
        state = SWITCH_SET_LED_STATE;
    }
#endif
    digitalWrite(pin, state);
}

/* PINOUT DIAGRAMS FOR ATTINYX4 / ATTINYX5

                               ATtinyX4
                              .--------.
          VCC [2.7-5.5V] - 1 =| O      |= 14 - GND
       10(0),X1,PCI8,PB0 - 2 =|        |= 13 - 0(10),A0,PCI0,PA0
        9(1),X2,PCI9,PB1 - 3 =|        |= 12 - 1(9),A1,TXD,PCI1,PA1
  RESET,11(11),PCI11,PB3 - 4 =|        |= 11 - 2(8),A2,RXD,PCI2,PA2
     8(2),INT0,PCI10,PB2 - 5 <|        |= 10 - 3(7),A3,PCI3,PA3
        7(3),A7,PCI7,PA7 - 6 <|        |= 9  - 4(6),A4,SCK/SCL,PCI4,PA4
 6(4),A6,SDA/DI,PCI6,PA6 - 7 <|        |> 8  - 5(5),A5,DO,PCI5,PA5
                              '--------'

                               ATtinyX5
                              .--------.
     RESET,5,A0,PCI5,PB5 - 1 =| O      |= 8 - VCC [2.7-5.5V]
        3,A3,X1,PCI3,PB3 - 2 =|        |= 7 - 2,INT0,A1,SCK/SCL,PCI2,PB2
        4,A2,X2,PCI4,PB4 - 3 <|        |> 6 - 1,DO,RXD,PCI1,PB1
                     GND - 4 =|        |> 5 - 0,DI,TXD,SDA,PCI0,PB0
                              '--------'

    = - Pin
    > - PWM Pin
 4(6) - Arduino Pins, with CW(CCW) pinout (CCW on ATtinyX4 only)
 PCI - Pin Change Interrupt (PCINT) pins
 INT - External Interrupt pins
RESET - Reset Pin (fuses must be set to use)
DI/DO - Data In/Data Out (aka MOSI/MISO)
PA/PB - Port pins, Port A and Port B (PA on ATtinyX4 only)

Sourced from ATTinyCore by SpenceKonde
GitHub: https://github.com/SpenceKonde/ATTinyCore
ATtinyX4 Pinout: https://raw.githubusercontent.com/SpenceKonde/ATTinyCore/refs/heads/v2.0.0-devThis-is-the-head-submit-PRs-against-this/avr/extras/Pinout_x4.jpg
ATtinyX5 Pinout: https://raw.githubusercontent.com/SpenceKonde/ATTinyCore/refs/heads/v2.0.0-devThis-is-the-head-submit-PRs-against-this/avr/extras/Pinout_x5.jpg

Datasheets:
ATtinyX4: https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7701_Automotive-Microcontrollers-ATtiny24-44-84_Datasheet.pdf
ATtniyX5: https://ww1.microchip.com/downloads/en/devicedoc/atmel-2586-avr-8-bit-microcontroller-attiny25-attiny45-attiny85_datasheet.pdf

AVR Fuse Calculator (for using RESET pin as I/O):
https://www.engbedded.com/fusecalc/

*/