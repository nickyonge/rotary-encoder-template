#include <Arduino.h>

#define PIN_LED 5
#define PIN_SWITCH 4

#define BLINK_MIN 100
#define BLINK_MAX 2000

#define BLINK_INCREMENT 5

int blinkTimer = 0;    // time value, constantly incrementing in ms
int blinkLerp = 0;     // lerp value, 0-100, to map between BLINK_MIN and BLINK_MAX
bool blinkLED = false; // is the LED currently on via blinking

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

    bool sw = !digitalRead(PIN_SWITCH); // if sw true, force LED on

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