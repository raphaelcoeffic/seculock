#include "Display.h"
#include "Seculock.h"
#include "Register.h"
#include "RotaryEncoder.h"
#include "Log.h"

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

enum {
    MenuMain=0,
    MenuMainChoices,
    MenuUsers,
    MenuUsersChoices,
    MenuUserEdit,
    MenuLogs,
    MenuLogsChoices,
    MenuLog
};

// Strings
const PROGMEM char LOCKED[]      = "     > LOCKED <     ";
const PROGMEM char EMPTY_LOG[]   =   "  -- emtpy --     ";
const PROGMEM char SYSTEM_BOOT[] = "  --> System boot   ";
const PROGMEM char UNLOCKED[]    = "    > UNLOCKED <    ";
const PROGMEM char CHOICES[]     = "Choices:            ";

// Main menu
const PROGMEM char EDIT_USERS[] = "  Show Users        ";
const PROGMEM char SHOW_LOGS[]  = "  Show Logs         ";
const PROGMEM char EXIT[]       = "  Exit              ";

// Users menu
const PROGMEM char EDIT_USER[]  = "  Edit User         ";
const PROGMEM char MAIN_MENU[]  = "  Main Menu         ";
// EXIT

// Logs menu
const PROGMEM char SHOW_LOG[]   = "  Show Log Entry    ";
// MAIN_MENU
// EXIT


byte startChar[] = {
  B00000,
  B00000,
  B00100,
  B00100,
  B11111,
  B00100,
  B00100,
  B00000
};

byte unlockedChar[] = {
  B01110,
  B00001,
  B00001,
  B00001,
  B11111,
  B11111,
  B11111,
  B11111
};

byte lockedChar[] = {
  B00000,
  B01110,
  B10001,
  B10001,
  B11111,
  B11111,
  B11111,
  B11111
};

byte adminChar[] = {
  B10001,
  B01110,
  B00100,
  B11111,
  B11111,
  B00100,
  B01110,
  B10001
};

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
                        POSITIVE),
      refreshTS(0),
      dirty(true),
      menuState(0),
      menuCursor(0),
      lineCursor(0),
      lineOffset(0)
{
}

void Display::begin()
{
    LiquidCrystal_I2C::begin(20, 4);

    createChar(1, startChar);
    createChar(2, unlockedChar);
    createChar(3, lockedChar);
    createChar(4, adminChar);
}

void Display::clear()
{
    memset(lines, ' ', sizeof(lines));
    for (uint8_t i=0; i<4; i++) {
        lines[i][20] = '\0';
    }
}

void Display::printScreen()
{
    for (uint8_t i=0; i<4; i++) {
        setCursor(0,i);
        print(lines[i]);
    }
}

void Display::printDateTime(char* buffer, size_t len, time_t utc)
{
    time_t local = CE.toLocal(utc);

    snprintf(buffer, len, "%02d:%02d:%02d  %02d/%02d/%04d",
             hour(local), minute(local), second(local),
             day(local), month(local), year(local));
}

void Display::printShortDateTime(char* buffer, size_t len, time_t utc)
{
    time_t local = CE.toLocal(utc);

    snprintf(buffer, len, "%02d:%02d %02d/%02d  ",
             hour(local), minute(local),
             day(local), month(local), year(local)-2000);
}

