// OLED_Display
/*********************************************************************
This is an example for our Monochrome OLEDs based on SSD1306 drivers

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/category/63_98

This example is for a 128x64 size display using I2C to communicate
3 pins are required to interface (2 I2C and one reset)

Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!

Written by Limor Fried/Ladyada  for Adafruit Industries.
BSD license, check license.txt for more information
All text above, and the splash screen must be included in any redistribution
*********************************************************************/

#include <SPI.h> //just for compilation - to not show errors
#include <Wire.h> //I2C communication
// #include <Adafruit_GFX.h> //Base graphic library
// #include <ESP_SSD1306.h> //OLED display controling library

//16x2 display Version
#include <LiquidCrystal_I2C.h>
#include "symbolsDefine.h"

#define OLED_DISPLAY_ADDRESS 0x3c
// #define LCD_DISPLAY_ADDRESS 0X27 //Starší display
#define LCD_DISPLAY_ADDRESS 0x3F //Novější display
#define LCD_DISPLAY_ROWS_COUNT 2
#define LCD_DISPLAY_COLS_COUNT 16


LiquidCrystal_I2C lcd(LCD_DISPLAY_ADDRESS, LCD_DISPLAY_COLS_COUNT, LCD_DISPLAY_ROWS_COUNT); // set the lcd address to 0x27 for a 16 chars and 2 line display
// ESP_SSD1306 oledDisplay(-1);// -1 means no reset pin, library edited

#define serviceAreaDisplayStartCol 1
#define serviceAreaDisplayStartRow 1

#define ERASE_SERVICE_AREA_ONLY 0
#define ERASE_SERVICE_AREA_AND_PIPE_TEMP_AREA 1

bool lcdDisplayConnected_global = false;
bool oledDisplayConnected_global = false;

//////// Animation definitions ////////
#define SYMBOL_ANIMATION_ON true
#define SYMBOL_ANIMATION_OFF false
byte * setActiveAnimation(byte symbol[][8], uint16_t animationStepDuration, uint8_t animationFramesCount);
//////// //////// //////// //////// ///

void i2cBus_setup()
{
  // if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:i2cBus_setup()"));
  // Wire.begin(SCL, SDA); //Wire.begin(int sda, int scl) !!! labels GPIO4 and GPIO5 on EPS8266 are swaped at old esps!!!!
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN); //Wire.begin(int sda, int scl) !!! labels GPIO4 and GPIO5 on EPS8266 are swaped at old esps!!!!

  Wire.beginTransmission(LCD_DISPLAY_ADDRESS);
  uint8_t result = Wire.endTransmission();
  if (Wire.endTransmission() == 0)
  {
    lcd.begin();
    lcdDisplayConnected_global = true;
    displayServiceLine(cE("Initialization.."));

    createCustomLcdChar(0, wifiNOTConnectedSymbol);
    lcd.setCursor(0, 1);
    lcd.write(0);//WiFi symbol
  
  }
  else
    if(SHOW_WARNING_DEBUG_MESSAGES) DEBUG_OUTPUT.println(sE("!!WARNING: 16X2 LCD display not found! Error:") + result); //error codes:      0:success     1:data too long to fit in transmit buffer     2:received NACK on transmit of address     3:received NACK on transmit of data     4:other error https://www.arduino.cc/en/Reference/WireEndTransmission

  // Wire.beginTransmission(OLED_DISPLAY_ADDRESS);
  // if (Wire.endTransmission() == 0)
  //   oledDisplay_setup();
  // else
  //   if(SHOW_WARNING_DEBUG_MESSAGES) DEBUG_OUTPUT.println(E("!!WARNING: OLED display not found!"));
  yield();
 }


void lcdCreateScaleChars()
{ 
  createCustomLcdChar(1, signalStrengthSymbolAnimation[0]);
  createCustomLcdChar(2, signalStrengthSymbolAnimation[1]);
  createCustomLcdChar(3, signalStrengthSymbolAnimation[2]);
  createCustomLcdChar(4, signalStrengthSymbolAnimation[3]);
  createCustomLcdChar(5, signalStrengthSymbolAnimation[4]);
  createCustomLcdChar(6, signalStrengthSymbolAnimation[5]);
}

void createCustomLcdChar(uint8_t charPosition, uint8_t progmemSymbol[])
{
  uint8_t temporarySymbol[8];
  for(uint8_t row = 0 ; row < 8; row++) 
    temporarySymbol[row] = pgm_read_byte(&progmemSymbol[row]);
  lcd.createChar(charPosition, temporarySymbol);
}

