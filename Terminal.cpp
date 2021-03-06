#include "Seculock.h"
#include "Terminal.h"
#include "Register.h"
#include "Log.h"
#include "debug.h"

#include <Arduino.h>
#include <DS1307RTC.h>


static void help();
static void addCard();
static void editCard();
static void editUser();
static void blockUser();
static void unblockUser();
static void eraseCard();
static void setRtcTime();

static void writeUser(uint8_t slot);

struct SerialCmd {
    const char* cmd;
    void (*func)();
};

static const SerialCmd cmds[] = {
    { "help",         help },
    { "add_card",     addCard },
    { "edit_card",    editCard },
    { "edit_user",    editUser },
    { "block_user",   blockUser },
    { "unblock_user", unblockUser },
    { "erase_card",   eraseCard },
    { "erase_all",    registerEraseCards },
    { "erase_log",    wipeLogTable },
    { "erase_full_log", wipeLogClean },
    { "set_time",     setRtcTime },
    { nullptr, nullptr }
};

static const char* DONE = "> done";
static uint8_t buffer[64];
static uint8_t idx = 0;

static void parseCmd(uint8_t* buffer, uint8_t& idx)
{
    while (idx > 0) {

        if (buffer[idx-1] == '\n'
            || buffer[idx-1] == '\r'
            || buffer[idx-1] == ' ') {

            idx--;
        }
        else
            break;
    }

    buffer[idx] = '\0';
}

static bool exeCmd(uint8_t* buffer, uint8_t len)
{
    const SerialCmd* p = cmds;

    while (p->cmd) {
        if (!strncmp(p->cmd, (const char*)buffer, len)) {
            debugPrint(F("# Executing '"));
            Serial.print((char*)buffer);
            Serial.println(F("'"));
            p->func();
            return true;
        }
        p++;
    }

    return false;
}

static void readLine(char* buffer, uint8_t size)
{
    if (!size) return;
    
    while(size - 1) {

        while(!Serial.available());

        char c = Serial.read();
        switch (c) {

        case '\r':
            break;

        case '\n':
            *buffer = '\0';
            return;

        default:
            *buffer++ = c;
            size--;
            break;
        }
    }

    *buffer = '\0';
}

static bool readYesNo()
{
    bool res = false;

    while(1) {
        while(!Serial.available());
        char c = Serial.read();
        switch(c) {
        case 'y':
            res = true;
            break;

        case '\r':
        case '\n':
            return res;
        }
    }
}

void handleTerminal()
{
    if (!Serial.available())
        return;

    while(Serial.available() && idx < sizeof(buffer))
        buffer[idx++] = Serial.read();

    if (idx == sizeof(buffer)) {
        idx = 0;
        Serial.println(F("# Command buffer overflow"));
        return;
    }

    if (!idx || (buffer[idx-1] != '\n'))
        return;

    buffer[idx] = '\0';

    switch(buffer[0]) {
    case 'D':
        debugTrace = !debugTrace;
        Serial.print(F("debug "));
        Serial.println(debugTrace ? F("ON") : F("OFF"));
        break;

    case 'R':
        // Read all 64KB from EERPOM
        for (unsigned long x = 0 ; x < 0x400 ; x++) {
            prom.read(x << 6, buffer, sizeof(buffer));
            Serial.write(buffer, sizeof(buffer));
        }
        break;

    case 'W':
        // Write all 64KB to EEPROM
        for (unsigned long x = 0 ; x < 0x400 ; x++) {

            size_t rb = Serial.readBytes(buffer, sizeof(buffer));
            prom.write(x << 6, buffer, rb);

            Serial.print("0x");
            Serial.println(x, HEX);
        }
        Serial.println(DONE);

        // re-init log after EEPROM write
        logInit();
        break;

    case 'T': {
        time_t pctime = strtoul((const char*)buffer+1, nullptr, 10);
        RTC.set(pctime);
        setTime(pctime);
        break;
    }

    default:
        //TODO:
        // - cut command into pieces
        // - search command in array
        // - execute command
        parseCmd(buffer, idx);

        if (idx && !exeCmd(buffer, idx)) {
            Serial.print(F("# Unknown command '"));
            Serial.print((char*)buffer);
            Serial.println('\'');
        }
        else {
            Serial.println(DONE);
        }
        break;
    }

    // reset command
    idx = 0;
}

static void addCard()
{
    // Serial.println(F("> Enter slot #"));

    // while(!Serial.available());
    // uint8_t slot = Serial.parseInt();

    // Serial.print(F("Slot = "));
    // Serial.print(slot, DEC);

    Serial.println(F("> Insert Card"));
    while(!cardReader.poll(buffer));

    uint8_t slot = registerFindCard(buffer);
    if (slot != INVALID_CARD) {
        Serial.print(F("Card already exist at slot "));
        Serial.println(slot, DEC);
        return;
    }

    slot = registerFindFreeSlot();
    if (slot == INVALID_CARD) {
        Serial.println(F("No empty slot found"));
        return;
    }

    registerWriteCardId(buffer, slot);
    cardReader.setLed(CardReader::LedBlinkGreen, 1500);

    Serial.print(F("> Card written at slot "));
    Serial.println(slot, DEC);

    writeUser(slot);
}

