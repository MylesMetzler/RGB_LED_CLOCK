#include <Adafruit_NeoPixel.h>
#include <TinyWireM.h>
#include "TinyRTClib.h"

#define BUTTON_PLUS_DISP (PINB & B00010000)
#define BUTTON_MINUS_SET (PINB & B00001000)
#define LED_PIN 1

#define DEBUG 1
#define GREEN 0
#define RED 1
#define BLUE 2

RTC_DS1307 RTC;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(60,LED_PIN,NEO_GRB + NEO_KHZ800);

uint8_t colorEmpty[] = {0,0,0};
uint8_t colorHour[] = {0,15,0};
uint8_t colorMinute[] = {0,0,15};
uint8_t colorSecond[] = {15,0,0};
uint8_t widthHour = 0;
uint8_t widthMinute = 0;
uint8_t widthSecond = 0;

uint8_t set = 0;
uint8_t clockMode = 0;
uint8_t pbPlusDisp = 0;
uint8_t pbMinusSet = 0;
uint8_t setPulseCount = 0;
uint8_t timeUpdate = 0;
uint8_t cHour;
uint8_t cMinute;
uint8_t cSecond;
uint8_t rapidSetCount;
uint8_t setPulseDir = 1;

void setup()
{
  //code size requires NO pinmode/digitalRead ETC
  DDRB =   B11000111; //set pins 3,4,5 to inputs leave the rest alone
  PORTB |= B00011000; // set pins az 3 & 5 to high (enable pullups) leave the rest alone
  DDRB |=  B00000010;
  PORTB |= B11111101;
  TinyWireM.begin();
  RTC.begin();
  //strip.begin(); //replaced this call with direct port assignments save code size
  strip.show();
  
  if (!RTC.isrunning())
  {
    RTC.adjust(DateTime(__DATE__,__TIME__));
  }
}

void loop()
{
  DateTime now = RTC.now();
  uint8_t cbPlusDisp = BUTTON_PLUS_DISP;
  uint8_t cbMinusSet = BUTTON_MINUS_SET;
  int light = analogRead(0);
  light = 16 - (light/64); 
  if (set < 10) //not setting the time.
  {
    rapidSetCount = 0;
    cHour = now.hour();
    cMinute = now.minute();
    cSecond = now.second();
    if (cbPlusDisp != pbPlusDisp && !cbPlusDisp)
    {
      if (clockMode == 0)  //0 = color fade mode
      {
        clockMode = 1;
        colorEmpty[0] = 0;
        colorEmpty[1] = 0;
        colorEmpty[2] = 0;
        widthHour = 0;
        widthMinute = 0;
        widthSecond = 0;
      }
      else if (clockMode == 1)  //1 = single pixel colored time on blank background
      {
        clockMode = 2;
        colorEmpty[0] = 2;
        colorEmpty[1] = 2;
        colorEmpty[2] = 2;
      }
      else if (clockMode == 2) //2 = single pixel colored time on white background
      {
        clockMode = 3;
        colorEmpty[0] = 0;
        colorEmpty[1] = 0;
        colorEmpty[2] = 0;
        widthHour = 2;
        widthMinute = 1;
        widthSecond = 0;
      }
      else if (clockMode == 3) //3 = wide pixel colored time on blank background
      {
        clockMode = 4;
        colorEmpty[0] = 2;
        colorEmpty[1] = 2;
        colorEmpty[2] = 2;
      }
      else if (clockMode == 4) //4 = wide pixel colored time on white background
      {
        clockMode = 5;
      }
      else if (clockMode == 5) //analog input display test
      {
        clockMode = 0;
      }
    }
  
    if (!cbMinusSet)
    {
      set++;
      if (set == 10)
        set = 60;
    }
    else
      set = 0;
  } 
  else //set the time
  {
    set--;
    if (setPulseDir)
      setPulseCount += 1;
    else
      setPulseCount -= 1;
    if (setPulseCount == 32)
      setPulseDir = 0;
    else if (setPulseCount == 0);
      setPulseDir = 1;
    colorEmpty[0] = setPulseCount;
    colorEmpty[1] = setPulseCount;
    colorEmpty[2] = setPulseCount;
    if (!cbMinusSet || !cbPlusDisp)
    {
      set = 60;
      timeUpdate = 1;
    }
    if (cbMinusSet && cbPlusDisp)
      rapidSetCount = 0;
    if (!cbMinusSet)
    {
      rapidSetCount++;
      if (pbMinusSet != cbMinusSet && rapidSetCount <= 10)
        cMinute--;
      else if (rapidSetCount > 10)
        cMinute--;
      if (cMinute > 59)
      {
        if (cHour == 1)
          cHour = 24;
        else
          cHour--;
        cMinute = 59;
      } 
    }
    if (!cbPlusDisp)
    {
      rapidSetCount++;
      if (pbPlusDisp != cbPlusDisp && rapidSetCount <= 10)
        cMinute++;
      else if (rapidSetCount > 10)
        cMinute++;
      if (cMinute > 59)
      {
        if (cHour == 24)
          cHour = 1;
        else
          cHour++;
        cMinute = 0;
      }
    }
      
    if(set == 10)
    {
      if (timeUpdate)
      RTC.adjust(DateTime(now.year(),now.month(),now.day(),cHour,cMinute,cSecond));
      set = 0;
      timeUpdate = 0;
    }
  }
  
  if (clockMode == 0 && set < 10)
    fadeTime(cSecond,cMinute,cHour,(uint8_t)light);
  else if (clockMode == 5)
    analogTest();
  else
    standardTime(cSecond,cMinute,cHour,(uint8_t)light);
  strip.show();
  pbPlusDisp = cbPlusDisp;
  pbMinusSet = cbMinusSet;
  delay(100);
}