#define SHOWER_SYMBOL 2
#define DEGREE_CELSIUS_SYMBOL 3
#define BOILER_SYMBOL 4
#define DEGREE_SYMBOL 5
#define WATER_DROP_SYMBOL 6
#define TEMP_SYMBOL 7

void lcd_setup()
{ 
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:lcd_setup()"));

  displayServiceLine(cE("")); //erase first row
  // initialize the lcd
  createCustomLcdChar(SHOWER_SYMBOL, showerSymbolAnimation[0]);
  createCustomLcdChar(DEGREE_CELSIUS_SYMBOL, degreeCelsiusSymbol);
  createCustomLcdChar(BOILER_SYMBOL, boilerSymbol);
  createCustomLcdChar(DEGREE_SYMBOL, degreeSymbol);
  createCustomLcdChar(WATER_DROP_SYMBOL, waterDropSymbol);
  createCustomLcdChar(TEMP_SYMBOL, tempSymbol);

  lcd.setCursor(9, 0);
  lcd.write(WATER_DROP_SYMBOL);

  lcd.setCursor(0, 0);
  lcd.write(SHOWER_SYMBOL);
  displayData_loop(0);
}


// void oledDisplay_setup()
// {
//   if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:oledDisplay_setup()"));
//   oledDisplayConnected_global = true;
//   // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
//   oledDisplay.begin(SSD1306_SWITCHCAPVCC, OLED_DISPLAY_ADDRESS);  // initialize with the I2C addr 0x3c (for the 128x64)
//   oledDisplay.clearDisplay();
//   oledDisplay.display();

//   //if(!DEBUG_REMOTE_CONSOLE) remoteConsole.assignDisplayOutput(&oledDisplay, 0,2,150);
//   oledDisplay.setTextSize(1);
//   oledDisplay.setTextColor(WHITE, BLACK);
// }

void displayData_loop(int)
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug(RESET_SPACE_COUNT_DEBUG) + E("F:displayData_loop()"));  
  char bufferCharConversion[10];//temporarily holds data from vals

  if(lcdDisplayConnected_global)
  {
    int16_t spareComfortWater = getSpareComfortWater();
    displayPrintAt(((spareComfortWater == 0? sE("??"): (String)spareComfortWater) + E("L  ")), 1, 0);

    displayPrintAt((String)(int16_t)getSpareHotWater(), 10, 0); // prints rest of watter in Littres (100L minus spotřeba)
    lcd.write(TEMP_SYMBOL);
    if(isTemperatureCorrectMeasurment(GLOBAL.TEMP.lastHeated))
    {
      displayPrint((String)(uint8_t)GLOBAL.TEMP.lastHeated); // prints temp on display
      lcd.write(DEGREE_CELSIUS_SYMBOL);//Degree Symbol°c
      displayPrint(E("  ")); // prints erase 
    }
    else
      displayPrint(E("?")); // prints
  }

  // Uptime - posledni nahrata teplota
  // Minus Cas do nizkeho tarifu - nastavena teplota ohrevu
  // Normal time - actual temp
  if(lcdDisplayConnected_global || oledDisplayConnected_global)
  {
    //watter flowing  
    if(isWaterFlowingLastStateFlag_global || isDebugButtonPressed())
    {
      float waterFlowInLitresPerMinute = (isDebugButtonPressed()) ? 2.5 : getCurentWaterFlowInLitresPerMinute();
      if(waterFlowInLitresPerMinute > 1)
      {
        if(displayRotationPosition_global % 3 == 0)
        displayServiceMessageWithSymbol((String)dtostrf(waterFlowInLitresPerMinute, 1, 1, bufferCharConversion) + E("l/m"), setActiveAnimation(waterFlowSymbolAnimation, 280, 2), SYMBOL_ANIMATION_ON, ERASE_SERVICE_AREA_AND_PIPE_TEMP_AREA); //first num is mininum width, second is precision
        else if(displayRotationPosition_global % 3 == 1)
        displayServiceMessageWithSymbol((String)getSpareMinutesOfHotWater(waterFlowInLitresPerMinute) + E("minut"), setActiveAnimation(showerSymbolAnimation, 250, 2 ), SYMBOL_ANIMATION_ON, ERASE_SERVICE_AREA_AND_PIPE_TEMP_AREA);
        else if(displayRotationPosition_global % 3 == 2)
        displayServiceMessageWithSymbol(getNowTimeString(), timeSymbol, SYMBOL_ANIMATION_OFF, ERASE_SERVICE_AREA_AND_PIPE_TEMP_AREA);

        //PipeTemp
        displayTempInServiceAreaWithSymbol(GLOBAL.TEMP.sensors[PIPE].temp, waterFlowSymbolAnimation[1]);
      }
    }
    else//Normal state - (NO Watter)
    {
      if(displayRotationPosition_global % 6 <= 2)
      {
        //actual temp / UPTIME
        displayServiceMessageWithSymbol(getUpTime(false), (isWifiConnected()? wifiConnectedSymbol : wifiNOTConnectedSymbol), SYMBOL_ANIMATION_OFF, ERASE_SERVICE_AREA_ONLY);
        displayTempInServiceAreaWithSymbol(GLOBAL.TEMP.topHeating, upTempSymbol);
      }
      else if(displayRotationPosition_global % 6 <= 5)
      {
        //Low tarif time / setHeatingTemp
        //Time / actual pipeTemp
        String timeString = getNowTimeString();
        if(displayRotationPosition_global % 6 == 4)
          timeString.replace(E(":"),E(" "));
        displayServiceMessageWithSymbol(timeString, timeSymbol, SYMBOL_ANIMATION_OFF, ERASE_SERVICE_AREA_ONLY);

        displayTempInServiceAreaWithSymbol(GLOBAL.TEMP.sensors[BOJLER].temp, boilerSymbol);
      }
    }
    displayRotationPosition_global++;
  }
}

