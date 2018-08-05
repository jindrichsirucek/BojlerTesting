
/*Freeheap during loops: loop(31280) 35232 - at start
RESET FLASH: 
esptool.py --port $(ls  /dev/cu.* | grep -v 'Blue') erase_flash
Upload EMPTY BIN
esptool.py --port $(ls  /dev/cu.* | grep -v 'Blue') write_flash 0x000000 /Users/jindra/Documents/Platform.IO\ Projects/_Empty\ sketch\ -\ erase\ flash/blank4mb.bin
Upload AT COMMANDS
esptool.py --port /dev/cu.SLAB_USBtoUART write_flash 0x000000 /Users/jindra/Downloads/AT_bin/512+512/user1.1024.new.2.bin
DOWNLOAD BINARY 
esptool.py --port $(ls  /dev/cu.* | grep -v 'Blue') --baud 115200 read_flash 0x00000 0x400000 backup.img
ls /dev/tty.* | ls /dev/cu.* | ls  /dev/cu.* | grep -v 'Blue'

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
//  2 - D4   |   D2 - 4   -  I2C SCL
//  3 - D9   |   D3 - 0   -  IN - FLASH button
//  4 - D2   |   D4 - 2   -  LED pin
//  5 - D1   |   D5 - 14  -  
//  9 - D11  |   D6 - 12  -  IN - WaterFlow sensor
// 10 - D12  |   D7 - 13  -  OneWire
// 16 - D0   |   D8 - 15  -  
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
#define HEATING_SWITCH_OFF_RELAY_PIN RXD_PIN //D9 = RXD -  15 - not booting!! when used for relay
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
#define HTTPS_REDIRECT_DEBUG false
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
//WIFI AP
// #define AP_SSID "APJindra"
// #define AP_PASSWORD "cargocargo"

#define DATA_SERVER_HOST_ADDRESS F("script.google.com")
#define DATA_SERVER_SCRIPT_URL F("/macros/s/AKfycbziCOySJ-cxsjn6-v8WpIbcsmlE77RqkzGX728nht2wO4HYmvVK/exec"); //Testing script, data goes here: https://docs.google.com/spreadsheets/d/1JyISlQ2zlttjRfxDFLuECTkgKGiBDn5A_I6WSVT3l5s/edit#gid=538340115   //Script address: https://script.google.com/macros/d/Magj9VtC_7VJMKhlPxI7rWgcRztdqeR-I/edit?uiv=2&mid=ACjPJvEivmgMleau6F0s_c2EsMfGlndE8W7Ls8gVj1MQ_8lgOg0eUus8N2GJ4NBMyAmUaa3dtUi9fnETPQNdpStDwnuy1LGlfiQFRvEOzq0JVA3E0nz5a-Jfqqk9GdIOuEocXf9pIdvamg

#define SETTINGS_FILENAME F("/settings.json")

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
////////////////////////////////////////////////////////
//SAFETY MAX and MIN SETTINGS
////////////////////////////////////////////////////////
#define MAXIM_FILE_LOG_SIZE maxSizeToSend_global
#define OSWATCH_RESET_TIME 300 //in secconds

////////////////////////////////////////////////////////
//DEVELOPMENT SETTINGS
////////////////////////////////////////////////////////
#define TEST_MODULE "" //Comment when working on home 
// #define DOMA_MODULE "" //Comment when working on home 
// #define PRODUCTION_MODULE "" //Comment when working on home 

#ifdef TEST_MODULE
  // #define NODE_NAME "PurpleBattery - nodemcu + STEPUP5V"
  // #define NODE_NAME "RedBattery - 622K + direct"
  // #define NODE_NAME "PurpleBattery StepDown 3.3V (direct module) + Esp.vcc()"
  // #define NODE_NAME "TestModule_LongRunHome"
  // #define NODE_NAME "TestModule_RadioOn_ButNoUpload_Purple_USBPlug_4sensors_Lolin"
  // #define NODE_NAME "TestModule_RadioOn_ButNoUpload_Red2_USBPlug_4sensors_Lolin"
  // #define NODE_NAME "TestModule_RadioOff_NoUpload_Purple1_USBPlug_4sensors_Lolin"
  // #define NODE_NAME "TestModule_RadioOff_NoUpload_Purple2_FromBatteryToMini3.3Vstab_3sensors_ESPQ12"
  #define NODE_NAME "Testing"
  #define DISPLAY_LOG_MESSAGES false
  #define QUICK_DEVELOPMENT_BOOT !true
  #define WIFI_RADIO_OFF true
  // #define UPLOADING_DATA_MODULE_ENABLED false

  // #define DEBUG_OUTPUT Serial
  // #define ERROR_OUTPUT Serial
  #define DEBUG_OUTPUT RemoteDebug
  #define ERROR_OUTPUT RemoteDebug
#endif

#ifdef DOMA_MODULE
  #define NODE_NAME "Doma"
  #define WIFI_RADIO_OFF false
  #define DISPLAY_LOG_MESSAGES true
  #define QUICK_DEVELOPMENT_BOOT false
  #define DEBUG_OUTPUT RemoteDebug
  #define ERROR_OUTPUT RemoteDebug
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
  #define ERROR_OUTPUT RemoteDebug
  #undef OTA_MODULE_ENABLED
  #define OTA_MODULE_ENABLED false
  #undef SPIFFS_DEBUG
  #define SPIFFS_DEBUG false
#endif

#define COMPILATION_DATE sE(__TIME__) + E(" ")+ E(__DATE__)
// #define COMPILATION_DATE sE("10:00:00") + E(" ")+ E(__DATE__)
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

//1 - arduino(programatic), 2 - Manual(thermostat), 0 - bojler Off (electricity)
enum {
  BOILER_CONTROL_OFF = 0,
  BOILER_CONTROL_PROGRAMATIC,
  BOILER_CONTROL_MANUAL
};

struct TempSensorAddressesStruct {
  DeviceAddress bojler;
  DeviceAddress pipe;
  DeviceAddress roomTemp;
  DeviceAddress insideFlow;
};



struct EepromSettingsStruct {
  struct TempSensorAddressesStruct tempSensorAddresses;
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

struct TempGenericStruct {
  TempSensorStruct sensors[MAX_TEMP_SENSORS_COUNT];
  byte topHeating = 0;
  byte lowDroping = 0;
  byte lastHeated = 0;
  byte boilerControlStyle = BOILER_CONTROL_PROGRAMATIC; 
  uint8_t asyncMeasurmentCounter = MAX_TEMPERATURE_SAMPLES_COUNT;
};

struct AnimationGenericStruct {
  byte (*activeSymbol)[8]; //Symbol animation array reference
  int8_t progress = -1;
  uint8_t framesCount;
  uint16_t stepDuration;
};


struct GlobalStruct {
  TempGenericStruct TEMP;
  AnimationGenericStruct ANIMATION;
  unsigned long nodeStatusUpdateTime = 60 * 60 * 1000; //60 * 60 * 1000 = 1 hour
  bool connectToLastRememberedWifi = true;

  bool nodeInBootSequence = true;
  uint8_t spaceCountDebug = 0; 
};

////////////////////////////////////////////////////////
//  GLOBAL VARIABLES
////////////////////////////////////////////////////////
// Tasker tasker(ERROR_OUTPUT, DEBUG_OUTPUT); //Error fisrt, normal debug second
RemoteDebug RemoteDebug;
Tasker tasker(ERROR_OUTPUT, DEBUG_OUTPUT); //Error fisrt, normal debug second
//SW Loop watchdog
Ticker tickerOSWatch;
GlobalStruct GLOBAL;

String lastCurrentMeasurmentText_global = "";
bool lastElectricCurrentState_global = false;
bool lastElectricityConnectedState_global = false;

uint32_t lastWaterFlowSensorCount_global = 0;
volatile uint16_t waterFlowSensorCount_ISR_global;  // Measures flow meter pulses
int32_t waterFlowDisplay_global = 0;
unsigned long lastWaterFlowResetTime_global = 0; //last time measured water flow in millis()
bool isWaterFlowingLastStateFlag_global = false; //water flowing flag

String objectAskingForResponse_global = "";
String responseText_global = "";

bool isThereEventToSend_global = false;
uint16_t logFileRowsCount_global = 0;
uint16_t curentLogNumber_global = 0;
uint32_t maxSizeToSend_global = 50000;
String lastNodeStateTempString_global = "";

uint8_t dislayRotationPosition_global = 0;

////////////////////////////////////////////////////////
//DEBUGING VARIABLES and SETTINGS
////////////////////////////////////////////////////////
static unsigned long lastWdtLoopTime_global;

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
#define isFlashButtonPressed() digitalRead(FLASH_BUTTON_PIN) == 0
#define isElectricityConnected() digitalRead(ELECTRICITY_SENSOR_PIN)
void ICACHE_RAM_ATTR osWatch();
void ICACHE_RAM_ATTR ISR_flowCount();//Preprocesor
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//Program itself - initialization
void setup()
{
  Serial.begin(115200);
  Serial.println(sE("\n\nStarted Serial, free ram: ") + ESP.getFreeHeap());
  
  if(DISPLAY_MODULE_ENABLED) 
    i2cBus_setup();//+Display setup
  
  runServiceMenuIfNeeded();
  
  SPIFFS_setup();
  printBegginGraphics();
  remoteDebug_setup();

  printSettingsStructSavedInEeeprom();
  printResetInfo();

  ////////////////////////
  // BUSES & SENSORS & CONTROLS INITIALIZATION
  ////////////////////////
  if(TEMP_MODULE_ENABLED)            oneWireBus_setup();  
  if(TEMP_MODULE_ENABLED)            temperature_setup();
  if(WATER_FLOW_MODULE_ENABLED)      waterFlowSensor_setup();
  if(CURRENT_MODULE_ENABLED)         currentAndElectricity_setup();
  if(HEATING_CONTROL_MODULE_ENABLED) relayBoard_setup();

  ////////////////////////
  // LOOP WATCHDOG
  ////////////////////////
  lastWdtLoopTime_global = millis();
  //Loop watchdog attaching interrupt
  tickerOSWatch.attach_ms(((OSWATCH_RESET_TIME) * 1000), osWatch);
  
  ////////////////////////
  // WIFI, DEBUG, TELNET
  ////////////////////////
  if(QUICK_DEVELOPMENT_BOOT == false)
    autoWifiConnect();

  //Automatic check new FW Update
  bool restartAfterInitalSequence = (isWifiConnected() && QUICK_DEVELOPMENT_BOOT == false)? checkUpdatesFromHttpServer() : false;
  
  ////////////////////////
  // BOOTING SEQUENCE
  ////////////////////////
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug(RESET_SPACE_COUNT_DEBUG) + E("F:MAIN_setup()"));
  DEBUG_OUTPUT.println(sE("Initializing node ") + E(NODE_NAME));

  RemoteDebug.handle();
  
  if(QUICK_DEVELOPMENT_BOOT == false)
    saveAndSendExceptionLogFile();

  if(isWifiConnected() && QUICK_DEVELOPMENT_BOOT == false)
  {//With internet connection
    displayServiceLine(cE("Init: Server"));
    sendDebugInformationsAfterReset();
    RemoteDebug.handle();

    if(sendAllLogFiles())
      doNecesaryActionsUponResponse();
  }
  else
  {//Without internet connection
    displayServiceLine(cE("Init: Offline"));
    if(loadLastSavedBoilerStateFromFile())
    {
      logNewNodeState(E("settings file loaded from EEPROM"));
      doNecesaryActionsUponResponse();
    }
    else
      logNewNodeState(E("settings file NOT loaded from EEPROM"));
  }
  
  RemoteDebug.handle();
  
  yield_debug();

  if(OTA_MODULE_ENABLED) OTA_setup();

  if(DISPLAY_MODULE_ENABLED) lcd_setup();

  if(restartAfterInitalSequence)
    restartEsp(E("HTTP FW Updated"));

  GLOBAL.nodeInBootSequence = false;
}


//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

void loop()
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(E("\nF:MAIN_loop()"));
  //---------------------------------- NON-SKIPABLE TASKS ------------------------------------------
  if(true)                          tasker.setInterval(feedWatchDog_taskerLoop, 1*SEC, TASKER_SKIP_NEVER);
  if(UPLOADING_DATA_MODULE_ENABLED) tasker.setInterval(uploadLogFile_loop, 0*MIN + 10*SEC, TASKER_SKIP_NEVER);
  if(OTA_MODULE_ENABLED)            tasker.setInterval(silentOTA_loop,  500*MSEC, TASKER_SKIP_NEVER);
  if(DISPLAY_MODULE_ENABLED)        tasker.setInterval(displayData_loop, 1*SEC, TASKER_SKIP_NEVER);
  if(TEMP_MODULE_ENABLED)           tasker.setInterval(temperature_loop, 2*SEC, TASKER_SKIP_NEVER);
  //---------------------------------- SKIPABLE TASKS ----------------------------------------------
  if(WATER_FLOW_MODULE_ENABLED)     tasker.setInterval(waterFlow_loop, 20*SEC, TASKER_SKIP_WHEN_NEEDED);
  if(CURRENT_MODULE_ENABLED)        tasker.setInterval(current_loop, 6*SEC, TASKER_SKIP_WHEN_NEEDED);
  //---------------------------------- ONE TIME TASKS --------------------------------------------------
  if(SENDING_BEACON_MODULE_ENABLED) tasker.setTimeout(checkSystemState_loop,  GLOBAL.nodeStatusUpdateTime, TASKER_SKIP_NEVER); //60 * 60 * 1000 = 1 hour
  //---------------------------------- OUTRO TASK --------------------------------------------------
  if(SHOW_WARNING_DEBUG_MESSAGES)   tasker.setOutroTask(outroTask_loop);

  DEBUG_OUTPUT.printf(cE("FreeHeap:%d\n"), ESP.getFreeHeap());
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

  //Check if there is some error message after last function called
  if(RemoteDebug.isThereErrorMessage())
  {
    if(MAIN_DEBUG) DEBUG_OUTPUT.println();
    String uri = E("&quickEvent=");
    uri += URLEncode(RemoteDebug.getLastErrorMessage());
    uploadDebugLogFileWithGetParams(uri);
  }
  if(isDebugButtonPressed())
  {
    uint8_t number = 100;
    while(number--)
      ISR_flowCount();
  }
}


void uploadLogFile_loop(int)
{
  //Dont upload over wifi if water is flowing (only save new states, sends it later)
  if(isWaterFlowingLastStateFlag_global == true && isFlashButtonPressed() == false)
    return;

  //Dont upload over wifi if there is no electricity (on battery)
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
  
  if(OTA_MODULE_ENABLED) 
    OTA_begin();

  if(MAIN_DEBUG) DEBUG_OUTPUT.print(getUpTimeDebug(RESET_SPACE_COUNT_DEBUG) + E("F:uploadLogFile_loop(int)"));

  if(isThereAnyPendingEventsToSend())
  {
    if(MAIN_DEBUG) DEBUG_OUTPUT.println();
    
    bool success = sendAllLogFiles();
    if(success)
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
//Funkce reset spiffs s parametrem - debug log, nacte nejdriv debugovací infomrace z extra logu, přidá důvot resetu a pak uloží do nového souboru , tak abych nepřišel o olog co se stalo a proč se to stalo..
//Při formátování SPIFFS error sobor zkusit zachránit načtením do Stringu RAMKY ale nastavit max několik řádků at to nespadne na nedostatek ramky
//Error logy ukládat do souboru error.log a ty odesílat na net + výstřižek z normálního logu před chybou třeba 5 řádků k tomu přidat.
//vyřešit extra ERROR loging!! to se nesmí ztrácet, nedoručené packety odpovědi apod.. to chci vše zapisovat do logu
//NodeName - nastavovat v Debug sheetu ne pevně v kodu!!
//Porestartu modulu zkontrolovat nejdříve zda kolem není připojená "servisní síť" ESP-sevice - pokud ano, tak se připojí na ní a hodí se do konfiguaračního režimu, zapne se server, který bude přijímat webové požadavky přes get params a později vytvoří normálně servisí webové rozhraní
//Logovat vsechny warningy jako nlze se připojit k wifi, nebo teplotní vzorek nebyl dobře načte a errory jako není připjen teplotní senzro apod
//Inteligentní nastavení startu času ohřevu vody v noci, tak aby se ohřev ukončil právě v okamžik kdy dojde k vypnutí NT, aby nebyl bojler nahřátej a pak ještě 2 hodiny nejel NT a bojler nám mezitím chládnul (zbytečné ztráty)
//HOTOVO? //Někde na display zobrazit ohřev (blíkání teplotního symbolu např? nebo tečka v levém rohu?) 
//Zobrazovat ne počet litrů v bojleru, ale vynásobeno koeficentem ředění na 40°C vodu - prostě kolik zbývá 40°C vody aby šlo poznat kolik tam ej vody na praktické využití ibez toho,že bych musel vědět původní teplotu
//Hodnotu koeficientu u ampérměřáku uložit do eeprom a udělat možnost tuto hotnotu měnit v google sheetu namísto v kodu
//SPeed up upload preocess https://github.com/esp8266/Arduino/issues/1853
//Write/read test of SPIFF in each cycle
//v remote debug - předělat telnet pouze na přání, vyhodit ho z těch funckí aby se server nestartoval automaticky
//v remote debug - zbavit se error buferu - použít nějak ten normální buffer 150 ramky ušetřené
//Udělat měřená zda teče voda jde tak, že se hodí do ISR funkce global flag, případně se porovná poslední ISR s aktuálním ISR aby se vědělo zda od posledně tekla voda


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

    //x°C difference on pipe to send
    if((GLOBAL.TEMP.sensors[PIPE].temp - GLOBAL.TEMP.sensors[PIPE].lastTemp) > 5)
    {
      logNewNodeState(E("Pipe Temp: rised"));
      GLOBAL.TEMP.sensors[PIPE].lastTemp = GLOBAL.TEMP.sensors[PIPE].temp;
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

  controlHeating_loop(GLOBAL.TEMP.sensors[BOJLER].temp);
}


void current_loop(int)
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug(RESET_SPACE_COUNT_DEBUG) + E("F:current_loop()"));

  //Electricity
  // DEBUG_OUTPUT.print(lastElectricityConnectedState_global);
  // DEBUG_OUTPUT.print(isElectricityConnected());

  // if(MAIN_DEBUG) DEBUG_OUTPUT.println(sE("Electricity is now: ") + (isElectricityConnected()? E("ON") : E("OFF")));
  if(lastElectricityConnectedState_global != isElectricityConnected())
  {
    lastElectricityConnectedState_global = !lastElectricityConnectedState_global;
    logNewNodeState(sE("Electricity: ") + (lastElectricityConnectedState_global? E("ON") : E("OFF")));
  }

  //Curent
  bool actualElectricCurrentState = isThereElectricCurrent();

  if(lastElectricCurrentState_global == true && actualElectricCurrentState == false)
  {
    waterFlowDisplay_global = 0; //Vynuluj měření spotřeby teplé vody - bojler je po vypnutí ohřevu celý nahřátý
    setLastHeatedTemp();
    lcd_setup(); //resets LCD - if some errors
    //TODO : převést na funkci a funkci dát do display souboru kam patří
    lastElectricCurrentState_global = actualElectricCurrentState;
    // logNewNodeState(E("watErflOwdisplay_reset"));
    logNewNodeState(E("Current: dropped"));
  }
  else if(lastElectricCurrentState_global == false && actualElectricCurrentState == true)
  {
    lastElectricCurrentState_global = actualElectricCurrentState;
    logNewNodeState(E("Current: rised"));
  }
}


void waterFlow_loop(int)
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug(RESET_SPACE_COUNT_DEBUG) + E("F:waterFlow_loop()"));

  bool isWaterFlowingRightNowFlag = isWaterFlowingRightNow();
  if(isWaterFlowingLastStateFlag_global == isWaterFlowingRightNowFlag)
    return;//Nothing changed

  if(isWaterFlowingLastStateFlag_global == false && isWaterFlowingRightNowFlag) //watter started to flow
  {
    logNewNodeState(E("Water: started"));
    lastWaterFlowResetTime_global = millis();
  }
  else//watter stopped flowing
    logNewNodeState(E("Water: stopped"));

  isWaterFlowingLastStateFlag_global = !isWaterFlowingLastStateFlag_global;
}


String getSystemStateInfo()
{
  String systemStateInfo = sE(" ") +
  E("Uptime:") + getUpTime() +
  E("\rUpdateTime:") + formatTimeToString(GLOBAL.nodeStatusUpdateTime) +
  E("\rtopHeatingTemp:") + GLOBAL.TEMP.topHeating +
  E("\rlowDropingTemp:") + GLOBAL.TEMP.lowDroping +
  //E("TimeFromLastUpdate: ") + getTimeFromLastUpdate() + E("\r");
  E("\rtotalErrors:") + totalErrorCount_global +
  // E("\rERROR_COUNT_PER_HOUR: ") + (totalErrorCount_global / millis() / 1000 / 60 / 60) + E("\r");
  E("\rnotParsed: ") + notParsedHttpResponses_errorCount +
  E("\rparsedOk: ") + parsedHttpResponses_notErrorCount +
  E("\rWaterFlowCount:") + lastWaterFlowSensorCount_global +
  E("\rwaterFlowDisplay:") + waterFlowDisplay_global +
  E("\rlastWaterFlowResetTime") + formatTimeToString(lastWaterFlowResetTime_global) +
  // E("\rTime&Date: ") + getNowTimeDateString();
  E("\rWiFi.RSSI(): ") + WiFi.RSSI() + E("dBm");
  return systemStateInfo;
}

void checkSystemState_loop(){return checkSystemState_loop(0);}
void checkSystemState_loop(int)
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug(RESET_SPACE_COUNT_DEBUG) + E("F:checkSystemState_loop()"));

  logNewNodeState(E("Node: beacon alive"));//zároveň slouží jako beacon alive

  if(RemoteDebug.isThereWarningMessage())
    logWarningMessage(RemoteDebug.getLastWarningMessage());

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
  lastWdtLoopTime_global = millis();
}

void ICACHE_RAM_ATTR osWatch(void) {
    unsigned long t = millis();
    unsigned long last_run = abs(t - lastWdtLoopTime_global);
    if(last_run >= (OSWATCH_RESET_TIME * 1000))
     {
      ERROR_OUTPUT.println(sE("\n\n!!!Error: Loop WDT reset! at: ") + millis());
      restartEsp(); 
    }
}
  




