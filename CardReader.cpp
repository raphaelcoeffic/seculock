#include <Arduino.h>
#include "CardReader.h"


#define LED_SOLID_GREEN 0x05
#define LED_BLINK_GREEN 0x04

#define LED_SOLID_RED   0x0A
#define LED_BLINK_RED   0x08

enum {
    LedSolidGreen,
    LedBlinkGreen,
    LedSolidRed,
    LedBlinkRed
};

const int8_t LED_DATA[6][2] = {
    { LED_SOLID_GREEN | 0x20, 0x0A },
    { LED_BLINK_GREEN | 0x20, 0x0A },
    { LED_SOLID_RED | 0x20, 0x0A },
    { LED_BLINK_RED | 0x20, 0x0A },
    { LED_SOLID_GREEN | LED_SOLID_RED | 0x20, 0x0A},
    { LED_BLINK_GREEN | LED_BLINK_RED | 0x20, 0x0A}
};



void CardReader::begin()
{
    // Start serial link
    com.begin(9600);

    // Version command
    while(!sendCmd(0x00)) {
        Serial.println("## Card reader KO (version)");
    }

    // EXTERN authentication
    while(!sendCmd(0x06)) {
        Serial.println("## Card reader KO (extern)");
    }
}

void CardReader::poll()
{
    uint8_t buffer[256];
    
    if (com.available()) {

        uint8_t b = com.read();
        switch(b) {

        case '+':
            Serial.println("## Card inserted");

            // IDENTIFY
            if (sendCmd(0x16, buffer, 9)) {
                Serial.print("## Card ID: ");
                for (uint8_t i=0; i < 8; i++) {
                    Serial.print(buffer[i], HEX);
                }
                Serial.println();
                //sendCmd(0x15, LED_DATA[4], 2); // LEDs
                //sendCmd(0x08); // Contact
                return;
            }

            Serial.println("## Error while reading card");
            sendCmd(0x15, LED_DATA[3], 2); // LEDs
            break;

        case '-':
            Serial.println("## Card removed");
            break;

        default:
            break;
        }
    }
}

bool CardReader::sendCmd(uint8_t cmd, uint8_t* buffer, uint16_t len)
{
    enum CmdState {
        CmdStart,
        CmdInit, // 0x2A
        CmdCmd,  // [cmd byte]
        CmdExe,  // 0xBB
        CmdRcvData, // [cmd data]
        CmdSendData, // [cmd data]
        CmdCRC,

        CmdError,
        CmdSuccess
    };

    CmdState st = CmdStart;
    unsigned long ts = 0;

    while(st < CmdError) {

        switch(st) {

        case CmdStart:
            com.write(0x2A);
            com.flush();
            Serial.println("-> 0x2A");
            st = CmdInit;
            ts = millis();
            break;

        case CmdInit:
            if (com.available()) {

                uint8_t b = com.read();
                Serial.print("<- 0x");
                Serial.println(b, HEX);

                if (b == 0xBB) {
                    com.write(cmd);
                    com.flush();
                    Serial.print("-> 0x");
                    Serial.println(cmd, HEX);

                    st = CmdCmd;
                    ts = millis();
                }
                else {
                    st = CmdError;
                    //Serial.print("CmdInit: wrong byte read: 0x");
                    //Serial.println(b, HEX);
                }
            }
            else if (millis() - ts > 200) {
                st = CmdError;
                Serial.println("CmdInit: timeout");
            }
            break;

        case CmdCmd:
            if (com.available()) {
                
                uint8_t b = com.read();
                Serial.print("<- 0x");
                Serial.println(b, HEX);

                if (b == cmd) {
                    com.write(0xBB);
                    com.flush();
                    Serial.println("-> 0xBB");

                    st = CmdExe;
                    ts = millis();
                }
                else {
                    st = CmdError;
                    Serial.println("CmdCmd: wrong byte read");
                }
            }
            else if (millis() - ts > 200) {
                st = CmdError;
                Serial.println("CmdCmd: timeout");
            }
            break;

        case CmdExe:
            if (com.available()) {

                uint8_t b = com.read();
                Serial.print("<- 0x");
                Serial.println(b, HEX);

                if (b == 0xBB) {
                    st = CmdRcvData;
                    ts = millis();

                    // LEDs
                    if (cmd == 0x15) {
                    //     // All green
                    //     com.write(0x18);
                    //     Serial.println("-> 0x18");
                    //     // time
                    //     com.write(0x08);
                    //     Serial.println("-> 0x08");
                        st = CmdSendData;
                    }
                }
                else {
                    st = CmdError;
                    Serial.print("CmdExe: wrong byte read: 0x");
                    Serial.println(b, HEX);
                }
            }
            else if (millis() - ts > 200) {
                st = CmdError;
                Serial.println("CmdExe: timeout");
            }
            break;

        case CmdRcvData:

            if (!buffer || !len) {
                st = CmdSuccess;
                Serial.println();
            }
            else if (com.available()) {
                Serial.print(".");
                *buffer++ = com.read();
                len--;
            }
            else if (millis() - ts > 600) {
                st = CmdError;
                Serial.println();
                Serial.println("CmdExe: timeout");
            }
            break;

        case CmdSendData:

            if (buffer && len) {
                while(len > 0) {
                    Serial.print("-> 0x");
                    Serial.println(*buffer, HEX);
                    com.write(*buffer++);
                    len--;
                }
            }
            st = CmdCRC;
            ts = millis();
            break;

        case CmdCRC:

            if (com.available()) {
                Serial.print("<- 0x");
                Serial.println(com.read(), HEX);
                st = CmdSuccess;
                Serial.println();
            }
            else if (millis() - ts > 100) {
                st = CmdError;
                Serial.println();
                Serial.println("CmdCRC: timeout");
            }
            break;            
            
        default:
            break;
        }
    }

    return st == CmdSuccess;
}

