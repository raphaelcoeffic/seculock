#include "RotaryEncoder.h"
#include <RotaryEncoder.h>

#define BUTTON_PIN A0

RotaryEncoder rotary(15,16);

static int32_t lastRotaryCounter = 0;

static uint8_t buttonState = HIGH;
static uint8_t lastButtonState = HIGH;
static unsigned long lastButtonTS = 0;

// Init interrupts for the rotary encoder
void initRotaryKnob()
{
    // This enables Pin Change Interrupt 0.
    PCICR |= (1 << PCIE0);

     // This enables the interrupt for pin 1 and 2 of Port B (D15 & D16).
    PCMSK0 |= (1 << PCINT1) | (1 << PCINT2);

    // Button pin mode
    pinMode(BUTTON_PIN, INPUT_PULLUP);
}

// The Interrupt Service Routine for Pin Change Interrupt 0
// This routine will only be called on any signal change on D15 and D16:
//  -> exactly where we need to check.
//
ISR(PCINT0_vect)
{
    rotary.tick(); // just call tick() to check the state.
}

// Get raw counter value
int32_t getRotaryCounter()
{
    return rotary.getPosition();
}

// Get difference from last call
int8_t getRotaryDiff()
{
    int32_t cnt  = getRotaryCounter();
    int32_t diff = cnt - lastRotaryCounter;

    lastRotaryCounter = cnt;
    return (int8_t)diff;
}

uint8_t getRotaryButton()
{
    uint8_t reading = digitalRead(BUTTON_PIN);
    if (reading != lastButtonState) {
        lastButtonTS = millis();
    }
    lastButtonState = reading;

    if ((millis() - lastButtonTS) > 50) {

        if (reading != buttonState) {
            buttonState = reading;
 
            if (buttonState == HIGH) {
                Serial.println("# Button released");
                return ROT_Released;
            }

            Serial.println("# Button pressed");
            return ROT_Pressed;
        }
    }

    return ROT_None;
}

bool getRotaryButtonPressed()
{
    return buttonState != HIGH;
}