void Display::mainScreen(uint8_t buttonEvent, int8_t rotaryDiff)
{
    if (menuState == MenuMainChoices) {
        memcpy_P(lines[0], CHOICES, 20);
        memcpy_P(lines[1], EDIT_USERS, 20);
        memcpy_P(lines[2], SHOW_LOGS, 20);
        memcpy_P(lines[3], EXIT, 20);

        lines[menuCursor+1][0] = '>';
        menuCursor += rotaryDiff;
        menuCursor = constrain(menuCursor, 0, 2); // 3 choices

        if (buttonEvent == ROT_Released) {
            switch(menuCursor) {
            case 0:
                menuState = MenuUsers;
                lineOffset = 0;
                lineCursor = 0;
                break;
            case 1:
                menuState = MenuLogs;
                lineOffset = 0;
                lineCursor = 0;
                break;
            default:
                menuState = MenuMain;
                break;
            }
            menuCursor = 0;
        }

        return;
    }
    
    printDateTime(lines[0], 21, now());

    if (cardSlot != INVALID_CARD) {
        User* user = registerReadUser(cardSlot);
        memcpy(lines[2], (const char*)user->name, constrain(strlen(user->name),0,20));
        memcpy(lines[3], (const char*)user->telNr, strlen(user->telNr));
    }
    else {
        memcpy_P(lines[2], LOCKED, 20);
    }

    if (buttonEvent == ROT_Released) {
        menuState = MenuMainChoices;
        menuCursor = 0;
    }
}

void Display::editUser(uint8_t buttonEvent, int8_t rotaryDiff)
{
    User* user = registerReadUser(lineOffset + lineCursor);
    memcpy(lines[0], user->name, constrain(strlen(user->name), 0, 20));
    memcpy(lines[1], user->telNr, strlen(user->telNr));
    //lines[2][0] = ' ';
    lines[2][1] = (user->flags & USER_ADMIN) ? '\x04' : ' ';
    lines[2][2] = (user->flags & USER_ADMIN) ? '\x04' : ' ';
    //lines[2][3] = ' ';
    lines[2][4] = (user->flags & USER_DENIED) ? '\x03' : ' ';
    lines[2][5] = (user->flags & USER_DENIED) ? '\x03' : ' ';

    if (buttonEvent == ROT_Released) {
        menuState = MenuUsers;
    }
}

void Display::usersChoices(uint8_t buttonEvent, int8_t rotaryDiff)
{
    memcpy_P(lines[0], CHOICES, 20);
    memcpy_P(lines[1], EDIT_USER, 20);
    memcpy_P(lines[2], MAIN_MENU, 20);
    memcpy_P(lines[3], EXIT, 20);

    lines[menuCursor+1][0] = '>';
    menuCursor += rotaryDiff;
    menuCursor = constrain(menuCursor, 0, 2); // 3 choices

    if (buttonEvent == ROT_Released) {
        switch(menuCursor) {
        case 0:
            menuState  = MenuUserEdit;
            break;
        case 1:
            menuState = MenuMain;
            break;
        case 2:
            menuState = MenuUsers;
            break;
        }

        menuCursor = 0;
    }

    return;
}

void Display::showUsers(uint8_t buttonEvent, int8_t rotaryDiff)
{
    if (menuState == MenuUsersChoices) {
        usersChoices(buttonEvent, rotaryDiff);
        return;
    }
    else if (menuState == MenuUserEdit) {

        editUser(buttonEvent, rotaryDiff);
        return;
    }

    lineCursor += rotaryDiff;
    if (lineCursor > 3) {
        lineOffset = constrain(lineOffset + lineCursor - 3, 0, 127-4);
        lineCursor = 3;
    }
    else if (lineCursor < 0) {
        lineOffset = constrain(lineOffset + lineCursor, 0, 127-4);
        lineCursor = 0;
    }

    for (uint8_t i=0; i < 4; i++) {
        User* user = registerReadUser(i + lineOffset);
        snprintf(lines[i], 21, " %2d:%-16s", i + lineOffset, user->name);
    }
    lines[lineCursor][0] = '>';
    
    if (buttonEvent == ROT_Released) {
        menuState = MenuUsersChoices;
    }
}

void Display::logsChoices(uint8_t buttonEvent, int8_t rotaryDiff)
{
    memcpy_P(lines[0], CHOICES, 20);
    memcpy_P(lines[1], SHOW_LOG, 20);
    memcpy_P(lines[2], MAIN_MENU, 20);
    memcpy_P(lines[3], EXIT, 20);

    lines[menuCursor+1][0] = '>';
    menuCursor += rotaryDiff;
    menuCursor = constrain(menuCursor, 0, 2); // 3 choices

    if (buttonEvent == ROT_Released) {
        switch(menuCursor) {
        case 0:
            menuState = MenuLog;
            break;
        case 1:
            menuState = MenuMain;
            break;
        case 2:
            menuState = MenuLogs;
            break;
        }

        menuCursor = 0;
    }

    return;
}

