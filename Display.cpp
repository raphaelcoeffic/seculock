#include "Display.h"
#include <Time.h>
#include <Timezone.h>

// 0x27 is the I2C bus address for an unmodified backpack
#define LCD_I2C_ADDR 0x27//0x38

#define LCD_I2C_EN   2
#define LCD_I2C_RW   1
#define LCD_I2C_RS   0
#define LCD_I2C_D4   4
#define LCD_I2C_D5   5
#define LCD_I2C_D6   6
#define LCD_I2C_D7   7
#define LCD_I2C_BL   3

//Central European Time (Frankfurt, Paris)
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     //Central European Summer Time
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};       //Central European Standard Time
Timezone CE(CEST, CET);

void Display::printDigits(int digits)
{
  // utility function for digital clock display:
  // prints preceding colon and leading 0
  if(digits < 10)
    print('0');
  print(digits);
}

Display::Display()
    : LiquidCrystal_I2C(LCD_I2C_ADDR,
                        LCD_I2C_EN,
                        LCD_I2C_RW,
                        LCD_I2C_RS,
                        LCD_I2C_D4,
                        LCD_I2C_D5,
                        LCD_I2C_D6,
                        LCD_I2C_D7,
                        LCD_I2C_BL,
                        POSITIVE)
{
}

void Display::begin()
{
    LiquidCrystal_I2C::begin(20, 4);
}

void Display::printTime()
{
    time_t local = CE.toLocal(now());

    printDigits(hour(local));
    print(':');
    printDigits(minute(local));
    print(':');
    printDigits(second(local));
}

void Display::printDate()
{
    time_t local = CE.toLocal(now());

    printDigits(day(local));
    print('/');
    printDigits(month(local));
    print('/');
    printDigits(year(local));
}
