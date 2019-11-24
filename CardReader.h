#ifndef _CardReader_h_
#define _CardReader_h_

#include <SoftwareSerial.h>

class CardReader
{
    SoftwareSerial com;
    
public:
    CardReader(uint8_t rxPin, uint8_t txPin)
        : com(rxPin, txPin, true)
    {}

    void begin();
    void poll();

    bool sendCmd(uint8_t cmd, uint8_t* buffer = NULL, uint16_t len = 0);
};

#endif
