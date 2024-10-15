#include <Arduino.h>
#include <RotaryEncoder.h>

#define MODE_LED_BLINK
#define MODE_LED_BOOLEAN

#define PIN_LED_BLINK 5
#define PIN_ENC_SWITCH 4

#define PIN_ENC_INPUT_1 2 // INT0, PCINT10
#define PIN_ENC_INPUT_2 3 // A7, PCINT7

#define LED_BLINK_MIN 100
#define LED_BLINK_MAX 2000

#define ENCODER_BLINK_WRAP // comment out to lock lerp at 0-100, uncomment to wrap

#define LED_BLINK_LERP_INCREMENT 1

int blinkTimer = 0;    // time value, constantly incrementing in ms
int blinkLerp = 0;     // lerp value, 0-100, to map between LED_BLINK_MIN and LED_BLINK_MAX
bool blinkLED = false; // is the LED currently on via blinking

RotaryEncoder encoder(PIN_ENC_INPUT_1, PIN_ENC_INPUT_2);
// RotaryEncoder encoder(PIN_ENC_INPUT_1, PIN_ENC_INPUT_2, RotaryEncoder::LatchMode::TWO03);
// RotaryEncoder encoder(PIN_ENC_INPUT_1, PIN_ENC_INPUT_2, RotaryEncoder::LatchMode::FOUR0);
// RotaryEncoder encoder(PIN_ENC_INPUT_1, PIN_ENC_INPUT_2, RotaryEncoder::LatchMode::FOUR3);

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

    pinMode(PIN_LED_BLINK, OUTPUT);
}

void loop()
{

    // get switch input
    static bool sw = !digitalRead(PIN_ENC_SWITCH); // if sw true, force LED on

    // get encoder input
    static int pos = 0;
    encoder.tick();
    int newPos = encoder.getPosition();
    if (pos != newPos)
    {
        int delta = newPos - pos;

#ifdef MODE_LED_BLINK
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

        pos = newPos;
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
    digitalWrite(PIN_LED_BLINK, sw || blinkLED ? HIGH : LOW);
#endif

    // end loop
    delay(1);
}