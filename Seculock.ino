#include "CardReader.h"
#include "DoorLock.h"
#include "Display.h"
#include "RotaryEncoder.h"
#include "Terminal.h"
#include "Register.h"
#include "log.h"

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

#define DoorButtonPin 14

uint8_t cardSlot = INVALID_CARD;


uint32_t sync_time()
{
  return RTC.get();
}

void setup()
{
    Wire.begin();
    Wire.setClock(400000);

    DoorLock::begin();
    pinMode(DoorButtonPin, INPUT_PULLUP);

    // Init rotary encoder
    initRotaryKnob();

    // Sync the Time lib with the RTC
    setSyncProvider(sync_time);

    // Init LCD
    disp.begin();

    //while(!Serial);
    
    // Serial Terminal
    Serial.begin(115200);
    Serial.println("## Serial OK");

    // Init logging
    logInit();
    logWriteEntry(LOG_Boot, INVALID_CARD);

    // Seculock connection
    cardReader.begin();
    cardReader.init();
    Serial.println("## Card reader OK");
}

// const char hexDigits[]  = "0123456789ABCDEF";
// const char eepromData[] = "Dead-Beef       ";

void loop()
{
  static unsigned long disp_ts = 0;

  uint8_t cardId[9];

  if (cardReader.poll(cardId)) {

    uint8_t slot = registerFindCard(cardId);
    if (slot != INVALID_CARD) {

      User* user = registerReadUser(slot);
      if (!(user->flags & USER_DENIED)) {
        
        cardReader.setLed(CardReader::LedBlinkGreen, 1500);

        if (cardReader.getLocked()) {
          cardReader.setLocked(false);
          cardSlot = slot;
          logWriteEntry(LOG_Unlock, slot);
        }
        else {
          cardReader.setLocked(true);
          cardSlot = INVALID_CARD;
          logWriteEntry(LOG_Lock, slot);
        }

        disp.setDirty();
      }
      else {
        Serial.println("# user denied");
        cardReader.setLed(CardReader::LedBlinkRed, 1500);
      }
    }
    else {
      cardReader.setLed(CardReader::LedBlinkRed, 1500);
    }
  }

  disp.refresh();

  if (!cardReader.getLocked() && !digitalRead(DoorButtonPin)) {
    DoorLock::open();
  }
  else {
    DoorLock::close();
  }

  handleTerminal();
}
