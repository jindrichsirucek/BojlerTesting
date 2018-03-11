//Before setup: FreeHeap:30448/31912/34240/34544
//Before loop: FreeHeap:8200/10728/10576/10848/12320

//31568/10384

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
  #build_flags = -Wl, -Wl,-T${PIOPACKAGES_DIR}/ldscripts/esp8266.flash.4m1m.ld
  Users/jindra/.platformio/platforms/espressif8266/ldscripts
  #esp8266.flash.4m.ld - tenhle je 3MB SPIFFS a 1MB scetch tímto nahradit tento: esp8266.flash.4m1m.ld
*/
//Reset auto clean option: 
//https://github.com/gepd/Deviot/issues/200



//Libraries included
#include "define.h"

#include "FS.h"
#include <RemoteDebug.h> //#include "ProjectSpecificLibraries/RemoteDebug/RemoteDebug.h"
#include <ESP8266WiFi.h>
#include "TaskerModified.h"
#include <OneWire.h> //its here because of using typeDefs in different tabs
#include <DallasTemperature.h>
#include <EspSaveCrash.h> //changed offset and basic size of buffer
#include <Ticker.h>

  
//------------------------- PIN DEFINITIONS ----------------------------
//  0 - D3   |   D0 - 16
//  1 - D10  |   D1 - 5
//  2 - D4   |   D2 - 4
//  3 - D9   |   D3 - 0
//  4 - D2   |   D4 - 2
//  5 - D1   |   D5 - 14
//  9 - D11  |   D6 - 12
// 10 - D12  |   D7 - 13
// 16 - D0   |   D8 - 15
// 13 - D7   |   D9 - 3
// 14 - D5   |  D10 - 1
// 15 - D8   |  D11 - 9
// 16 - D0   |  D12 - 10

// A0 - IN - Current sensor
// D0 - 16  -  Heating Relay control
// D1 - 5  -   I2C SDA
// D2 - 4  -   I2C SCL
// D3 - 0  -   IN - FLASH button
// D4 - 2  -   LED pin
// D5 - 14  -  
// D6 - 12  -  IN - WaterFlow sensor
// D7 - 13  -  OneWire
// D8 - 15  -  
// D9 - 3  -   RXD
// D10 - 1  -  TXD 
// D11 - 9  -  NOT ACCESIBLE
// D12 - 10 -  NOT ACCESIBLE

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
#define HEATING_SWITCH_OFF_RELAY_PIN D9 //D9 = RXD -  15 - not booting!! when used for relay
#define ELECTRICITY_SENSOR_PIN 16 // 16-D0 it has to be 16 only port with input INPUT_PULLDOWN_16

//----------------------------------------------------------------------
//----------------------------------------------------------------------

// #define DEVELOPING_SETTINGS "" //Comment when working on home 

#ifdef DEVELOPING_SETTINGS
  // #define NODE_NAME "PurpleBattery - nodemcu + STEPUP5V"
  // #define NODE_NAME "RedBattery - 622K + direct"
  // #define NODE_NAME "PurpleBattery StepDown 3.3V (direct module) + Esp.vcc()"
  // #define NODE_NAME "TestModule_LongRunHome"
  // #define NODE_NAME "TestModule_RadioOn_ButNoUpload_Purple_USBPlug_4sensors_Lolin"
  // #define NODE_NAME "TestModule_RadioOn_ButNoUpload_Red2_USBPlug_4sensors_Lolin"
  // #define NODE_NAME "TestModule_RadioOff_NoUpload_Purple1_USBPlug_4sensors_Lolin"
  // #define NODE_NAME "TestModule_RadioOff_NoUpload_Purple2_FromBatteryToMini3.3Vstab_3sensors_ESPQ12"
  #define NODE_NAME "Testing"
  #define WIFI_RADIO_OFF true

  #define DEBUG_OUTPUT Serial
  #define ERROR_OUTPUT Serial
  // #define DEBUG_OUTPUT RemoteDebug
  // #define ERROR_OUTPUT RemoteDebug
  
#else
  #define WIFI_RADIO_OFF false
  #define NODE_NAME "Doma"
  #define DEBUG_OUTPUT RemoteDebug
  #define ERROR_OUTPUT RemoteDebug
#endif
//192.168.0.18


////////////////////////////////////////////////////////
//MODULE ENABLING SETTINGS
////////////////////////////////////////////////////////

