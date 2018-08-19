#define NEWEST_CODE_MODIFICATIONS "heap aptimization"

/*Freeheap during loops: loop(34640) 38488 - at start
RESET FLASH: 
esptool.py --port $(ls  /dev/cu.* | grep -v 'Blue') erase_flash
Upload EMPTY BIN
esptool.py --port $(ls  /dev/cu.* | grep -v 'Blue') write_flash 0x000000 /Users/jindra/Documents/Platform.IO\ Projects/_Empty\ sketch\ -\ erase\ flash/blank4mb.bin
Upload AT COMMANDS
esptool.py --port /dev/cu.SLAB_USBtoUART write_flash 0x000000 /Users/jindra/Downloads/AT_bin/512+512/user1.1024.new.2.bin
DOWNLOAD BINARY 
esptool.py --port $(ls  /dev/cu.* | grep -v 'Blue') --baud 115200 read_flash 0x00000 0x400000 backup.img
ls /dev/tty.* | ls /dev/cu.* | ls  /dev/cu.* | grep -v 'Blue'
//brew upgrade  platformio
//pip install -U https://github.com/platformio/platformio-core/archive/develop.zip //Netestované

//MANUAL
//Po výměně tepelného čidla je potřeba udělat plný reset
//PLNÝ RESET - POSTUP
//PO restartu ihned zmáčknout a držet FLASH tlačítko, dokud se nerozbliká ledka a pro potvrzení restartu zmáčknout ještě jednou FLASH
//1 - DEL SENSOR ADDR - Smaže EEPROM (uložené adresy teplotních čidel)
//2 - FORMAT SPIFFS - Smaže vešerkou SPIFF paměť
//3 - RESET WIFI SETTINGS

/*
  Created by Jindrich SIRUCEK
  See www.jindrichsirucek.cz
  Email: jindrich.sirucek@gmail.com
*/

/*Platformio Settings
  [env:esp12e]
  platform = espressif8266
  board = esp12e
  framework = arduino
  upload_speed = 921600
  upload_speed = 460800
  #build_flags = -Wl,-Tesp8266.flash.4m.ld
  board_build.f_flash = 20000000L #flesh speed 

  board_build.flash_mode = dout #http://docs.platformio.org/en/latest/platforms/espressif8266.html
*/

//Libraries included
#include "define.h"

#include "FS.h"
#include "RemoteDebug/RemoteDebug.h" //700B of RAM
#include "TaskerModified.h"
#include <ESP8266WiFi.h>
#include <OneWire.h> //its here because of using typeDefs in different tabs
#include <DallasTemperature.h>
#include <EspSaveCrash.h> //changed offset and basic size of buffer
#include <Ticker.h>

  
//------------------ PIN DEFINITIONS ----------------------------

// A0 - A0   |   A0 - IN  -  Current sensor
//  0 - D3   |   D0 - 16  -  Electricity sensor
//  1 - D10  |   D1 - 5   -  I2C SDA
//  2 - D4   |   D2 - 4   -  I2C SCLř
//  3 - D9   |   D3 - 0   -  IN - FLASH button
//  4 - D2   |   D4 - 2   -  LED pin
//  5 - D1   |   D5 - 14  -  
//  9 - D11  |   D6 - 12  -  IN - WaterFlow sensor
// 10 - D12  |   D7 - 13  -  OneWire
// 16 - D0   |   D8 - 15  -  SSR
// 13 - D7   |   D9 - 3   -  RXD / Heating Relay control
// 14 - D5   |   D10 - 1  -  TXD 
// 15 - D8   |   D11 - 9  -  NOT ACCESIBLE
// 16 - D0   |   D12 - 10 -  NOT ACCESIBLE

//GPIO 0 and 2,can be used only in special cases viz: http://www.instructables.com/id/ESP8266-Using-GPIO0-GPIO2-as-inputs/
//!!GPIO 16 is left next to ch_pd pin, uncorectly signed as GPIO15 on universal white board!!!
//GPIO 15 have hardware input pulldown - and MUST keep LOW during boot
//GPIO 2 MUST keep during boot HIGH

#define FLASH_BUTTON_PIN 0 //D3
#define TXD_PIN 1
#define LED_PIN 2 // 2-D4
#define RXD_PIN 3
#define I2C_SDA_PIN 4  // 4-D2
#define I2C_SCL_PIN 5  // 5-D1
#define WATER_FLOW_SENSOR_PIN 12 // 12-D6
#define ONE_WIRE_BUS_PIN 13 // 13-D7 // DS18B20 pin
#define CURRENT_SENSOR_PIN A0 // 14-D5
#define HEATING_SWITCH_OFF_RELAY_PIN 15 //D9 = RXD -  15 - not booting!! when used for relay
#define HEATING_SWITCH_ON_SSR_PIN RXD_PIN
#define ELECTRICITY_SENSOR_PIN 16 // 16-D0 it has to be 16 only port with input INPUT_PULLDOWN_16

//----------------------------------------------------------------------
//----------------------------------------------------------------------
////////////////////////////////////////////////////////
//MODULE ENABLING SETTINGS
////////////////////////////////////////////////////////

