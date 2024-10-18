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

#pragma region PINMAPPING_CCW_DEFINITION
// some basic warnings to ensure Arduino pinmapping is CCW
#ifdef PINMAPPING_CW
#error "Code written for CCW pin mapping!"
#else
#ifndef PINMAPPING_CCW
#error "Code written for CCW pin mapping, but PINMAPPING_CCW is undefined!"
#endif
#endif
#pragma endregion PINMAPPING_CCW_DEFINITION

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