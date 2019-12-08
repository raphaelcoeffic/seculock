#ifndef _Display_h_
#define _Display_h_

#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <TimeLib.h>

class Display: protected LiquidCrystal_I2C
{
    char lines[4][21];
    
    unsigned long refreshTS;
    bool          dirty;

    uint8_t menuState;
    int8_t  menuCursor;

    int16_t lineCursor;
    int16_t lineOffset;
    
    void clear();
    void printScreen();
    
    //void printDigits(int digits);
    void printDateTime(char* buffer, size_t len, time_t utc);
    void printShortDateTime(char* buffer, size_t len, time_t utc);

    void mainScreen(uint8_t buttonEvent, int8_t rotaryDiff);

    void editUser(uint8_t buttonEvent, int8_t rotaryDiff);
    void usersChoices(uint8_t buttonEvent, int8_t rotaryDiff);
    void showUsers(uint8_t buttonEvent, int8_t rotaryDiff);

    void showSingleLog(uint8_t buttonEvent, int8_t rotaryDiff);
    void logsChoices(uint8_t buttonEvent, int8_t rotaryDiff);
    void showLogs(uint8_t buttonEvent, int8_t rotaryDiff);
    
public:
    Display();
    void begin();

    void setDirty() { dirty = true; }
    void refresh();
};


#endif