#define OTA_MODULE_ENABLED !true   //1kB of Heap
#define DISPLAY_MODULE_ENABLED true
#define WATER_FLOW_MODULE_ENABLED true
#define CURRENT_MODULE_ENABLED true
#define TEMP_MODULE_ENABLED true
#define WAITING_FOR_RESPONSES_MODULE_ENABLED true
#define UPLOADING_DATA_MODULE_ENABLED true
#define SENDING_BEACON_MODULE_ENABLED true
#define HEATING_CONTROL_MODULE_ENABLED true
#define DISPLAY_ANIMATIONS_ENABLED true

////////////////////////////////////////////////////////
//DEBUG SETTINGS
////////////////////////////////////////////////////////

#define MAIN_DEBUG true
#define WIFI_DEBUG false
#define INTERNET_COMMUNICATION_DEBUG false
#define YIELD_DEBUG false
#define DATA_LOGGING_DEBUG false
#define DISPLAY_DEBUG false
#define TEMPERATURE_DEBUG false
#define RELAY_DEBUG false
#define CURRENT_DEBUG false
#define WATER_FLOW_DEBUG false
#define SPIFFS_DEBUG false

#define SHOW_ERROR_DEBUG_MESSAGES true
#define SHOW_WARNING_DEBUG_MESSAGES true


////////////////////////////////////////////////////////
//PROJECT SETTINGS
////////////////////////////////////////////////////////
#define DATA_SERVER_HOST_ADDRESS E("script.google.com")
#define DATA_SERVER_SCRIPT_URL E("/macros/s/AKfycbziCOySJ-cxsjn6-v8WpIbcsmlE77RqkzGX728nht2wO4HYmvVK/exec?"); //Testing script, data goes here: https://docs.google.com/spreadsheets/d/1JyISlQ2zlttjRfxDFLuECTkgKGiBDn5A_I6WSVT3l5s/edit#gid=538340115   //Script address: https://script.google.com/macros/d/Magj9VtC_7VJMKhlPxI7rWgcRztdqeR-I/edit?uiv=2&mid=ACjPJvEivmgMleau6F0s_c2EsMfGlndE8W7Ls8gVj1MQ_8lgOg0eUus8N2GJ4NBMyAmUaa3dtUi9fnETPQNdpStDwnuy1LGlfiQFRvEOzq0JVA3E0nz5a-Jfqqk9GdIOuEocXf9pIdvamg

#define SETTINGS_FILENAME E("/lastResponse.json")
#define MODULE_UPDATE_FILE_NAME E("/moduleUpdate.info")

////////////////////////////////////////////////////////
//BASIC DEFINITIONS
////////////////////////////////////////////////////////
#define MIMIMAL_SENDING_TEMP_DIFFERENCE 0.07 // after the difference between two measurments cross this level, data will be uploaded. Lower values bigger acuracy, but values can jump up and down around one value - too many samples with no real value
#define CORRECT_TEMP_SAMPLES_COUNT 3 // number of samples from which count average temperature
#define MAX_TEMPERATURE_SAMPLES_COUNT 7 // number of trials to get correct temperature samples
#define MAX_ACCEPTED_TEMP_SAMPLES_DIFFERENCE 10 //Maximal accepted temp difference between two measurments, in degrees of celsius // eliminates wrong measurments

#define ERROR_TEMP_VALUE_MINUS_127 -127
#define ERROR_TEMP_VALUE_PLUS_85 85
#define BOILER_SIZE_LITRES 100

#define EEPROM_ALOCATION_SIZE 512
#define CURRENT_TRESHOLD_VALUE 8 //value from above is took like there is heating going
////////////////////////////////////////////////////////
//SAFETY MAX and MIN SETTINGS
////////////////////////////////////////////////////////
#define MAXIM_FILE_LOG_SIZE maxSizeToSend_global
#define OSWATCH_RESET_TIME 2*MIN //minutesa

#define SPIFFS_SINGLETON true

////////////////////////////////////////////////////////
//DEVELOPMENT SETTINGS
////////////////////////////////////////////////////////
#define DOMA_MODULE "" //Comment when working on home 
// #define TEST_MODULE "" //Comment when working on home 
// #define PRODUCTION_MODULE "" //Comment when working on home 

#ifdef TEST_MODULE
  #define NODE_NAME "Testing"
  #define DISPLAY_LOG_MESSAGES !false
  #define QUICK_DEVELOPMENT_BOOT false
  #define WIFI_RADIO_OFF false
  // #define UPLOADING_DATA_MODULE_ENABLED false

  // #define DEBUG_OUTPUT Serial
  #define DEBUG_OUTPUT RemoteDebug
#endif

#ifdef DOMA_MODULE
  #define NODE_NAME "Doma"
  #define WIFI_RADIO_OFF false
  #define DISPLAY_LOG_MESSAGES true
  #define QUICK_DEVELOPMENT_BOOT false
  #define DEBUG_OUTPUT RemoteDebug
  #undef SPIFFS_DEBUG
  #define SPIFFS_DEBUG false
  #undef OTA_MODULE_ENABLED
  #define OTA_MODULE_ENABLED true
#endif