void displayTempInServiceAreaWithSymbol(float temp, byte *symbolToDraw)
{
  createCustomLcdChar(4, symbolToDraw);
  lcd.setCursor(10, 1);
  lcd.write(4);
  
  char bufferCharConversion[10];               //temporarily holds data from vals
  if(isTemperatureCorrectMeasurment(temp))
  {
    dtostrf(temp, 2, 1, bufferCharConversion);  //float value lastTemp_global is copied onto buff bufferCharConversion
    displayPrintAt(bufferCharConversion, 11, 1); // prints temp on display
    lcd.write(3);//Degree Symbol°C
  }
  else
    displayPrintAt(E(" --- "), 11, 1); // prints
}

void displayServiceLine(String message) {return displayServiceLine(message.c_str());}
void displayServiceLine(char const* message)
{
  String eraseString = "";
  uint8_t messageLength = strlen(message);
  while(messageLength < LCD_DISPLAY_COLS_COUNT)
  {
    eraseString += E(" ");
    messageLength++;
  }

  displayPrintAt((String)message + eraseString, 0, 0);
}


void displayServiceMessage(String message) {return displayServiceMessage(message.c_str());}
void displayServiceMessage(char const* message)
{
  //if no display
  if(!lcdDisplayConnected_global && !oledDisplayConnected_global)
  {
    if(DISPLAY_DEBUG) DEBUG_OUTPUT.println(sE("DisplayServiceMessage: ") + message);
    RemoteDebug.handle();
    return; 
  }
  displayServiceMessageWithSymbol(message, (isWifiConnected()? wifiConnectedSymbol : wifiNOTConnectedSymbol), SYMBOL_ANIMATION_OFF, ERASE_SERVICE_AREA_AND_PIPE_TEMP_AREA); 
}


void displayServiceMessageWithSymbol(String message, byte *symbolToDraw, bool isSymbolAnimated, bool erasePipeTempArea){return displayServiceMessageWithSymbol(message.c_str(), symbolToDraw, isSymbolAnimated, erasePipeTempArea);}
void displayServiceMessageWithSymbol(char const* message, byte *symbolToDraw, bool isSymbolAnimated, bool erasePipeTempArea)
{
  if(!isSymbolAnimated)
    stopActiveAnimation();

  createCustomLcdChar(0, symbolToDraw);

  String eraseString = "";
  uint8_t messageLength = strlen(message);
  while(messageLength < (LCD_DISPLAY_COLS_COUNT-(erasePipeTempArea? 1 : 7)))
  {
    eraseString += E(" ");
    messageLength++;
  }

  displayPrintAt((String)message + eraseString, serviceAreaDisplayStartCol, serviceAreaDisplayStartRow);
}


