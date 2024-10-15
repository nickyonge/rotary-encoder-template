#include <Arduino.h>

#define PIN_LED 5
#define PIN_SWITCH 6

void setup()
{
    // put your setup code here, to run once:

    pinMode(PIN_SWITCH, INPUT_PULLUP);

    pinMode(PIN_LED, OUTPUT);
}

void loop()
{

    bool sw = digitalRead(PIN_SWITCH);

    // put your main code here, to run repeatedly:
    digitalWrite(PIN_LED, sw ? HIGH : LOW);
    delay(50);
}