#ifdef PRODUCTION_MODULE
  #define NODE_NAME "Daniel"
  #define WIFI_RADIO_OFF false
  #define DISPLAY_LOG_MESSAGES true
  #define QUICK_DEVELOPMENT_BOOT false
  #define DEBUG_OUTPUT RemoteDebug
  #undef OTA_MODULE_ENABLED
  #define OTA_MODULE_ENABLED false
  #undef SPIFFS_DEBUG
  #define SPIFFS_DEBUG false
#endif

#define ERROR_OUTPUT DEBUG_OUTPUT
#define COMPILATION_DATE sE(__TIME__) + E(" ")+ E(__DATE__)
////////////////////////////////////////////////////////
//  STRUCTS DEFINITIONS
////////////////////////////////////////////////////////
enum {
  BOJLER = 0,
  PIPE,
  ROOM_TEMP,
  INSIDE_FLOW,
  MAX_TEMP_SENSORS_COUNT
};

enum {
  BOILER_CONTROL_OFF = 0,
  BOILER_CONTROL_PROGRAMATIC,
  BOILER_CONTROL_MANUAL
};


struct EepromSettingsStruct 
{
  struct TempSensorAddressesStruct 
  {
    DeviceAddress bojler;
    DeviceAddress pipe;
    DeviceAddress roomTemp;
    DeviceAddress insideFlow;
  } tempSensorAddresses;
  
  struct {
    IPAddress ipAddress;
    IPAddress maskAddress;
    IPAddress gatewayAddress;
    char ssid[40];
    bool initialized = false;
    } wifi;

  float currentLinearizationCoeficient = 1.0986;
  float currentBurdenResistorValue = 77.5;
};


struct TempSensorStruct {
  DeviceAddress address;
  bool sensorConnected = false;
  float temp = ERROR_TEMP_VALUE_MINUS_127;
  float newTempSamples[CORRECT_TEMP_SAMPLES_COUNT];
  uint8_t correctTempSampleIndex = 0;
  float lastTemp = ERROR_TEMP_VALUE_MINUS_127;
  // uint8_t resolution = 9;
  const char *sensorName;
};


struct GlobalStruct {
  struct TempGenericStruct 
  {
    TempSensorStruct sensors[MAX_TEMP_SENSORS_COUNT];
    uint8_t topHeating = 45;
    uint8_t lowDroping = 44;
    uint8_t lastHeated = 0;
    uint8_t boilerControlStyle = BOILER_CONTROL_PROGRAMATIC; 
    bool heatingState = true;
    uint8_t asyncMeasurmentCounter = MAX_TEMPERATURE_SAMPLES_COUNT;
    } TEMP;

  struct AnimationGenericStruct 
  {
    byte (*activeSymbol)[8]; //Symbol animation array reference
    int8_t progress = -1;
    uint8_t framesCount;
    uint16_t stepDuration;
  } ANIMATION;

  size_t nodeStatusUpdateTime = 1*HOUR; //60 * 60 * 1000 = 1 hour
  bool connectToLastRememberedWifi = true;

  uint16_t lastFreeHeap = 0;
  bool nodeInBootSequence = true;
  uint8_t spaceCountDebug = 0; 
} GLOBAL;

////////////////////////////////////////////////////////
//  GLOBAL VARIABLES
////////////////////////////////////////////////////////
// Tasker tasker(ERROR_OUTPUT, DEBUG_OUTPUT); //Error fisrt, normal debug second
RemoteDebug RemoteDebug;
Tasker tasker(ERROR_OUTPUT, DEBUG_OUTPUT); //Error fisrt, normal debug second
//SW Loop watchdog
Ticker tickerOSWatch;

String lastCurrentMeasurmentText_global = "";
bool lastElectricCurrentState_global = false;
bool lastElectricityConnectedState_global = false;

uint32_t lastWaterFlowSensorCount_global = 0;
volatile uint16_t waterFlowSensorCount_ISR_global;  // Measures flow meter pulses
int32_t waterFlowDisplay_global = 0;
bool isWaterFlowingLastStateFlag_global = false; //water flowing flag

// String objectAskingForResponse_global = "";

bool isThereEventToSend_global = false;
uint16_t logFileRowsCount_global = 0;
uint16_t curentLogNumber_global = 0;
uint32_t maxSizeToSend_global = 50000;
String lastNodeStateTempString_global = "";

uint8_t displayRotationPosition_global = 0;

////////////////////////////////////////////////////////
//DEBUGING VARIABLES and SETTINGS
////////////////////////////////////////////////////////
static size_t lastWdtLoopTime_global;

uint16_t notParsedHttpResponses_errorCount = 0;
uint16_t parsedHttpResponses_notErrorCount = 0;
uint16_t totalErrorCount_global = 0;


uint8_t getDebuggerSpacesCount() {return GLOBAL.spaceCountDebug;}
#define RESET_SPACE_COUNT_DEBUG 0

// ADC_MODE(ADC_VCC); //Battery measurment throw inner system, cant be used when you want Analog values

