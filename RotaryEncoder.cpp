#include <RotaryEncoder.h>

RotaryEncoder rotary(15,16);

void initRotaryPCINT()
{
    // Init interrupts for the rotary encoder
    PCICR |= (1 << PCIE0);    // This enables Pin Change Interrupt 0.
    PCMSK0 |= (1 << PCINT1) | (1 << PCINT2); // This enables the interrupt for pin 1 and 2 of Port B (D15 & D16).
}

int32_t getRotary()
{
    return rotary.getPosition();
}

// The Interrupt Service Routine for Pin Change Interrupt 0
// This routine will only be called on any signal change on D15 and D16: exactly where we need to check.
//void rotary_pcint_handler() {
ISR(PCINT0_vect)
{
  rotary.tick(); // just call tick() to check the state.
}

