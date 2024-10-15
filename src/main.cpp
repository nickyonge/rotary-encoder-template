#include <Arduino.h>
#include <RotaryEncoder.h>

#define PIN_LED 5
#define PIN_SWITCH 4

#define PIN_IN1 2 // INT0, PCINT10
#define PIN_IN2 3 // A7, PCINT7

#define BLINK_MIN 100
#define BLINK_MAX 2000

#define BLINK_INCREMENT 1

int blinkTimer = 0;    // time value, constantly incrementing in ms
int blinkLerp = 0;     // lerp value, 0-100, to map between BLINK_MIN and BLINK_MAX
bool blinkLED = false; // is the LED currently on via blinking

RotaryEncoder encoder(PIN_IN1, PIN_IN2);
// RotaryEncoder encoder(PIN_IN1, PIN_IN2, RotaryEncoder::LatchMode::TWO03);
// RotaryEncoder encoder(PIN_IN1, PIN_IN2, RotaryEncoder::LatchMode::FOUR0);
// RotaryEncoder encoder(PIN_IN1, PIN_IN2, RotaryEncoder::LatchMode::FOUR3);

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

    pinMode(PIN_SWITCH, INPUT_PULLUP);

    pinMode(PIN_LED, OUTPUT);
}

void loop()
{

    // get switch input
    static bool sw = !digitalRead(PIN_SWITCH); // if sw true, force LED on

    // get encoder input
    static int pos = 0;
    encoder.tick();
    int newPos = encoder.getPosition();
    if (pos != newPos) {
        int delta = newPos - pos;
        blinkLerp += delta * BLINK_INCREMENT;
        if (blinkLerp < 0) {
            blinkLerp = 0;
        } else if (blinkLerp > 100) {
            blinkLerp = 100;
        }
        pos = newPos;
    }

    // update blink timer 
    blinkTimer++;
    if (blinkTimer > map(blinkLerp, 0, 100, BLINK_MIN, BLINK_MAX))
    {
        blinkLED = !blinkLED;
        blinkTimer = 0;
    }

    // put your main code here, to run repeatedly:
    digitalWrite(PIN_LED, sw || blinkLED ? HIGH : LOW);

    delay(1);
}