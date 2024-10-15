#include <Arduino.h>

#define PIN_LED 5
#define PIN_SWITCH 4

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

    bool sw = !digitalRead(PIN_SWITCH);

    // put your main code here, to run repeatedly:
    digitalWrite(PIN_LED, sw ? HIGH : LOW);
    delay(50);
}