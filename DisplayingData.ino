//OLED_Display
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

bool lcdDisplayConnected_global = false;
bool oledDisplayConnected_global = false;

//////// Animation definitions ////////
#define SYMBOL_ANIMATION_ON true
#define SYMBOL_ANIMATION_OFF false
byte * setActiveAnimation(byte symbol[][8], uint16_t animationStepDuration, uint8_t animationFramesCount);
//////// //////// //////// //////// ///

void i2cBus_setup()
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + F("F:i2cBus_setup()"));
  // Wire.begin(SCL, SDA); //Wire.begin(int sda, int scl) !!! labels GPIO4 and GPIO5 on EPS8266 are swaped at old esps!!!!
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN); //Wire.begin(int sda, int scl) !!! labels GPIO4 and GPIO5 on EPS8266 are swaped at old esps!!!!

  Wire.beginTransmission(LCD_DISPLAY_ADDRESS);
  if (Wire.endTransmission() == 0)
    lcd_setup();
  else
    if(SHOW_WARNING_DEBUG_MESSAGES) DEBUG_OUTPUT.println(F("!!WARNING: 16X2 LCD display not found!"));

  // Wire.beginTransmission(OLED_DISPLAY_ADDRESS);
  // if (Wire.endTransmission() == 0)
  //   oledDisplay_setup();
  // else
  //   if(SHOW_WARNING_DEBUG_MESSAGES) DEBUG_OUTPUT.println(F("!!WARNING: OLED display not found!"));
 }


void lcd_setup()
{ 
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + F("F:lcd_setup()"));
  lcdDisplayConnected_global = true;
  // initialize the lcd
  lcd.begin();
  lcd.createChar(0, newCharWifiNOTConnected);
  lcd.createChar(5, newCharC);
  lcd.createChar(6, newCharWaterDrop);
  lcd.createChar(7, newCharTemp);

  // Turn on the blacklight and print a message.
  lcd.backlight();
  showServiceMessage(E("Loading..."));
  lcd.setCursor(10, 0);
  lcd.write(6);//drop symbol
  // displayPrint(E("Full"));

  lcd.setCursor(0, 0);
  lcd.write(7);//temp symbol
  lcd.setCursor(0, 1);
  lcd.write(0);//created symbol
  displayData_loop(0);
}


// void oledDisplay_setup()
// {
//   if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + F("F:oledDisplay_setup()"));
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
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug(RESET_SPACE_COUNT_DEBUG) + F("F:displayData_loop()"));  
  
  char bufferCharConversion[10];               //temporarily holds data from vals

  if(lcdDisplayConnected_global || oledDisplayConnected_global)
  {
    if(isWaterFlowingRightNow() || isDebugButtonPressed())
    {
      float waterFlowInLitresPerMinute = (isDebugButtonPressed()) ? 2.5 : getCurentWaterFlowInLitresPerMinute();
      if(waterFlowInLitresPerMinute > 1)
      {
        displayPrintAt(getSpareWaterInLittres()+E("L  "), 11, 0); // prints rest of watter in Littres (100L minus spotřeba)
        if(dislayRotationPosition_global % 3 == 0)
          printServiceMessageWithSymbol((String)dtostrf(waterFlowInLitresPerMinute, 1, 1, bufferCharConversion) + E("l/m"), setActiveAnimation(newCharWaterFlow, 280, 2), SYMBOL_ANIMATION_ON); //first num is mininum width, second is precision
        else if(dislayRotationPosition_global % 3 == 1)
          printServiceMessageWithSymbol((String)getSpareMinutesOfHotWater(waterFlowInLitresPerMinute) + E("minut"), setActiveAnimation(newCharShower, 250, 2 ), SYMBOL_ANIMATION_ON);
        else if(dislayRotationPosition_global % 3 == 2)
          printServiceMessageWithSymbol(getNowTimeString(), newCharTime, SYMBOL_ANIMATION_OFF);
        dislayRotationPosition_global++;
      }
      else
        displayPrintAt(getUpTime()+E("  "), serviceAreaDisplayStartCol, serviceAreaDisplayStartRow);
    }
    else
      displayPrintAt(getUpTime()+E("  "), serviceAreaDisplayStartCol, serviceAreaDisplayStartRow);
  }

  if(lcdDisplayConnected_global)
  {
    if(isTemperatureCorrectMeasurment(lastTemp_global))
    {
      dtostrf(lastTemp_global, 2, 2, bufferCharConversion);  //float value lastTemp_global is copied onto buff bufferCharConversion //first num is mininum width, second is precision
      displayPrintAt(bufferCharConversion, 1, 0); // prints temp on display
      // lcd.print((char)223);//Degree Symbol°
  //    oledDisplay.write(759);//° symbol
      lcd.write(5);//Degree Symbol°
      displayPrint(E("C"));
    }
    else
      displayPrintAt(E(" --- "), 1, 0); // prints
    
    if(isTemperatureCorrectMeasurment(tempSensors[PIPE].temp))
    {
      dtostrf(tempSensors[PIPE].temp, 2, 1, bufferCharConversion);  //float value lastTemp_global is copied onto buff bufferCharConversion
      displayPrintAt(bufferCharConversion, 11, 1); // prints temp on display
      // lcd.print((char)223);//Degree Symbol°
      //oledDisplay.write(759);//° symbol
      lcd.write(5);//Degree Symbol°
      displayPrint(E("C"));


      /////// VOLTAGE MEASURMENT

      // int rawData = analogRead(A0);
      // dtostrf((float)rawData/1023*4.43, 2, 2, bufferCharConversion);  //float value lastTemp_global is copied onto buff bufferCharConversion
      // displayPrintAt(bufferCharConversion, 11, 1); // prints voltage
      // displayPrint("V"); // prints voltage

      // displayPrintAt((String)rawData, 1, 1); // prints voltage
      // displayPrint("/1023"); // prints voltage

    }
    else
      displayPrintAt(" --- ", 11, 1); // prints
  }


  // if(oledDisplayConnected_global)
  // {
  //   //float namerenaTeplotaVBojleru = readTemperature();
  //   if(isTemperatureCorrectMeasurment(lastTemp_global))
  //     if(lastTemp_global)
  //     {
  //       //lcd.write('^');
  //       oledDisplay.write(24);//up arrow symbol
  //     }
  //     else
  //     {
  //       //lcd.write('_');
  //       oledDisplay.write(25);//down arrow symbol
  //     }
  // }
  
  
  // if(oledDisplayConnected_global)
  // {
  //   oledDisplay.setCursor(0, 4 * 8);//4*8 4.řádek * 8bodů font
  //   oledDisplay.print("MainTemp: ");
  //   oledDisplay.print(lastTemp_global);
  //   oledDisplay.println("   ");
  //   oledDisplay.print("FlowTemp: ");
  //   oledDisplay.print(flowTemp_global);
  //   oledDisplay.println("     ");
  //   oledDisplay.print("PipeTemp: ");
  //   oledDisplay.print(tempSensors[PIPE].temp);
  //   oledDisplay.println("                                                  ");

  //   oledDisplay.display();
  // }
}