#define OTA_MODULE_ENABLED true   //1kB of Heap
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
#define TEMPERATURE_DEBUG true
#define RELAY_DEBUG false
#define CURRENT_DEBUG false
#define WATER_FLOW_DEBUG false

#define SHOW_ERROR_DEBUG_MESSAGES true
// #define DEBUG_SERVICE_MESSAGES true
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
#define LOGGER_FILE_NAME F("/progressFile.log")

////////////////////////////////////////////////////////
//AUTONOMY SETTINGS
////////////////////////////////////////////////////////
#define TEMP_DIFFERENCE_TO_ASK_FOR_RESPONSE 1 //in degrees of celsia
#define MIMIMAL_SENDING_TEMP_DIFFERENCE 0.07 // after the difference between two measurments cross this level, data will be uploaded. Lower values bigger acuracy, but values can jump up and down around one value - too many samples with no real value
#define ERROR_TEMP_VALUE_MINUS_127 -127
#define ERROR_TEMP_VALUE_PLUS_85 85
////////////////////////////////////////////////////////
//SAFETY MAX and MIN SETTINGS
////////////////////////////////////////////////////////
#define TASKER_MAX_TASKS 10 // define max number of tasks to save precious Arduino RAM, 10 is default value
#define MAXIM_FILE_LOG_SIZE maxSizeToSend_global
#define OSWATCH_RESET_TIME 120 //in secconds

////////////////////////////////////////////////////////
//  STRUCTS DEFINITIONS
////////////////////////////////////////////////////////
enum{
  BOJLER = 0,
  PIPE,
  ROOM_TEMP,
  INSIDE_FLOW,
  MAX_TEMP_SENSORS_COUNT
};

struct TempSensorStruct {
  DeviceAddress address;
  bool sensorConnected = false;
  float temp = ERROR_TEMP_VALUE_MINUS_127;
  // float lastSuccesfullTemp;
  const char *sensorName;
};

////////////////////////////////////////////////////////
//  GLOBAL VARIABLES
////////////////////////////////////////////////////////
Tasker tasker;
RemoteDebug RemoteDebug;
Ticker tickerOSWatch; //SW Loop watchdog

TempSensorStruct tempSensors[MAX_TEMP_SENSORS_COUNT];

float lastTemp_global = ERROR_TEMP_VALUE_MINUS_127;
byte topHeatingTemp_global = 40;
byte lowDropingTemp_global = 35;
byte boilerTempControlStyle_global = 1; //1 - arduino(programatic), 2 - Manual(thermostat), 0 - bojler Off (electricity)

unsigned long nodeStatusUpdateTime_global = 60 * 60 * 1000; //60 * 60 * 1000 = 1 hour

String lastCurrentMeasurmentText_global = "";
bool lastElectricCurrentState_global = false;

uint32_t lastWaterFlowSensorCount_global = 0;
volatile uint16_t waterFlowSensorCount_ISR_global;  // Measures flow meter pulses
int32_t waterFlowDisplay_global = 0;
unsigned long lastWaterFlowResetTime_global = 0; //last time measured water flow in millis()
bool isWaterFlowingLastStateFlag_global = false; //water flowing flag

/// Restart will be triggered on this time
unsigned long espRestartTime_global = 160 * 1000; //this value need to be changed also in function itself

String systemStateInfo_global = "";
String objectAskingForResponse_global = "";
String responseText_global = "";
bool isThereEventToSend_global = false;

uint16_t curentLogNumber_global = 0;
uint32_t maxSizeToSend_global = 50000;


byte (*activeAnimationSymbol_global)[8]; //Symbol animation array reference
int8_t animationProgress_global = -1;
uint8_t activeAnimationFramesCount_global;
uint16_t activeAnimationStepDuration_global;
uint8_t dislayRotationPosition_global = 0;

////////////////////////////////////////////////////////
//DEBUGING VARIABLES and SETTINGS
////////////////////////////////////////////////////////
static unsigned long lastWdtLoopTime_global;

uint16_t notParsedHttpResponses_errorCount = 0;
uint16_t parsedHttpResponses_notErrorCount = 0;
uint16_t totalErrorCount_global = 0;

uint8_t spaceCountDebug_global = 0; 
uint8_t getDebuggerSpacesCount() {return spaceCountDebug_global;}
#define RESET_SPACE_COUNT_DEBUG 0

// ADC_MODE(ADC_VCC); //Battery measurment throw inner system, cant be used when you want Analog values

