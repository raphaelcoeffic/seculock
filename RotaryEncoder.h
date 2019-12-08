#ifndef _RotaryEncoder_h_
#define _RotaryEncoder_h_

#include <Arduino.h>

void initRotaryKnob();
int32_t getRotaryCounter();

// Get difference from last call
int8_t getRotaryDiff();

// Get changes since last call
enum {
    ROT_None=0,
    ROT_Pressed,
    ROT_Released
};

uint8_t getRotaryButton();

// Get current state
bool getRotaryButtonPressed();

#endif