static void editCard()
{
    Serial.println(F("> Enter slot #"));

    while(!Serial.available());
    uint8_t slot = Serial.parseInt();
    Serial.read(); // eat LF

    Serial.println(F("> Insert Card"));
    while(!cardReader.poll(buffer));

    uint8_t f_slot = registerFindCard(buffer);
    if (f_slot != INVALID_CARD) {
        if (f_slot != slot) {
            Serial.print(F("Card already exist at slot "));
            Serial.println(f_slot, DEC);
        }
    } else if (f_slot == slot) {
        Serial.print(F("Card was already at slot"));
        Serial.println(slot, DEC);
    }
    else {
        registerWriteCardId(buffer, slot);
        cardReader.setLed(CardReader::LedBlinkGreen, 1500);
    }
}

static void writeUser(uint8_t slot)
{
    User* user = (User*)registerGetBuffer();
    Serial.print(F("> Enter name (max 45 chars): "));
    readLine(user->name, sizeof(User::name));
    Serial.println(user->name);

    Serial.print(F("> Enter telephone number (max 15 chars): "));
    readLine(user->telNr, sizeof(User::telNr));
    Serial.println(user->telNr);

    Serial.print(F("> Is admin? (y/n): "));
    if (readYesNo()) {
        user->flags |= USER_ADMIN;
        Serial.println(F("yes"));
    }
    else {
        Serial.println(F("no"));
    }

    registerWriteUser(user, slot);
}

static void eraseCard()
{
    Serial.println(F("> Enter slot #"));

    while(!Serial.available());
    uint8_t slot = Serial.parseInt();
    Serial.read(); // eat LF

    registerEraseCard(slot);

    Serial.print(F("Slot "));
    Serial.print(slot, DEC);
    Serial.println(F(" erased"));
}

static void editUser()
{
    Serial.println(F("> Enter slot #"));

    while(!Serial.available());
    uint8_t slot = Serial.parseInt();
    Serial.read(); // eat LF
    
    User* user = registerReadUser(slot);

    Serial.print(F("> Name: "));
    Serial.println(user->name);

    Serial.print(F("> Tel: "));
    Serial.println(user->telNr);

    Serial.print(F("> Flags: "));

    if (user->flags & USER_DENIED)
        Serial.print(F("disabled "));

    if (user->flags & USER_ADMIN)
        Serial.print(F("admin "));

    Serial.println();

    Serial.print(F("> Edit User? (y/n): "));
    if (readYesNo()) {
        Serial.println(F("yes"));
        writeUser(slot);
    }
    else {
        Serial.println(F("no"));
    }
}

static void blockUser()
{
    Serial.println(F("> Enter slot #"));

    while(!Serial.available());
    uint8_t slot = Serial.parseInt();
    Serial.read(); // eat LF
    
    User* user = registerReadUser(slot);
    Serial.print(F(">  "));
    Serial.println(user->name);
    Serial.print(F(">  "));
    Serial.println(user->telNr);


    Serial.print(F("> Block User? (y/n): "));
    if (readYesNo()) {
        Serial.println(F("yes"));

        user->flags |= USER_DENIED;
        registerWriteUser(user, slot);
    }
    else {
        Serial.println(F("no"));
    }
}

static void unblockUser()
{
    Serial.println(F("> Enter slot #"));

    while(!Serial.available());
    uint8_t slot = Serial.parseInt();
    Serial.read(); // eat LF
    
    User* user = registerReadUser(slot);
    Serial.print(F(">  "));
    Serial.println(user->name);
    Serial.print(F(">  "));
    Serial.println(user->telNr);


    Serial.print(F("> Unblock User? (y/n): "));
    if (readYesNo()) {
        Serial.println(F("yes"));

        user->flags &= ~USER_DENIED;
        registerWriteUser(user, slot);
    }
    else {
        Serial.println(F("no"));
    }
}

static void help()
{
    Serial.println(F("> Commands:"));

    const SerialCmd* p = cmds;
    while (p->cmd) {
        Serial.println(p->cmd);
        p++;
    }
}

static void setRtcTime()
{
    Serial.println(F("> Enter time string (unix timestamp):"));
    const unsigned long DEFAULT_TIME = 1609459200; // Jan 1 2021 
    while(!Serial.available());
    unsigned long pctime = Serial.parseInt();

    if( pctime < DEFAULT_TIME) { // check the value is a valid time (greater than Jan 1 2013)
        pctime = DEFAULT_TIME; // return 0 to indicate that the time is not valid
    }

    RTC.set(pctime);
    setTime(pctime);
}