////////////////////////////////////////////////////////
//  FUNCTION DEFINITIONS
////////////////////////////////////////////////////////
#define isDebugButtonPressed() isFlashButtonPressed()
#define isFlashButtonPressed() (digitalRead(FLASH_BUTTON_PIN) == 0)
#define isElectricityConnected() digitalRead(ELECTRICITY_SENSOR_PIN)
void ICACHE_RAM_ATTR osWatch();
void ICACHE_RAM_ATTR ISR_flowCount();//Preprocesor
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//Program itself - initialization
void setup()
{
  Serial.begin(115200);
  Serial.println(sE("\n\nStarted Serial, free ram: ") + (GLOBAL.lastFreeHeap = ESP.getFreeHeap()));
  ////////////////////////
  // LOOP WATCHDOG
  ////////////////////////
  lastWdtLoopTime_global = millis();
  //Loop watchdog attaching interrupt
  tickerOSWatch.attach_ms((OSWATCH_RESET_TIME), osWatch);
  
  ////////////////////////
  // BASIC MODULES SETUP 
  ////////////////////////

  lastNodeStateTempString_global.reserve(200);
  
  if(DISPLAY_MODULE_ENABLED) 
    i2cBus_setup();//+Display setup
  
  runServiceMenuIfNeeded();
  
  SPIFFS_setup();
  printBegginGraphics();
  remoteDebug_setup();

  printSettingsStructSavedInEeeprom();
  printResetInfo();
  blinkNotificationLed(SHORT_BLINK); //Blink to show modulle is in order working
  ////////////////////////
  // BUSES & SENSORS & CONTROLS INITIALIZATION
  ////////////////////////
  if(TEMP_MODULE_ENABLED)            oneWireBus_setup();  
  if(TEMP_MODULE_ENABLED)            temperature_setup();
  if(WATER_FLOW_MODULE_ENABLED)      waterFlowSensor_setup();
  if(CURRENT_MODULE_ENABLED)         currentAndElectricity_setup();
  if(HEATING_CONTROL_MODULE_ENABLED) relayBoard_setup();
  
  blinkNotificationLed(SHORT_BLINK); //Blink to show modulle is in order working
  ////////////////////////
  // WIFI, DEBUG, TELNET
  ////////////////////////
  if(QUICK_DEVELOPMENT_BOOT == false)
    if(autoWifiConnect())
      logNewNodeState(sE("WiFi: connected to: ") + WiFi.SSID() + E(" (")+ WiFi.RSSI()+ E("dBm)"));

  blinkNotificationLed(SHORT_BLINK); //Blink to show modulle is in order working    
  ////////////////////////
  // BOOTING SEQUENCE
  ////////////////////////
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug(RESET_SPACE_COUNT_DEBUG) + E("F:MAIN_setup()"));
  DEBUG_OUTPUT.println(sE("Initializing node ") + E(NODE_NAME));

  RemoteDebug.handle();
  
  if(isWifiConnected() && QUICK_DEVELOPMENT_BOOT == false)
  {//With internet connection
    displayServiceLine(cE("Init: Server"));
    sendDebugInformationsAfterReset();
    RemoteDebug.handle();

    if(sendAllLogFiles())
      doNecesaryActionsUponResponse();

    //Automatic check new FW Update
    if(QUICK_DEVELOPMENT_BOOT == false)
      checkUpdatesFromHttpServer();
  }
  else
  {//Without internet connection
    displayServiceLine(cE("Init: Offline"));
    if(isLastSavedServerResponseOk())
    {
      logNewNodeState(E("settings file loaded from EEPROM"));
      doNecesaryActionsUponResponse();
    }
    else
      logNewNodeState(E("settings file NOT loaded from EEPROM"));

  }

  blinkNotificationLed(SHORT_BLINK); //Blink to show modulle is in order working

  if(!isTemperatureCorrectMeasurment(GLOBAL.TEMP.lastHeated))
    setLastHeatedTemp(GLOBAL.TEMP.sensors[BOJLER].temp);
  
  RemoteDebug.handle();
  
  yield_debug();

  if(OTA_MODULE_ENABLED) OTA_setup();
  if(DISPLAY_MODULE_ENABLED) lcd_setup();
  
  GLOBAL.nodeInBootSequence = false;
}


//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

