#ifndef _Display_h_
#define _Display_h_

#include <LiquidCrystal_I2C.h>
#include <Wire.h>

class Display: public LiquidCrystal_I2C
{
    void printDigits(int digits);
    
public:
    Display();
    void begin();

    void printTime();
    void printDate();
};


#endif
