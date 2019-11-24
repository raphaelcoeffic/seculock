#include "CardReader.h"
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

// RTC support
#include <Time.h>
#include <DS1307RTC.h>
#include <Timezone.h>

//Central European Time (Frankfurt, Paris)
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     //Central European Summer Time
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};       //Central European Standard Time
Timezone CE(CEST, CET);

uint32_t sync_time()
{
  return RTC.get();
}

// Seculock com
CardReader cardReader(10, 9);

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

LiquidCrystal_I2C lcd(LCD_I2C_ADDR,
                      LCD_I2C_EN,
                      LCD_I2C_RW,
                      LCD_I2C_RS,
                      LCD_I2C_D4,
                      LCD_I2C_D5,
                      LCD_I2C_D6,
                      LCD_I2C_D7,
                      LCD_I2C_BL,
                      POSITIVE);

void setup()
{
    Wire.begin();
    Wire.setClock(400000);

    // Sync the Time lib with the RTC
    setSyncProvider(sync_time);
    
    // Init LCD
    lcd.begin(20, 4);
    lcd.print("Hello World!");
    
    // Serial Terminal
    Serial.begin(115200);
    Serial.println("## Serial OK");

    // Seculock connection
    cardReader.begin();
    Serial.println("## Card reader OK");
}

void printDigits(int digits){
  // utility function for digital clock display:
  // prints preceding colon and leading 0
  if(digits < 10)
    lcd.print('0');
  lcd.print(digits);
}

const uint8_t leds[2] = { 0x15, 0xF0 };

void loop()
{
    static unsigned long ts = 0;
    
    cardReader.poll();

    time_t local = CE.toLocal(now());

    lcd.setCursor(2,2);
    printDigits(hour(local));
    lcd.print(":");
    printDigits(minute(local));
    lcd.print(":");
    printDigits(second(local));

    lcd.setCursor(2,3);
    lcd.print(millis()/1000);

    if (millis() - ts > 10000) {
        cardReader.sendCmd(0x15, leds, 2);
        ts = millis();
    }
}