void loop()
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(E("\nF:MAIN_loop()"));
  //---------------------------------- NON-SKIPABLE TASKS ------------------------------------------
  if(true)                          tasker.setInterval(feedWatchDog_taskerLoop, 0.5*SEC, TASKER_SKIP_NEVER);
  if(UPLOADING_DATA_MODULE_ENABLED) tasker.setInterval(uploadLogFile_loop, 0*MIN + 10*SEC, TASKER_SKIP_NEVER);
  if(OTA_MODULE_ENABLED)            tasker.setInterval(silentOTA_loop,  500*MSEC, TASKER_SKIP_NEVER);
  if(DISPLAY_MODULE_ENABLED)        tasker.setInterval(displayData_loop, 1*SEC, TASKER_SKIP_NEVER);
  if(TEMP_MODULE_ENABLED)           tasker.setInterval(temperature_loop, 2*SEC, TASKER_SKIP_NEVER);
  //---------------------------------- SKIPABLE TASKS ----------------------------------------------
  if(WATER_FLOW_MODULE_ENABLED)     tasker.setInterval(waterFlow_loop, 20*SEC, TASKER_SKIP_WHEN_NEEDED);
  if(CURRENT_MODULE_ENABLED)        tasker.setInterval(current_electricity_loop, 5*SEC, TASKER_SKIP_WHEN_NEEDED);
  //---------------------------------- ONE TIME TASKS --------------------------------------------------
  if(SENDING_BEACON_MODULE_ENABLED) tasker.setTimeout(checkSystemState_loop,  GLOBAL.nodeStatusUpdateTime, TASKER_SKIP_NEVER); //60 * 60 * 1000 = 1 hour
  //---------------------------------- OUTRO TASK --------------------------------------------------
  if(true)                          tasker.setOutroTask(outroTask_loop);

  DEBUG_OUTPUT.printf(cE("FreeHeap:%d\n"), ESP.getFreeHeap());
  GLOBAL.lastFreeHeap = ESP.getFreeHeap();
  tasker.run();
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
void outroTask_loop()
{
  // Serial.println((String)"ELECTRICITY_SENSOR_PIN pin: " + digitalRead(ELECTRICITY_SENSOR_PIN));
  yield_debug();
  RemoteDebug.handle();

  //Log warning messages
  if(RemoteDebug.isThereWarningMessage())
  {
    if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug(RESET_SPACE_COUNT_DEBUG) + E("F:outroTask_loop(): Logging error messages"));
    File errorLogFile = RemoteDebug.getErrorFile();
    uint8_t maxWarnings = 10;
    uint8_t curentDebuggerLevel = getDebuggerSpacesCount();
    while(maxWarnings-- && errorLogFile.available())
    {
      getUpTimeDebug(curentDebuggerLevel);
      String line = errorLogFile.readStringUntil('\r');
      // Serial.println(sE("Error line: ") + line);
      logWarningMessage(line);
    }
    errorLogFile.close();
    RemoteDebug.removeErrorLogFile();
    
    if(maxWarnings == 0)
     logWarningMessage(E("Too many warning messages")); //TODO_ ERROR MESega sshould send, this is herror not warning
  }

  //Check if there is some error message after last function called
  if(RemoteDebug.isThereErrorMessage())
  {
    if(MAIN_DEBUG) DEBUG_OUTPUT.println();
    String uri = E("&quickEvent=");
    uri += URLEncode(E("Errors"));
    uploadDebugLogFileWithGetParams(uri);
  }

  //Debug button
  if(isDebugButtonPressed())
  {
    uint8_t number = 100;
    while(number--)
      ISR_flowCount();

    logNewNodeState(E("TEST: Water started"));
  }


  //Free heap check // if changed more the 2kB
  if((ESP.getFreeHeap()+2000) < GLOBAL.lastFreeHeap)
  {
    logWarningMessage(sE("!!!Error: Freeheap loosing"), sE("previous: ") + GLOBAL.lastFreeHeap + E(" current: ") + ESP.getFreeHeap());
    GLOBAL.lastFreeHeap = ESP.getFreeHeap();
  }

  if(GLOBAL.lastFreeHeap++ == 10)//First tasks are skipped
    GLOBAL.lastFreeHeap = ESP.getFreeHeap();
}


void uploadLogFile_loop(int)
{
  //Dont upload over wifi if water is flowing (only save new states, sends it later)
  if(isWaterFlowingLastStateFlag_global == true && isFlashButtonPressed() == false)
    return;

  //Turn wifi OFF if there is no electricity (on battery)
  if(isElectricityConnected() == false && WIFI_RADIO_OFF == true && isFlashButtonPressed() == false)
  {
    if(wifi_get_opmode() != NULL_MODE)
      turnWifiOff();
  
    if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug(RESET_SPACE_COUNT_DEBUG) + E("F:uploadLogFile_loop(int): Disabled - Battery saving"));
    return;
  }

  //If wifi in sleep mode - radiof OFF - turn it on before uploading
  if(wifi_get_opmode() == NULL_MODE)
    turnWifiOn();

  //If wifi not connected - connects
  autoWifiConnect();
  
  if(OTA_MODULE_ENABLED) 
    OTA_begin();

  if(MAIN_DEBUG) DEBUG_OUTPUT.print(getUpTimeDebug(RESET_SPACE_COUNT_DEBUG) + E("F:uploadLogFile_loop(int)"));

  if(isThereAnyPendingEventsToSend())
  {
    if(MAIN_DEBUG) DEBUG_OUTPUT.println();
    
    bool success = sendAllLogFiles();
    if(success)
      doNecesaryActionsUponResponse();
  }
  else if(MAIN_DEBUG) DEBUG_OUTPUT.println(E(" - Nothing to send!"));
}

bool isThereAnyPendingEventsToSend()
{
  return isThereEventToSend_global;
}


void setPendingEventToSend(bool isThereEventToSend)
{
  // if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:setPendingEventToSend(): ") + ((isThereEventToSend) ? E("true") : E("false")));
  isThereEventToSend_global = isThereEventToSend;
}