////////////////////////////////////////////////////////
//  FUNCTION DEFINITIONS
////////////////////////////////////////////////////////
#define isDebugButtonPressed() isFlashButtonPressed()
#define isFlashButtonPressed() digitalRead(FLASH_BUTTON_PIN) == 0
#define isElectricityConnected() digitalRead(ELECTRICITY_SENSOR_PIN)

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//Program itself - initialization
void setup()
{
  Serial.begin(115200);
  Serial.println(E("\n\nSerial begin\n"));

  Serial.println(sE("Last reset reason: ") + ESP.getResetReason().c_str());
  printResetInfo();

  Serial.printf(cE("\nFreeHeap:%d\n"), ESP.getFreeHeap());
  //Loop watchdog
  lastWdtLoopTime_global = millis();
  tickerOSWatch.attach_ms(((OSWATCH_RESET_TIME) * 1000), osWatch);
  SPIFFS_setup();
  ////////////////////////
  // BUSES INITIALIZATION
  ////////////////////////
  if(DISPLAY_MODULE_ENABLED) i2cBus_setup(); //+Display setup
  if(TEMP_MODULE_ENABLED) oneWireBus_setup(); //Temp sensors

  ////////////////////////
  // SERVICE MENU
  ////////////////////////
  showServiceMessage(E("Service Menu?"));
  delay(1000); // wait for a second to enter service menu
  if(isFlashButtonPressed())
    showServiceMenu();
  else
    loadNodeSettingsAfterBoot();

  showServiceMessage(E("Booting.."));
  ////////////////////////
  // WIFI, DEBUG, TELNET
  ////////////////////////
  wifiConnect();

  RemoteDebug.begin(cE("Telnet_HostName")); // Initiaze the telnet server
  RemoteDebug.setLogFileEnabled(true);
  RemoteDebug.setResetCmdEnabled(true); // Enable the reset command
  RemoteDebug.setSerialEnabled(true);
  RemoteDebug.showDebugLevel(false);
  RemoteDebug.handle();

  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:MAIN_setup()"));
  DEBUG_OUTPUT.println(sE(NODE_NAME) + E(" node initializing.."));

  RemoteDebug.handle();

  if(isWifiConnected())
  {//With internet connection
    sendDebugInformationsAfterReset();
    RemoteDebug.handle();

    File f = openFile(("/exception.stack"), "a");
    SaveCrash.print(f);
    f.close();
    SaveCrash.clear();

    File fileTosend = openFile(("/exception.stack"), "r");
    bool successfullySend;
    if(fileTosend.size() > 22)
    successfullySend = sendGetParamsWithPostFile(E("&quickEvent=exception"), fileTosend);
    f.close();

    if(successfullySend)
    {
      File f = openFile(("/exception.stack"), "w");
      f.close();
    }

    sendAllLogFiles();
    doNecesaryActionsUponResponse();
  }
  else
  {//Without internet connection
    if(loadLastSavedBoilerStateFromFile())
    {
      logNewNodeState(E("settings file loaded from EEPROM"));
      doNecesaryActionsUponResponse();
    }
    else
      logNewNodeState(E("settings file NOT loaded from EEPROM"));
  }
  
  RemoteDebug.handle();
  ////////////////////////
  // SENSORS SETUP
  ////////////////////////
  if(TEMP_MODULE_ENABLED) temperature_setup();
  if(CURRENT_MODULE_ENABLED) current_setup();
  if(WATER_FLOW_MODULE_ENABLED) waterFlowSensor_setup();
  if(HEATING_CONTROL_MODULE_ENABLED)relayBoard_setup();

  
  yield_debug();
  RemoteDebug.handle();

  if(OTA_MODULE_ENABLED) OTA_setup();
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

void loop()
{
  Serial.printf(cE("FreeHeap:%d\n"), ESP.getFreeHeap());
  pinMode(ELECTRICITY_SENSOR_PIN, INPUT_PULLDOWN_16);

  if(MAIN_DEBUG) DEBUG_OUTPUT.println(E("\nF:MAIN_loop()"));
  //---------------------------------- NON-SKIPABLE TASKS ------------------------------------------
  if(true)                          tasker.setInterval(feedWatchDog_taskerLoop, 1*SEC, TASKER_SKIP_NEVER);
  if(UPLOADING_DATA_MODULE_ENABLED) tasker.setInterval(uploadLogFile_loop, 0*MIN + 10*SEC, TASKER_SKIP_NEVER);
  if(OTA_MODULE_ENABLED)            tasker.setInterval(silentOTA_loop,  500*MSEC, TASKER_SKIP_NEVER);
  if(DISPLAY_MODULE_ENABLED)        tasker.setInterval(displayData_loop, 1*SEC, TASKER_SKIP_NEVER);
  //---------------------------------- SKIPABLE TASKS ----------------------------------------------
  if(TEMP_MODULE_ENABLED)           tasker.setInterval(temperature_loop, 10*SEC, TASKER_SKIP_WHEN_NEEDED);
  if(WATER_FLOW_MODULE_ENABLED)     tasker.setInterval(waterFlow_loop, 20*SEC, TASKER_SKIP_WHEN_NEEDED);
  if(CURRENT_MODULE_ENABLED)        tasker.setInterval(current_loop, 6*SEC, TASKER_SKIP_WHEN_NEEDED);
  //---------------------------------- ONE TIME TASKS --------------------------------------------------
  if(SENDING_BEACON_MODULE_ENABLED) tasker.setTimeout(checkSystemState_loop,  nodeStatusUpdateTime_global, TASKER_SKIP_NEVER); //60 * 60 * 1000 = 1 hour
  //---------------------------------- OUTRO TASK --------------------------------------------------
  if(SHOW_WARNING_DEBUG_MESSAGES)   tasker.setOutroTask(outroTask_loop);

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
  //Dont upload over wifi if there is no electricity (on battery)
  if(isElectricityConnected() == false && WIFI_RADIO_OFF == true)
  {
    if(wifi_get_opmode() != NULL_MODE)
      turnWifiOff();
  
    if(MAIN_DEBUG) DEBUG_OUTPUT.print(getUpTimeDebug(RESET_SPACE_COUNT_DEBUG) + E("F:uploadLogFile_loop(int): Disabled - Battery saving"));
    return;
  }

  //If wifi in sleep mode - radiof OFF - turn it on before uploading
  if(wifi_get_opmode() == NULL_MODE)
    turnWifiOn();
  
  if(MAIN_DEBUG) DEBUG_OUTPUT.print(getUpTimeDebug(RESET_SPACE_COUNT_DEBUG) + E("F:uploadLogFile_loop(int)"));

  if(wifiConnect())
    if(OTA_MODULE_ENABLED) OTA_begin();


  if(isThereAnyPendingEventsToSend() && !isWaterFlowingRightNow())
  {
    if(MAIN_DEBUG) DEBUG_OUTPUT.println();
    bool success = sendAllLogFiles();
    if(success)
    {
      setPendingEventToSend(false);
      doNecesaryActionsUponResponse();
      // maxSizeToSend_global += 10000;
    }
    // else
    // maxSizeToSend_global = 10000;
  }
  else if(MAIN_DEBUG) DEBUG_OUTPUT.println(E(" - Nothing to send!"));
}

bool isThereAnyPendingEventsToSend()
{
  return isThereEventToSend_global;
}


void setPendingEventToSend(bool isThereEventToSend)
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:setPendingEventToSend(bool isThereEventToSend): ") + ((isThereEventToSend) ? E("true") : E("false")));
  isThereEventToSend_global = isThereEventToSend;
}



