#include "CardReader.h"
#include "DoorLock.h"
#include "Display.h"
#include "RotaryEncoder.h"
#include "Terminal.h"
#include "Register.h"
#include "debug.h"

// Modded AltSoftSerial (local)
#include "AltSoftSerial.h"

#include <Wire.h>
#include <extEEPROM.h>

// RTC support
#include <Time.h>
#include <DS1307RTC.h>

// Seculock com
CardReader cardReader(10, 9);

// LCD Display
Display disp;

// Pair of 24LC256
extEEPROM prom(kbits_256, 2, 64);

const uint8_t RotaryKnobPin = A0;
const uint8_t DoorButtonPin = 14;

uint32_t sync_time()
{
  return RTC.get();
}

void setup()
{
    Wire.begin();
    Wire.setClock(400000);

    DoorLock::begin();
    pinMode(RotaryKnobPin, INPUT_PULLUP);
    pinMode(DoorButtonPin, INPUT_PULLUP);

    // Init rotary encoder PCINT
    initRotaryPCINT();

    // Sync the Time lib with the RTC
    setSyncProvider(sync_time);

    // Init LCD
    disp.begin();
    
    // Serial Terminal
    Serial.begin(115200);
    Serial.println("## Serial OK");

    // Seculock connection
    cardReader.begin();
    cardReader.init();
    Serial.println("## Card reader OK");
}

void printInt32(int32_t i32)
{
  char buffer[12] = {' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ', '\0'};
  bool neg = false;

  if (i32 < 0) {
    neg = true;
    i32 = -i32;
  }

  int8_t i = 10;
  for(; i > 0; i--) {
    buffer[i] = (i32 % 10) + '0';
    i32 = i32 / 10;
    if (i32 == 0) {
      i--;
      break;
    }
  }

  if (neg)
    buffer[i] = '-';

  disp.print(buffer);
}

const char hexDigits[]  = "0123456789ABCDEF";
const char eepromData[] = "Dead-Beef       ";

void loop()
{
  static unsigned long ts = 0;
  static unsigned long disp_ts = 0;
  static uint8_t lastCardSlot = INVALID_CARD;

  // static char lastID[17] = { 0 };

  uint8_t cardId[9];

  if (cardReader.poll(cardId)) {

    uint8_t slot = registerFindCard(cardId);
    if (slot != INVALID_CARD) {
    
      // for (int8_t i=0; i < sizeof(lastID)-1; i+=2) {
      //   lastID[i] = hexDigits[cardBuffer[i/2] & 0xF];
      //   lastID[i+1] = hexDigits[(cardBuffer[i/2] & 0xF0) >> 4];
      // }

      cardReader.setLed(CardReader::LedBlinkGreen, 1500);

      if (cardReader.getLocked()) {
        cardReader.setLocked(false);
        lastCardSlot = slot;
      }
      else {
        cardReader.setLocked(true);
        lastCardSlot = INVALID_CARD;
      }
    }
    else {
      cardReader.setLed(CardReader::LedBlinkRed, 1500);
    }
  }

  if (millis() - disp_ts > 200) {

    disp.setCursor(0,0);
    disp.printTime();
    disp.print(F("  "));
    disp.printDate();

    if (lastCardSlot != INVALID_CARD) {
      disp.setCursor(0,2);
      User* user = registerReadUser(lastCardSlot);
      disp.print((const char*)user->name);
    }
    else {
      disp.print(F("                    "));
    }
      
    disp.setCursor(0,3);
    printInt32(getRotary());

    disp.print(F("  "));
    disp.print(digitalRead(RotaryKnobPin));

    disp_ts = millis();
  }

  if (!cardReader.getLocked() && !digitalRead(DoorButtonPin)) {
    DoorLock::open();
  }
  else {
    DoorLock::close();
  }

  handleTerminal();
}