// HOTOVO? přichozí timestamp uložit do notes a když bude rozdíl velký tak podbarvit červeně - aby to šlo vidět
// Když přijde nová hodnota, tak zkontrolovat předchozí čas a pokud je větší než minimal node alive time, tak mi pošle email, že tam došlo k zpoždění
//Funkce reset spiffs s parametrem - debug log, nacte nejdriv debugovací infomrace z extra logu, přidá důvot resetu a pak uloží do nového souboru , tak abych nepřišel o olog co se stalo a proč se to stalo..
//Při formátování SPIFFS error sobor zkusit zachránit načtením do Stringu RAMKY ale nastavit max několik řádků at to nespadne na nedostatek ramky, zkontrolovat free heap, podle toho vytáhnout jen tolik bytu z souboru, porovnat velikost souboru atd..
//vyřešit extra ERROR loging!! to se nesmí ztrácet, nedoručené packety odpovědi apod.. to chci vše zapisovat do logu
//NodeName - nastavovat v Debug sheetu ne pevně v kodu!!
//Porestartu modulu zkontrolovat nejdříve zda kolem není připojená "servisní síť" ESP-sevice - pokud ano, tak se připojí na ní a hodí se do konfiguaračního režimu, zapne se server, který bude přijímat webové požadavky přes get params a později vytvoří normálně servisí webové rozhraní
//Logovat vsechny warningy jako nlze se připojit k wifi, nebo teplotní vzorek nebyl dobře načte a errory jako není připjen teplotní senzro apod
//Inteligentní nastavení startu času ohřevu vody v noci, tak aby se ohřev ukončil právě v okamžik kdy dojde k vypnutí NT, aby nebyl bojler nahřátej a pak ještě 2 hodiny nejel NT a bojler nám mezitím chládnul (zbytečné ztráty)
//Hodnotu koeficientu u ampérměřáku uložit do eeprom a udělat možnost tuto hotnotu měnit v google sheetu namísto v kodu
//SPeed up upload preocess https://github.com/esp8266/Arduino/issues/1853
//Write/read test of SPIFF in each cycle
//v remote debug - předělat telnet pouze na přání, vyhodit ho z těch funckí aby se server nestartoval automaticky
//Init temp: Sensors: 4x (3/2/1/0), nebo OK OK OK OK nebo teploty jednotlivých
//Vyřešit warning messages - aby se posílali správně jako eventy
//Check first if tehre is a new update file - in response send new md5 checksum, date of file (datum nového souboru poté poslat do logu) and maybe version or smth, když bude soubor tak stáhnout v druhém volání, zase tak často se neupdatuje aby se to nemohlo udělat na 2x
//Vyřešit logování Pipe temp: rised aby to logovalo rised (water flow) jenom jednou do té doby, než začne zase klesat, pak zaloguje pipe temp down a může zase zalogovat rise.. dal bych tam rozdíl mezi měřeníma asi jen jeden stupeń jakmile povyrost/sníže.. Pokud se teplota sníží o 2 stupně oproti lastpipeTemp, tak..
//Uptime na display zobrazovat 0d15h21m
//Logovat jednotlivé md5 na serveru pro udpdate, složky s názvem md5 a datum kvůli analýze případných exceptions
//V realay controle, když nastane chaby neposílat mail hned, ale vložit pouze Emialovou chybu logFatalErrorState()
//Udělat událostní funkce: onElectricityOn(), on electricityOff(), current, water atd...
//Prozkoumat odesílací proces přes https redirect, kdy přesně dojde k odeslání dat/souboru a udělat možnost, že se data pouze odešlou a nebude se čekat na odpvoěd u serveru - např u posílání  chybvových zpráv
//Vyřešit chybu s loosing heap druing loops
//Udělat měření zda teče voda tak, že se hodí do ISR funkce global flag, případně se porovná poslední ISR s aktuálním ISR aby se vědělo zda od posledně tekla voda, udělat to přes flag, protože se pak ten flag může použít jako identifikátor události onWaterStarted/Stoped
//Vyřešit klidový odběr 2,231A a celkově odběr proudu,a by to bylo přesně, když to je v krabičce nebo prostě v reálném stavu
//Udělat měření napětí na baterii - přes střídání připjeného měřidla ADC current a napětí..
//Pipe temp rise propojit s nárustem za čas - tím se vyřeší aby se nelogovalo postupné zvyšování teploty tím že voda stojí v potrubí a ohřívá se vzducehm, průtok vody to ohřeje vždy skokově
//V google aps scriptu udělat aby se aktualizace v události v mém soukromém kalendáři promítly do už vytvořené kopie v temp kalendáři, např když smažu u sebe událost +10°C aby se to smazalo i z temp kalendáře
//Udělat přerušení, které zkontroluje, že pokud se systém není schopnej sám nabootovat do 5ti min do loop část, tak to zformatuje spiffs - self recover
//SPIFFS let the log file open and only FLUSH() Test power loose after flush, if its saved really fhttps://github.com/pellepl/spiffs/wiki/Performance-and-Optimizing
//Logování chyb a warningů udělat váhradně přes logovací funkce, které budou hlídat opakování chyb - aby neposílaly pořád tu stejnou chybu, když se vyskytuje opakovaně
//Zrušit ty vykřičníky z remoteDebugu
//Chyby a warningy nedjřív ukládat do mezistavu, ale odesílat až v normálním rutinním kolečku - aby se nestalo, že se node snaží odeslat chybu hluboko zanořenej ve stakcku  předchozího volání (kvůli ramce a stabilitě systému)
//RemoteDebug.printf_P(PSTR("adfsdf"),1);
//Current coeficient se načítá z příslušné google tabulky a ukládá se do eeprom, stejná metoda jako změna adres tepltních čidel