//No temp measurment - relay driving Off,
//zamezit zbytečném relé spínání
//poslat zprávu že přijal změnu refresh time nodeinfoupdate/změna adrsy senzoru taky posílat
//přichozí timestamp uložit do notes a když bude rozdíl velký tak podbarvit červeně - aby to šlo vidět
//obnovení OTA po připojení na internet? upravi si to samo iP adresu?
//rozhodit všechny funkce na zjištění aktuálního stavu do souborů kam patří z funkce odesílání.. water flow do water atd
//Funkce reset spiffs s parametrem - debug log, nacte nejdriv debugovací infomrace z extra logu, přidá důvot resetu a pak uloží do nového souboru , tak abych nepřišel o olog co se stalo a proč se to stalo..
//Při formátování SPIFFS error sobor zkusit zachránit načtením do Stringu RAMKY ale nastavit max několik řádků at to nespadne na nedostatek ramky
//Error logy ukládat do souboru error.log a ty odesílat na net + výstřižek z normálního logu před chybou třeba 5 řádků k tomu přidat.
//vyřešit extra ERROR loging!! to se nesmí ztrácet, nedoručené packety odpovědi apod.. to chci vše zapisovat do logu
//Ukládat na web zbylou teplou vodu a po restartu poslat zpět do node, aby to vědělo jak je na tom voda (pokut to ale není víc jak 24hodin - to už by byla nahřáta znovu)
//Zbatit se Stringů v RedirectHTTPS.h!!!!
//NodeName - nastavovat v Debug sheetu ne pevně v kodu!!
//vyřešit ten reset pomocí NT elektřiny - watchdog chip
//Jeden řádek dat uložit do paměti Ram a když se zrovna odešle, tak se tím bude štřit SPIFF flash paměť - když je bojler na elektřině tak odesílá každý řádek
//Udělat SW WDG který bude krmený v hlavní smičce, který bude krmený jen v hlavním loopu, kdyby se to někde seklo ve smičce (možná krmený v outro tasku?), např při odesílání souboru se to může zaseknout
//Porestartu modulu zkontrolovat nejdříve zda kolem není připojená "servisní síť" ESP-sevice - pokud ano, tak se připojí na ní a hodí se do konfiguaračního režimu, zapne se server, který bude přijímat webové požadavky přes get params a později vytvoří normálně servisí webové rozhraní
//logovat změnu napájení- Even NT ON/OFF
//Nastavit statickou IP adresu po prvním načtením z DHCP - by mohlo zlepšit spojení u DANA na té jeho slabší wifině
//Vyřešit schopnost modulu stahovat si novou verzi firmwaru!
//Logovat vsechny warningy jako nlze se připojit k wifi, nebo teplotní vzorek nebyl dobře načte a errory jako není připjen teplotní senzro apod
//Inteligentní nastavení startu času ohřevu vody v noci, tak aby se ohřev ukončil právě v okamžik kdy dojde k vypnutí NT, aby nebyl bojler nahřátej a pak ještě 2 hodiny nejel NT a bojler nám mezitím chládnul (zbytečné ztráty)
//Někde na display zobrazit ohřev (blíkání teplotního symbolu např? nebo tečka v levém rohu?)
//Na display zobrazit že relé ohřevu je zapnuté nebo zobrazovat dokonce cílovou tepltou bojleru? možná blikaz na střídačku s UPTIME?
//Zobrazovat původní taplotu/aktuální/cílovou
//Zobrazovat ne počet litrů v bojleru, ale vynásobeno koeficentem ředění na 40°C vodu - prostě kolik zbývá 40°C vody aby šlo poznat kolik tam ej vody na praktické využití ibez toho,že bych musel vědět původní teplotu
//Zbavit se stringu v getSpareWaterInLittres místo toho předat refenrecni na proměnou




