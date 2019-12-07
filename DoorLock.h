#ifndef _DoorLock_h_
#define _DoorLock_h_

#include <Arduino.h>

#define LOCK_PIN 5

class DoorLock
{
public:
    static void begin()
    {
        pinMode(LOCK_PIN, OUTPUT);
        digitalWrite(LOCK_PIN, LOW);
    }

    static void open()
    {
        digitalWrite(LOCK_PIN, HIGH);
    }

    static void close()
    {
        digitalWrite(LOCK_PIN, LOW);
    }
};

#endif