void temperature_loop(int)
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug(RESET_SPACE_COUNT_DEBUG) + E("F:temperature_loop()"));
  
  //It Meausures temps and if not All temp samples are measured yet it returns
  if(readTemperaturesAsync() == false)
    return;
  
    // if(TEMPERATURE_DEBUG) DEBUG_OUTPUT.println(sE("isTemperatureCorrectMeasurment(GLOBAL.TEMP.sensors[PIPE].temp): ") + isTemperatureCorrectMeasurment(GLOBAL.TEMP.sensors[PIPE].temp));
    // if(TEMPERATURE_DEBUG) DEBUG_OUTPUT.println(sE("isTemperatureCorrectMeasurment(GLOBAL.TEMP.sensors[PIPE].lastTemp): ") + isTemperatureCorrectMeasurment(GLOBAL.TEMP.sensors[PIPE].lastTemp));
  
  if(isTemperatureCorrectMeasurment(GLOBAL.TEMP.sensors[PIPE].temp) && isTemperatureCorrectMeasurment(GLOBAL.TEMP.sensors[PIPE].lastTemp))
  {
    if(TEMPERATURE_DEBUG) DEBUG_OUTPUT.println(sE("checking difference: ") + (GLOBAL.TEMP.sensors[PIPE].temp - GLOBAL.TEMP.sensors[PIPE].lastTemp));

    //Check minimal °C difference on pipe
    float pipeTempDifference = abs(GLOBAL.TEMP.sensors[PIPE].temp - GLOBAL.TEMP.sensors[PIPE].lastTemp);
    {
      static bool isPipeTempRising = false;
      //Temp is rising
      if(GLOBAL.TEMP.sensors[PIPE].temp > GLOBAL.TEMP.sensors[PIPE].lastTemp)
      {
        if(isPipeTempRising == false  && pipeTempDifference >= 5)
        {
          logNewNodeState(E("Pipe Temp: rised"));
          isPipeTempRising = true;
          GLOBAL.TEMP.sensors[PIPE].lastTemp = GLOBAL.TEMP.sensors[PIPE].temp;
        }
        //its here to avoid logging false events, due to temp changes in room temp
        if(isPipeTempRising == true)
          GLOBAL.TEMP.sensors[PIPE].lastTemp = GLOBAL.TEMP.sensors[PIPE].temp;
      }
      else//Pipe temp is falling
      {
        if(isPipeTempRising == true && pipeTempDifference >= 0.1)
        {
          logNewNodeState(E("Pipe Temp: falling"));
          isPipeTempRising = false;
          GLOBAL.TEMP.sensors[PIPE].lastTemp = GLOBAL.TEMP.sensors[PIPE].temp;
        }
        //its here to avoid logging false events, due to temp changes in room temp
        if(isPipeTempRising == false)
          GLOBAL.TEMP.sensors[PIPE].lastTemp = GLOBAL.TEMP.sensors[PIPE].temp;
      }      
    }
  }
  
  if(isTemperatureCorrectMeasurment(GLOBAL.TEMP.sensors[BOJLER].temp) == false)
  {
    totalErrorCount_global++;
    GLOBAL.TEMP.sensors[BOJLER].lastTemp = ERROR_TEMP_VALUE_MINUS_127;
    return;
  }

  if(isTemperatureCorrectMeasurment(GLOBAL.TEMP.sensors[BOJLER].lastTemp) == false)
  {
    GLOBAL.TEMP.sensors[BOJLER].lastTemp = GLOBAL.TEMP.sensors[BOJLER].temp;
    return;//minimálně dvě měření po sobě jdoucí musí být skutečné hodnoty
  }

  if(abs(GLOBAL.TEMP.sensors[BOJLER].temp - GLOBAL.TEMP.sensors[BOJLER].lastTemp) >= MIMIMAL_SENDING_TEMP_DIFFERENCE)
  {
    if(abs((int)GLOBAL.TEMP.sensors[BOJLER].temp - (int)GLOBAL.TEMP.sensors[BOJLER].lastTemp) >= 1) //pokud měření právě překročilo hranici stupně
    logNewNodeState(E("Temp: changed"));
    GLOBAL.TEMP.sensors[BOJLER].lastTemp = GLOBAL.TEMP.sensors[BOJLER].temp;
  }
}