void temperature_loop(int)
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug(RESET_SPACE_COUNT_DEBUG) + E("F:temperature_loop()"));

  if(readTemperatures())
  ;//TODO if all measurment was done at first try
  //if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("   :returned: ") + (String)tempSensors[BOJLER].temp + "(°C)");

  if(isTemperatureCorrectMeasurment(tempSensors[BOJLER].temp) == false)
  {
    totalErrorCount_global++;
    lastTemp_global = ERROR_TEMP_VALUE_MINUS_127;
    return;
  }

  if(isTemperatureCorrectMeasurment(lastTemp_global) == false)
  {
    lastTemp_global = tempSensors[BOJLER].temp;
    return;//minimálně dvě měření po sobě jdoucí musí být skutečné hodnoty
  }

  if(abs(tempSensors[BOJLER].temp - lastTemp_global) >= MIMIMAL_SENDING_TEMP_DIFFERENCE)
  {
    if(abs((int)tempSensors[BOJLER].temp - (int)lastTemp_global) >= 1) //pokud měření právě překročilo hranici stupně
      if(((int)tempSensors[BOJLER].temp % TEMP_DIFFERENCE_TO_ASK_FOR_RESPONSE) == 0) //každé dva stupně si zažádej o vedení
        addObjectAskingForResponse(sE("Temp_") + TEMP_DIFFERENCE_TO_ASK_FOR_RESPONSE);
    lastTemp_global = tempSensors[BOJLER].temp;
    // DEBUG_OUTPUT.print(E(""));
    // DEBUG_OUTPUT.println(lastTemp_global);

    logNewNodeState(E("Temp: changed"));
  }

  controlHeating_loop(tempSensors[BOJLER].temp);
}