void standardTime( uint8_t second, uint8_t minute, uint8_t hour, uint8_t light)
{
  uint8_t hourSixty = (hour >= 12 ? hour -= 12 : hour)*5/* + (minute % 12)*/;
  for(uint8_t i = 0; i < 60; i++)
  {
    uint8_t set = 0;
    uint8_t cRed = 0;
    uint8_t cGreen = 0;
    uint8_t cBlue = 0;
    if (getDiff(i,second) <= widthSecond || getDiff(second,i) <= widthSecond)
    {
      set = 1;
      cRed += colorSecond[RED];
      cGreen += colorSecond[GREEN];
      cBlue += colorSecond[BLUE];
    }
    if (getDiff(i,minute) <= widthMinute || getDiff(minute,i) <= widthMinute)
    { set = 1;
      cRed += colorMinute[RED];
      cGreen += colorMinute[GREEN];
      cBlue += colorMinute[BLUE];
    }
    if (getDiff(i,hourSixty) <= widthHour || getDiff(hourSixty,i) <= widthHour)
    {
      set = 1;
      cRed += colorHour[RED];
      cGreen += colorHour[GREEN];
      cBlue += colorHour[BLUE];
    }
    if (set == 0)
    {
      cRed = colorEmpty[RED];
      cGreen = colorEmpty[GREEN];
      cBlue = colorEmpty[BLUE];
    }
    strip.setPixelColor(i,strip.Color(cRed*light,cGreen*light,cBlue*light));
  }
}


void fadeTime( uint8_t second, uint8_t minute, uint8_t hour, uint8_t light)
{
  uint8_t hourSixty = (hour >= 12 ? hour -= 12 : hour)*5/* + (minute % 12)*/;
  for(uint8_t i = 0; i < 60; i++)
  {
    strip.setPixelColor(i,strip.Color(fadeColor(i, hourSixty, light),fadeColor(i, second, light),fadeColor(i, minute, light)));
  }
}

uint8_t fadeColor(uint8_t index, uint8_t time, uint8_t light)
{
  uint8_t lightdiv = light/2;
  uint8_t diff = getDiff(index,time);
  if (light <= 1)
  {
    if (diff >= 16)
      return 0;
    else
      return 16 - diff;
  }
  else
  {  
    if (diff >= 32)
      return 0;
    else
      return (32 - diff)*lightdiv;
  }
}

uint8_t getDiff(uint8_t index, uint8_t time)
{
  uint8_t diff = 32;
  if (index <= time)
    diff = time - index;
  else if (time < diff && index > time)
    diff = 60 - index + time;
  return diff;
}

void analogTest()
{
  int aval= analogRead(0);
  aval = 64 - (aval / 16);
  for (uint8_t i = 0; i < 60; i++)
  {
    if (i < aval)
    strip.setPixelColor(i,strip.Color(128,0,0));
    else
      strip.setPixelColor(i,strip.Color(0,0,0));
  }
  strip.show();
}