void Display::showSingleLog(uint8_t buttonEvent, int8_t rotaryDiff)
{
    LogEntry entry;
    memset(&entry, 0, sizeof(LogEntry));

    if (logReadEntry(lineOffset + lineCursor, &entry)) {
        printDateTime(lines[0], 21, entry.ts);
        switch(entry.event) {
        case LOG_Boot:
            memcpy_P(lines[1], SYSTEM_BOOT, 20);
            break;
        case LOG_Unlock:
        case LOG_Lock: {
            memcpy_P(lines[1], entry.event == LOG_Lock ? LOCKED : UNLOCKED, 20);
            User* user = registerReadUser(entry.userId);
            memcpy(lines[2], user->name, constrain(strlen(user->name), 0, 20));
            memcpy(lines[3], user->telNr, strlen(user->telNr));
            break;
        }
        }
    }

    if (buttonEvent == ROT_Released)
        menuState = MenuLogs;

    return;
}

void Display::showLogs(uint8_t buttonEvent, int8_t rotaryDiff)
{
    if (menuState == MenuLogsChoices) {
        logsChoices(buttonEvent, rotaryDiff);
        return;
    }
    else if (menuState == MenuLog) {
        showSingleLog(buttonEvent, rotaryDiff);
        return;
    }

    lineCursor += rotaryDiff;
    if (lineCursor > 3) {
        lineOffset = constrain(lineOffset + lineCursor - 3, 0, 9999-4);
        lineCursor = 3;
    }
    else if (lineCursor < 0) {
        lineOffset = constrain(lineOffset + lineCursor, 0, 9999-4);
        lineCursor = 0;
    }

    for (uint8_t i=0; i < 4; i++) {

        LogEntry entry;

        memset(&entry, 0, sizeof(entry));
        logReadEntry(i + lineOffset, &entry);

        //snprintf(lines[i], 21, " %4d:              ", i + lineOffset);
        if (entry.ts) {
            char* c = lines[i]+1;
            *c++ = entry.event;
            printShortDateTime(c, 14, entry.ts); // 11 chars
            c += 12;
            if (entry.userId == INVALID_CARD) {
                *c++ = 'S';
                *c++ = 'y';
                *c++ = 's';
                *c++ = ' ';
                *c++ = ' ';
                *c++ = ' ';
            }
            else {
                User* user = registerReadUser(entry.userId);
                memcpy(c, user->name, constrain(strlen(user->name), 0, 6));
                //snprintf(c, 4, "%3d", entry.userId);
            }
        }
        else {
            memcpy_P(lines[i]+2, EMPTY_LOG, 18);
        }
    }
    lines[lineCursor][0] = '>';
    
    if (buttonEvent == ROT_Released) {
        menuState = MenuLogsChoices;
    }
}

void Display::refresh()
{
    uint8_t buttonEvent = getRotaryButton();
    int8_t  rotaryDiff  = getRotaryDiff();

    if (buttonEvent || rotaryDiff)
        dirty = true;
  
    if (dirty || (millis() - refreshTS > 100)) {

        clear();

        switch(menuState) {

        case MenuUsers:
        case MenuUserEdit:
        case MenuUsersChoices:
            showUsers(buttonEvent, rotaryDiff);
            break;

      // case MenuUserEdit:
      //     break;
        case MenuLog:
        case MenuLogs:
        case MenuLogsChoices:
            showLogs(buttonEvent, rotaryDiff);
            break;

        case MenuMain:
        case MenuMainChoices:

        default:
            mainScreen(buttonEvent, rotaryDiff);
            break;
        }

        printScreen();

        refreshTS = millis();
        dirty = false;
  }
}