void current_loop(int)
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug(RESET_SPACE_COUNT_DEBUG) + E("F:current_loop()"));
  DEBUG_OUTPUT.print(isElectricityConnected());
  DEBUG_OUTPUT.println((isElectricityConnected()==1 ? E("Charging") : E("Battery")));

  bool actualElectricCurrentState = isThereElectricCurrent();

  if(lastElectricCurrentState_global == true && actualElectricCurrentState == false)
  {
    waterFlowDisplay_global = 0; //Vynuluj měření spotřeby teplé vody - bojler je po vypnutí ohřevu celý nahřátý
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

  if(isWaterFlowingLastStateFlag_global == false && isWaterFlowingRightNow() == true) //watter started to flow
  {
    logNewNodeState(E("Water: started"));
    isWaterFlowingLastStateFlag_global = !isWaterFlowingLastStateFlag_global;
    lastWaterFlowResetTime_global = millis();
    return;
  }

  if(isWaterFlowingLastStateFlag_global == true && isWaterFlowingRightNow() == false)//watter stopped flowing
  {
    // DEBUG_OUTPUT.println(E("Flow state has droped to zero.. sending update data.."));
    isWaterFlowingLastStateFlag_global = !isWaterFlowingLastStateFlag_global;
    logNewNodeState(E("Water: stopped"));
  }

  // waterFlowSensorCount_ISR_global = 0;
  // lastWaterFlowSensorCount_global = 0;
}


String getSystemStateInfo()
{

  String systemStateInfo = sE("");
  systemStateInfo += sE("Uptime: ") + getUpTime();
  systemStateInfo += sE("\nnodeStatusUpdateTime_global: ") + formatTimeToString(nodeStatusUpdateTime_global);
  systemStateInfo += sE("\ntopHeatingTemp_global: ") + topHeatingTemp_global;
  systemStateInfo += sE("\nlowDropingTemp_global: ") + lowDropingTemp_global;

  //systemStateInfo +=\n sE("TimeFromLastUpdate: ") + getTimeFromLastUpdate() + sE("\n");
  systemStateInfo += sE("\ntotalErrorCount_global: ") + totalErrorCount_global;
  //systemStateInfo += sE("\nERROR_COUNT_PER_HOUR: ") + (totalErrorCount_global / millis() / 1000 / 60 / 60) + sE("\n");

  systemStateInfo += sE("\nnotParsedHttpResponses_errorCount: ") + notParsedHttpResponses_errorCount;
  systemStateInfo += sE("\nparsedHttpResponses_notErrorCount: ") + parsedHttpResponses_notErrorCount;

  systemStateInfo += sE("\nlastWaterFlowSensorCount_global: ") + lastWaterFlowSensorCount_global;
  systemStateInfo += sE("\nwaterFlowDisplay_global: ") + waterFlowDisplay_global;
  systemStateInfo += sE("\nlastWaterFlowResetTime_global: ") + formatTimeToString(lastWaterFlowResetTime_global);

  systemStateInfo += sE("\nTime&Date: ") + getNowTimeDateString();
  systemStateInfo += sE("\nWiFi.RSSI(): ") + WiFi.RSSI() + E("dBm");


  return systemStateInfo;
}

void checkSystemState_loop(int)
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug(RESET_SPACE_COUNT_DEBUG) + E("F:checkSystemState_loop()"));

  addObjectAskingForResponse(E("checkSystemState_loop"));
  String systemStateInfo = E("\n\n");

  systemStateInfo += getESPStatusUpdate();

  logNewNodeState(E("Node: beacon alive"));//zároveň slouží jako beacon alive

  systemStateInfo_global = "";

  if(RemoteDebug.isThereWarningMessage())
    logWarningMessage(RemoteDebug.getLastWarningMessage());

  //if(SHOW_ERROR_DEBUG_MESSAGES) DEBUG_OUTPUT.println(systemStateInfo);
  //if(SHOW_ERROR_DEBUG_MESSAGES) WiFi.printDiag(Serial);

  if(SENDING_BEACON_MODULE_ENABLED)
    tasker.setTimeout(checkSystemState_loop, nodeStatusUpdateTime_global, TASKER_SKIP_NEVER); //60 * 60 * 1000 = 1 hour
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
      Serial.println(sE("\n\n!!!Error: Loop WDT reset! at: ") + millis());
      restartEsp(); 
    }
}