void showServiceMessage(String message) {return showServiceMessage(message.c_str());}
void showServiceMessage(char const* message)
{
  if(DISPLAY_DEBUG) DEBUG_OUTPUT.print("DisplayServiceMessage: ");
  if(DISPLAY_DEBUG) DEBUG_OUTPUT.println(message);

  if(isWifiConnected())
    printServiceMessageWithSymbol(message, newCharWifiConnected, SYMBOL_ANIMATION_OFF); 
  else
    printServiceMessageWithSymbol(message, newCharWifiNOTConnected, SYMBOL_ANIMATION_OFF);
}


void printServiceMessageWithSymbol(String message, byte *symbolToDraw, bool isSymbolAnimated){return printServiceMessageWithSymbol(message.c_str(), symbolToDraw, isSymbolAnimated);}
void printServiceMessageWithSymbol(char const* message, byte *symbolToDraw, bool isSymbolAnimated)
{
  if(!isSymbolAnimated)
    stopActiveAnimation();

  lcd.createChar(0, symbolToDraw);      

  String eraseString = "";
  uint8_t messageLength = strlen(message);
  while(messageLength < (LCD_DISPLAY_COLS_COUNT-1))
  {
    eraseString += " ";
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
      shortendedMessageCharArray[LCD_DISPLAY_COLS_COUNT+1] = '\0';
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
byte * setActiveAnimation(byte symbol[][8], uint16_t animationStepDuration, uint8_t animationFramesCount)
{
  if(DISPLAY_ANIMATIONS_ENABLED)
  {
    // Serial.println(sE("Setting Animation on! frames: ") + animationFramesCount);
    activeAnimationSymbol_global = symbol;
    animationProgress_global = 0;
    activeAnimationStepDuration_global = animationStepDuration;
    activeAnimationFramesCount_global = animationFramesCount;
    tasker.setTimeout(animateActiveSymbol_taskerLoop, activeAnimationStepDuration_global, TASKER_SKIP_WHEN_NEEDED);
  }
  // else
  //   Serial.println("Animations are turned Off!");

  return symbol[0];
}

uint8_t animateWiFiProgressSymbol(uint8_t progressAnimationCounter)
{
  lcd.createChar(0, newCharWifiSending[progressAnimationCounter]);
  if(++progressAnimationCounter < 4)//Array length
    return progressAnimationCounter;
  else
    return 0;
}

void animateActiveSymbol_taskerLoop(int)
{
  if(animationProgress_global != -1)
  {
    // Serial.println(sE("Animating next step: ") + animationProgress_global);
    lcd.createChar(0, activeAnimationSymbol_global[incrementAnimationCount(activeAnimationFramesCount_global)]);
    if(DISPLAY_ANIMATIONS_ENABLED) tasker.setTimeout(animateActiveSymbol_taskerLoop, activeAnimationStepDuration_global, TASKER_SKIP_WHEN_NEEDED);
  }
  // else
  // Serial.println(sE("Animation is stopped: ") + animationProgress_global);
}

void stopActiveAnimation()
{
  animationProgress_global = -1;
}


uint8_t incrementAnimationCount(const uint8_t animationFramesCount)
{
  // Serial.println(sE("Incrementing Animation from: ") + animationProgress_global);
  return animationProgress_global = (++animationProgress_global >= animationFramesCount) ? 0 : animationProgress_global;
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

