#include <Arduino.h>
#include "CardReader.h"
#include "debug.h"

#define LED_SOLID_GREEN 0x05
#define LED_BLINK_GREEN 0x04

#define LED_SOLID_RED   0x0A
#define LED_BLINK_RED   0x08

const uint8_t LED_DATA[6][2] = {
    { LED_SOLID_GREEN | 0x20, 0xFF },
    { LED_BLINK_GREEN | 0x20, 0x0A },
    { LED_SOLID_RED | 0x20, 0xFF },
    { LED_BLINK_RED | 0x20, 0x0A },
    { LED_SOLID_GREEN | LED_SOLID_RED | 0x20, 0x0A},
    { LED_BLINK_GREEN | LED_BLINK_RED | 0x20, 0x0A}
};


void CardReader::begin()
{
    // Start serial link
    com.begin(9600);
}

void CardReader::init()
{
    // Version command
    while(!sendCmd(0x00)) {
        debugPrintln(F("## Card reader KO (version)"));
    }

    // EXTERN authentication
    while(!sendCmd(0x06)) {
        debugPrintln(F("## Card reader KO (extern)"));
    }
}

bool CardReader::poll(uint8_t* buffer)
{
    if (com.available()) {

        uint8_t b = com.read();
        switch(b) {

        case '+':
            debugPrint(F("## Card inserted: "));
            delay(50);

            // IDENTIFY
            if (sendCmd(0x16, buffer, 9)) {
                uint8_t crc = 0;
                for (uint8_t i=0; i < 8; i++) {
                    crc += buffer[i];
                    if (buffer[i] < 0x10) debugPrint('0');
                    debugPrint(buffer[i], HEX);
                }
                crc ^= buffer[8];

                if (crc) debugPrint(F(" ## CRC error"));
                debugPrintln();

                if (!crc)
                    return true;
            }
            
            debugPrintln(F("## Error while reading card"));
            setLed(LedBlinkRed, 1500);
            return false;

        case '-':
            debugPrintln(F("## Card removed"));
            break;

        default:
            break;
        }
    }

    unsigned long now = millis();
    if (!refresh_ts || now - refresh_ts > refresh_delay) {
        debugPrintln(F("## ping LEDs"));
        if (locked)
            sendCmd(0x15, (uint8_t*)LED_DATA[LedSolidRed], 2);
        else
            sendCmd(0x15, (uint8_t*)LED_DATA[LedSolidGreen], 2);
        refresh_ts = millis();
        refresh_delay = 10000;
    }

    return false;
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
            //debugPrintln("-> 0x2A");
            st = CmdInit;
            ts = millis();
            break;

        case CmdInit:
            if (com.available()) {

                uint8_t b = com.read();
                //debugPrint("<- 0x");
                //debugPrintln(b, HEX);

                if (b == 0xBB) {
                    com.write(cmd);
                    com.flush();
                    //debugPrint("-> 0x");
                    //debugPrintln(cmd, HEX);

                    st = CmdCmd;
                    ts = millis();
                }
                else {
                    st = CmdError;
                    //debugPrint("CmdInit: wrong byte read: 0x");
                    //debugPrintln(b, HEX);
                }
            }
            else if (millis() - ts > 200) {
                st = CmdError;
                //debugPrintln("CmdInit: timeout");
            }
            break;

        case CmdCmd:
            if (com.available()) {
                
                uint8_t b = com.read();
                //debugPrint("<- 0x");
                //debugPrintln(b, HEX);

                if (b == cmd) {
                    com.write(0xBB);
                    com.flush();
                    //debugPrintln("-> 0xBB");

                    st = CmdExe;
                    ts = millis();
                }
                else {
                    st = CmdError;
                    //debugPrintln("CmdCmd: wrong byte read");
                }
            }
            else if (millis() - ts > 200) {
                st = CmdError;
                //debugPrintln("CmdCmd: timeout");
            }
            break;

        case CmdExe:
            if (com.available()) {

                uint8_t b = com.read();
                //debugPrint("<- 0x");
                //debugPrintln(b, HEX);

                if (b == 0xBB) {
                    st = CmdRcvData;
                    ts = millis();

                    // LEDs
                    if (cmd == 0x15) {
                        st = CmdSendData;
                    }
                }
                else {
                    st = CmdError;
                    //debugPrint("CmdExe: wrong byte read: 0x");
                    //debugPrintln(b, HEX);
                }
            }
            else if (millis() - ts > 300) {
                st = CmdError;
                //debugPrintln("CmdExe: timeout");
            }
            break;

        case CmdRcvData:

            if (!buffer || !len) {
                st = CmdSuccess;
                //debugPrintln();
            }
            else if (com.available()) {
                //debugPrint(".");
                *buffer++ = com.read();
                len--;
            }
            else if (millis() - ts > 600) {
                st = CmdError;
                //debugPrintln();
                //debugPrintln("CmdExe: timeout");
            }
            break;

        case CmdSendData:

            if (buffer && len) {
                while(len > 0) {
                    //debugPrint("-> 0x");
                    //debugPrintln(*buffer, HEX);
                    com.write(*buffer++);
                    len--;
                }
            }
            st = CmdCRC;
            ts = millis();
            break;

        case CmdCRC:

            if (com.available()) {
                //debugPrint("<- 0x");
                //debugPrintln(com.read(), HEX);
                st = CmdSuccess;
                //debugPrintln();
            }
            else if (millis() - ts > 100) {
                st = CmdError;
                //debugPrintln();
                //debugPrintln("CmdCRC: timeout");
            }
            break;            
            
        default:
            break;
        }
    }

    return st == CmdSuccess;
}

void CardReader::setLed(LedColor c, unsigned int duration)
{
    sendCmd(0x15, (uint8_t*)LED_DATA[c], 2);

    refresh_delay = duration;
    refresh_ts    = millis();
}
