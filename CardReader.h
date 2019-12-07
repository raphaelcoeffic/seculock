#ifndef _CardReader_h_
#define _CardReader_h_

#include "AltSoftSerial.h"

class CardReader
{
    AltSoftSerial com;
    unsigned long refresh_ts;
    unsigned long refresh_delay;
    bool          locked;

    bool sendCmd(uint8_t cmd, uint8_t* buffer = NULL, uint16_t len = 0);

public:
    enum LedColor {
        LedSolidGreen,
        LedBlinkGreen,
        LedSolidRed,
        LedBlinkRed
    };

    CardReader(uint8_t rxPin, uint8_t txPin)
        : com(rxPin, txPin, true),
          refresh_ts(0),
          refresh_delay(10000),
          locked(true)
    {}

    void begin();
    void init();

    // buffer must have at least 9 bytes
    bool poll(uint8_t* buffer);

    bool getLocked() { return locked; }
    void setLocked(bool l) { locked = l; }

    void setLed(LedColor c, unsigned int duration);
};

#endif