/////////////////////////////////////////////////////
//////////////Internal functions/////////////////////
/////////////////////////////////////////////////////
void displayPrintAt(String message, int col, int row) {return displayPrintAt(message.c_str(), col, row);}
void displayPrintAt(char const* message, int col, int row)
{
  if(lcdDisplayConnected_global)
    lcd.setCursor(col, row);

  // if(oledDisplayConnected_global)
  //   oledDisplay.setCursor(col * 6, row * 8);

  displayPrint(message);
}


void displayPrint(String message) {return displayPrint(message.c_str());}
void displayPrint(char const* message)
{
  if(lcdDisplayConnected_global)
  {
    if(strlen(message) > LCD_DISPLAY_COLS_COUNT)
    {
      char shortendedMessageCharArray [LCD_DISPLAY_COLS_COUNT+1];
      strncpy(shortendedMessageCharArray, message, LCD_DISPLAY_COLS_COUNT);
      // now null terminate
      shortendedMessageCharArray[LCD_DISPLAY_COLS_COUNT] = '\0';
      lcd.print(shortendedMessageCharArray);
    }
    else
      lcd.print(message);
  }

  // if(oledDisplayConnected_global)
  // {
  //   oledDisplay.print(message);
  //   oledDisplay.display();
  // }
}

/////////////////////////////////////////////////////
////////////// ANIMATION functions //////////////////
/////////////////////////////////////////////////////
byte* setActiveAnimation(byte symbol[][8], uint16_t animationStepDuration, uint8_t animationFramesCount)
{
  if(DISPLAY_ANIMATIONS_ENABLED)
  {
    // Serial.println(sE("Setting Animation on! frames: ") + animationFramesCount);
    GLOBAL.ANIMATION.activeSymbol = symbol;
    GLOBAL.ANIMATION.progress = 0;
    GLOBAL.ANIMATION.stepDuration = animationStepDuration;
    GLOBAL.ANIMATION.framesCount = animationFramesCount;
    !tasker.setTimeout(animateActiveSymbol_taskerLoop, GLOBAL.ANIMATION.stepDuration, TASKER_SKIP_NEVER);
  }
  // else
  //   Serial.println("Animations are turned Off!");

  return symbol[0];
}

uint8_t animateWiFiProgressSymbol(uint8_t progressAnimationCounter)
{
  DEBUG_OUTPUT.print(E("."));
  createCustomLcdChar(0, wifiSendingSymbolAnimation[progressAnimationCounter]);
  if(++progressAnimationCounter < 4)//Array length
    return progressAnimationCounter;
  else
    return 0;
}

void animateActiveSymbol_taskerLoop(int)
{
  if(GLOBAL.ANIMATION.progress != -1)
  {
    // Serial.println(sE("Animating next step: ") + GLOBAL.ANIMATION.progress);
    createCustomLcdChar(0, GLOBAL.ANIMATION.activeSymbol[incrementAnimationCount(GLOBAL.ANIMATION.framesCount)]);
    if(DISPLAY_ANIMATIONS_ENABLED) 
      tasker.setTimeout(animateActiveSymbol_taskerLoop, GLOBAL.ANIMATION.stepDuration, TASKER_SKIP_NEVER);
  }
  // else
  // Serial.println(sE("Animation is stopped: ") + GLOBAL.ANIMATION.progress);
}

void stopActiveAnimation()
{
  GLOBAL.ANIMATION.progress = -1;
}


uint8_t incrementAnimationCount(const uint8_t animationFramesCount)
{
  // Serial.println(sE("Incrementing Animation from: ") + GLOBAL.ANIMATION.progress);
  return GLOBAL.ANIMATION.progress = (++GLOBAL.ANIMATION.progress >= animationFramesCount) ? 0 : GLOBAL.ANIMATION.progress;
}




// void eraseServiceDisplayArea()
// {
//   const char* message = "         ";      
  
//   if(lcdDisplayConnected_global)
//   {
//     lcd.setCursor(serviceAreaDisplayStartCol, serviceAreaDisplayStartRow);
//     lcd.print(message);
//   }

//   if(oledDisplayConnected_global)
//   {
//     oledDisplay.setCursor(serviceAreaDisplayStartCol * 6, serviceAreaDisplayStartRow * 8);
//     oledDisplay.print(message);
//     oledDisplay.display();
//   }
// }