void current_electricity_loop(int)
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug(RESET_SPACE_COUNT_DEBUG) + E("F:current_electricity_loop()"));
  if(CURRENT_DEBUG) DEBUG_OUTPUT.println(sE("Electricity is now: ") + (isElectricityConnected()? E("ON") : E("OFF")));

  bool isThereElectricCurrentNow = isThereElectricCurrent();
  if(isThereElectricCurrentNow == true && isElectricityConnected() == false)
    delay(2000); //waits for power source start

  //Electricity
  if(lastElectricityConnectedState_global != isElectricityConnected()) //if state was Changed
  {
    //OChrana relé před nehchráněným spínáním při naskočení elektřiny
    if(isElectricityConnected() == false)
      setHeatingRelayOpen(SET_HEATING_RELAY_CONNECTED);
    //Electricity ON and heating OFF - vypne relé, protože bylo seplé jen kvůli ochraně relé
    if(isElectricityConnected() == true && isBoilerHeatingOn() == false)
      setHeatingRelayOpen(SET_HEATING_RELAY_DISCONECTED); //Pull relay pin when heating is of and there is electricity


    lastElectricityConnectedState_global = !lastElectricityConnectedState_global;
    logNewNodeState(sE("Electricity: ") + (lastElectricityConnectedState_global? E("ON") : E("OFF")));
    lcd_setup(); // to reset display
  }
  
  controlHeating_loop();

  //Curent
  if(lastElectricCurrentState_global != isThereElectricCurrentNow)
  {
    //Current Off
    if(isThereElectricCurrentNow == false)
    {
      waterFlowDisplay_global = 0; //Vynuluj měření spotřeby teplé vody - bojler je po vypnutí ohřevu celý nahřátý
      setLastHeatedTemp(GLOBAL.TEMP.sensors[BOJLER].lastTemp);
      lcd_setup(); //resets LCD - if some errors
    }

    lastElectricCurrentState_global = isThereElectricCurrentNow;
    logNewNodeState(sE("Current: ") + (isThereElectricCurrentNow? E("rised") : E("dropped")));
  }
}


void waterFlow_loop(int)
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug(RESET_SPACE_COUNT_DEBUG) + E("F:waterFlow_loop()"));

  bool isWaterFlowingRightNowFlag = isWaterFlowingRightNow();
  if(isWaterFlowingLastStateFlag_global == isWaterFlowingRightNowFlag)
    return;//Nothing changed

  if(isWaterFlowingLastStateFlag_global == false && isWaterFlowingRightNowFlag) //watter started to flow
    logNewNodeState(E("Water: started"));
  else//watter stopped flowing
    logNewNodeState(E("Water: stopped"));

  isWaterFlowingLastStateFlag_global = !isWaterFlowingLastStateFlag_global;
}


String getSystemStateInfo()
{
  String systemStateInfo = sE("") +
  E("Up: ") + getUpTime() +
  E("\rfHeap: ") + ESP.getFreeHeap() +
  E("\rheatT: ") + GLOBAL.TEMP.topHeating +
  // E("\rlowTemp: ") + GLOBAL.TEMP.lowDroping +
  E("\rlastHT: ") +GLOBAL.TEMP.lastHeated +
  //E("TimeFromLastUpdate:  ") + getTimeFromLastUpdate() + E("\r");
  // E("\rtotalErrors: ") + totalErrorCount_global +
  E("\rRespOk: ") + parsedHttpResponses_notErrorCount +
  E("\r!OK: ") + notParsedHttpResponses_errorCount +
  // E("\rWaterFlowCount: ") + lastWaterFlowSensorCount_global +
  // E("\rwaterFlowDisplay: ") + waterFlowDisplay_global +
  // E("\rTime&Date: ") + getNowTimeDateString();
  E("\rupdT: ") + formatTimeToString(GLOBAL.nodeStatusUpdateTime) +
  E("\r") + WiFi.RSSI() + E("dBm");
  return systemStateInfo;
}

void checkSystemState_loop(){return checkSystemState_loop(0);}
void checkSystemState_loop(int)
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug(RESET_SPACE_COUNT_DEBUG) + E("F:checkSystemState_loop()"));

  logNewNodeState(E("Node: beacon alive"));//zároveň slouží jako beacon alive

  //bool connectToLastRememberedWifi_global = true; if(SHOW_ERROR_DEBUG_MESSAGES) DEBUG_OUTPUT.println(systemStateInfo);
  //if(SHOW_ERROR_DEBUG_MESSAGES) WiFi.printDiag(Serial);

  if(SENDING_BEACON_MODULE_ENABLED)
    tasker.setTimeout(checkSystemState_loop, GLOBAL.nodeStatusUpdateTime, TASKER_SKIP_NEVER); //60 * 60 * 1000 = 1 hour
}

///////////////////////////////////////////
// Loop watchdog functions
///////////////////////////////////////////
void feedWatchDog_taskerLoop(int)
{
  blinkNotificationLed(SHORT_BLINK); //Blink to show modulle is in order working
  lastWdtLoopTime_global = millis();
}

void ICACHE_RAM_ATTR osWatch(void) 
{
  size_t t = millis();
  size_t last_run = abs(t - lastWdtLoopTime_global);
  if(last_run >= OSWATCH_RESET_TIME)
   {
    digitalWrite(0, HIGH);//When gpio0 used as output, need to by high before reseting ESP
    digitalWrite(HEATING_SWITCH_ON_SSR_PIN, HIGH); delay(200);
    digitalWrite(HEATING_SWITCH_OFF_RELAY_PIN, LOW); delay(100);
    digitalWrite(HEATING_SWITCH_ON_SSR_PIN, LOW); delay(100);
    if(MAIN_DEBUG) DEBUG_OUTPUT.println(sE("\n\n!!!Error: Loop WDT reset! at: ") + millis());
    ESP.restart(); //https://github.com/esp8266/Arduino/issues/1722 // ESP.reset() and ESP.restart()?
  }
}
  

