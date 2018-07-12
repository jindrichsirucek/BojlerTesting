// int i;          // integer variable 'i'
// int *p;         // pointer 'p' to an integer
// int a[];        // array 'a' of integers
// int f();        // function 'f' with return value of type integer
// int **pp;       // pointer 'pp' to a pointer to an integer
// int (*pa)[];    // pointer 'pa' to an array of integer
// int (*pf)();    // pointer 'pf' to a function with return value integer
// int *ap[];      // array 'ap' of pointers to an integer
// int *fp();      // function 'fp' which returns a pointer to an integer
// int ***ppp;     // pointer 'ppp' to a pointer to a pointer to an integer
// int (**ppa)[];  // pointer 'ppa' to a pointer to an array of integers
// int (**ppf)();  // pointer 'ppf' to a pointer to a function with return value of type integer
// int *(*pap)[];  // pointer 'pap' to an array of pointers to an integer
// int *(*pfp)();  // pointer 'pfp' to function with return value of type pointer to an integer
// int **app[];    // array of pointers 'app' that point to pointers to integer values
// int (*apa[])[]; // array of pointers 'apa' to arrays of integers
// int (*apf[])(); // array of pointers 'apf' to functions with return values of type integer
// int ***fpp();   // function 'fpp' which returns a pointer to a pointer to a pointer to an int
// int (*fpa())[]; // function 'fpa' with return value of a pointer to array of integers
// int (*fpf())(); // function 'fpf' with return value of a pointer to function which returns an integer

//(\W)GLOBAL.TEMP.sensors(\W) $1GLOBAL.TEMP.sensors$2
#include <Arduino.h>
#include <EEPROM.h>
#include "DisplayMenu.h"

OneWire oneWire(ONE_WIRE_BUS_PIN);
DallasTemperature DS18B20(&oneWire);


void runServiceMenuIfNeeded()
{
  pinMode(FLASH_BUTTON_PIN, INPUT_PULLUP);

  DEBUG_OUTPUT.println(E("Run Service_Menu(?)"));
  displayServiceLine(cE("Service menu?"));
  displayServiceMessage(E(""));
  uint16_t counter = 1000;
  while(counter--) // wait for a second to enter service menu
    if(isFlashButtonPressed()) 
      showServiceMenu();
    else
      delay(1);
}


void resetSavedTempSensorAddresses()
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:resetSavedTempSensorAddresses(): "));

  DS18B20.begin();
  uint8_t tempSensorsCount = DS18B20.getDS18Count();
  if(tempSensorsCount == 0)
  {
    ERROR_OUTPUT.println(E("!!!Error: There is NO temp sensor connected!"));
    return;
  }
  displayServiceMessage(E("RESETING TEMP"));
  
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(sE("Found temp sensors: ") + tempSensorsCount);
  uint8_t index = 0;
  while(index < tempSensorsCount)
  {
    DS18B20.getAddress(GLOBAL.TEMP.sensors[index].address, index);
    GLOBAL.TEMP.sensors[index].sensorConnected = true;
    index++;
  }

  saveTempSensorAddressesToEeprom();
}


void saveTempSensorAddressesToEeprom()
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:saveTempSensorAddressesToEeprom(): "));
  TempSensorAddressesStruct sensorAddresses;
  uint8_t *eepromListOfAddreses[] = {sensorAddresses.bojler, sensorAddresses.pipe, sensorAddresses.roomTemp, sensorAddresses.insideFlow};

  for(uint8_t s = 0; s < MAX_TEMP_SENSORS_COUNT; s++)
  {
    //Copy device address
    for(uint8_t i = 0; i < 8; i++)
      eepromListOfAddreses[s][i] = GLOBAL.TEMP.sensors[s].address[i];

    DEBUG_OUTPUT.println(sE("GLOBAL.TEMP.sensors[")+s+E("]: ") + getSensorAddressHexString(GLOBAL.TEMP.sensors[s].address) + sE(" eeprom Address: ") + getSensorAddressHexString(eepromListOfAddreses[s]));
  }
  DEBUG_OUTPUT.println(E("What is going to be saved to memmory: "));
  DEBUG_OUTPUT.println(sE("bojler: ") + getSensorAddressHexString(sensorAddresses.bojler));
  DEBUG_OUTPUT.println(sE("pipe: ") + getSensorAddressHexString(sensorAddresses.pipe));
  DEBUG_OUTPUT.println(sE("roomTemp: ") + getSensorAddressHexString(sensorAddresses.roomTemp));
  DEBUG_OUTPUT.println(sE("insideFlow: ") + getSensorAddressHexString(sensorAddresses.insideFlow));

  saveTempSensorsStructToEeprom(sensorAddresses);
  logNewNodeState(E("Settings: New temp addresses saved"));
  loadTempSensorAddressesFromEeprom();
}


void loadTempSensorAddressesFromEeprom()
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:loadTempSensorAddressesFromEeprom(): "));
  // TempSensorAddressesStruct sensorAddresses;
  // loadSettingsStructFromEeprom(sensorAddresses);
  const char *sensorNamesList[MAX_TEMP_SENSORS_COUNT] = {"bojler","pipe","roomTemp","insideFlow"};
  uint8_t *eepromListOfAddreses[MAX_TEMP_SENSORS_COUNT] = {eepromSettings_global.tempSensorAddresses.bojler, eepromSettings_global.tempSensorAddresses.pipe, eepromSettings_global.tempSensorAddresses.roomTemp, eepromSettings_global.tempSensorAddresses.insideFlow};

  bool resetSavedAddresses = true;
  for(uint8_t s = 0; s < MAX_TEMP_SENSORS_COUNT; s++)
  {
    //Copy device address
    for(uint8_t i = 0; i < 8; i++) GLOBAL.TEMP.sensors[s].address[i] = eepromListOfAddreses[s][i];
    
    GLOBAL.TEMP.sensors[s].sensorName = sensorNamesList[s];
    if(isTempSensorAddressValid(GLOBAL.TEMP.sensors[s]))
    {
      resetSavedAddresses = false;
      if(isTempSensorConnectedAndPrintInfo(GLOBAL.TEMP.sensors[s]))
        GLOBAL.TEMP.sensors[s].sensorConnected = true;
    }
    yield_debug();
  }
  
  if(resetSavedAddresses)
  {
    ERROR_OUTPUT.println(E("!!!Error: There are no valid temp sensor addresses saved in eeprom! Reloading sensor addresses by index"));
    resetSavedTempSensorAddresses();
  }
}


void updateTempSensorAddressByNameFromHexString(const char *sensorName, const String hexString)
{
  // DEBUG_OUTPUT.println("What is NOW State: ");
  // DEBUG_OUTPUT.println((String)"bojler: " + getSensorAddressHexString(GLOBAL.TEMP.sensors[BOJLER].address) + " real NAME: " + GLOBAL.TEMP.sensors[BOJLER].sensorName);
  // DEBUG_OUTPUT.println((String)"pipe: " + getSensorAddressHexString(GLOBAL.TEMP.sensors[PIPE].address) + " real NAME: " + GLOBAL.TEMP.sensors[PIPE].sensorName);
  // DEBUG_OUTPUT.println((String)"roomTemp: " + getSensorAddressHexString(GLOBAL.TEMP.sensors[ROOM_TEMP].address) + " real NAME: " + GLOBAL.TEMP.sensors[ROOM_TEMP].sensorName);
  // DEBUG_OUTPUT.println((String)"insideFlow: " + getSensorAddressHexString(GLOBAL.TEMP.sensors[INSIDE_FLOW].address) + " real NAME: " + GLOBAL.TEMP.sensors[INSIDE_FLOW].sensorName);
  
  DEBUG_OUTPUT.printf(cE("Settings: Updating address for sensor name: %s, new hex address: %s\n"), sensorName, hexString.c_str());

  if(strncmp(sensorName, cE("bojler"), 16) == 0)
    setSensorAddressFromHexString(GLOBAL.TEMP.sensors[BOJLER].address, hexString);
  else 
  if(strncmp(sensorName, cE("pipe"), 16) == 0)
    setSensorAddressFromHexString(GLOBAL.TEMP.sensors[PIPE].address, hexString);
  else 
  if(strncmp(sensorName, cE("roomTemp"), 16) == 0)
    setSensorAddressFromHexString(GLOBAL.TEMP.sensors[ROOM_TEMP].address, hexString);
  else 
  if(strncmp(sensorName, cE("insideFlow"), 16) == 0)
    setSensorAddressFromHexString(GLOBAL.TEMP.sensors[INSIDE_FLOW].address, hexString);
  else
    DEBUG_OUTPUT.printf(cE("!!!Error: Temp sensor of name: %s, is not found!!:\n"), sensorName);

}


// void loadNodeSettingsAfterBoot()
// {
//   // loadTempSensorsAddressesSetingsFromEeprom(GLOBAL.TEMP.sensors);
// }


String stringifyTempSensorAddressesStruct()
{
  String settingsString;
  settingsString.reserve(250);
  settingsString = E("");

  settingsString += cE("&bojlerAddress=") + getSensorAddressHexString(GLOBAL.TEMP.sensors[BOJLER].address);
  settingsString += cE("&pipeAddress=") + getSensorAddressHexString(GLOBAL.TEMP.sensors[PIPE].address);
  settingsString += cE("&roomTempAddress=") + getSensorAddressHexString(GLOBAL.TEMP.sensors[ROOM_TEMP].address);
  settingsString += cE("&insideFlowAddress=") + getSensorAddressHexString(GLOBAL.TEMP.sensors[INSIDE_FLOW].address);

  DS18B20.begin();
  uint8_t tempSensorsCount = DS18B20.getDS18Count();
  settingsString += cE("&tempSensorsCount=") + tempSensorsCount;
  settingsString += cE("&allTempSensorsInfo=");
  while(tempSensorsCount--)
  {
    DeviceAddress tempAddress;
    DS18B20.getAddress(tempAddress, tempSensorsCount);
    settingsString += tempSensorsCount+cE("-") + getSensorAddressHexString(tempAddress) + cE("-") + DS18B20.getTempCByIndex(tempSensorsCount) + cE("-") ;
  }

  return settingsString;
}


void showServiceMenu()
{
  displayServiceLine(cE("Service menu:"));
  turnNotificationLedOn();

  DisplayMenu serviceMenu;
  serviceMenu.addEntry(restartEsp, ("CANCEL?"));
  serviceMenu.addEntry(resetSavedTempSensorAddresses, ("DEL SENSOR ADDR?"), PRESELECTED_MENU_ENTRY);
  serviceMenu.addEntry(displayRSSI, ("Measure RSSI?"));
  serviceMenu.addEntry(formatFileSystem, ("FORMAT SPIFFS?"));
  serviceMenu.addEntry(resetAllWifiSettings, ("RESET WIFI SETS?"));

  displayServiceMessage(serviceMenu.getSelectedEntryName());
  DEBUG_OUTPUT.println(serviceMenu.getSelectedEntryName());

  while(isFlashButtonPressed()) {delay(1);}
  delay(100); //debounce
  DEBUG_OUTPUT.println(E("Press SHORT 'flash' button to move in menu.. \nPress LONG 'flash' button to select.. \nOr press 'reset' button to Cancel operation"));

  uint8_t holdCounter = 0;
  while(true)
  {
    //Awaiting user input - button not pressed
    while(isFlashButtonPressed() == false){yield_debug();}
    //As long as user holds button
    while(isFlashButtonPressed())
    {
      turnNotificationLedOff();
      //Long press - USER SELECTED menu  - activate selected function
      if(++holdCounter > 250)
      {
        blinkNotificationLed(SHORT_BLINK);
        blinkNotificationLed(SHORT_BLINK);
        blinkNotificationLed(SHORT_BLINK);
        blinkNotificationLed(SHORT_BLINK);
        serviceMenu.runSelectedMenuEntry();
        return;
      }
      delay(2);
    }
    //Short press - change menu
    if(holdCounter)
    {
      turnNotificationLedOn();
      displayServiceMessage(serviceMenu.move());
      DEBUG_OUTPUT.println(sE("Menu:") + serviceMenu.getSelectedEntryName());
      holdCounter = 0;
    }
  }
}


void resetAllWifiSettings()
{
  displayServiceMessage(E("RESETTING WIFI!"));
  WiFi.softAPdisconnect(true);
  WiFi.disconnect(true);
  ESP.eraseConfig();
}

void formatFileSystem()
{
  displayServiceMessage(E("FORMATING FS!"));
  DEBUG_OUTPUT.print(E("Formating File System.."));
  displayServiceMessage(SPIFFS.format() ? E("Done!"):E("!!!Error ocured."));
  ESP.restart();
}
////////////////////////////////////////////////////////
//  LOAD / SAVE / INITIALIZE   SETTINGS
////////////////////////////////////////////////////////
void setSensorAddressFromHexString(DeviceAddress address, const String hexString)
{
  // DEBUG_OUTPUT.println((String)"Before change: " + getSensorAddressHexString(address));
  for (uint8_t i = 0; i < 8; i++)
  {
    address[i] = (uint8_t)strtoul(hexString.substring(i*2, i*2+2).c_str(), NULL, 16);
  }

  // DEBUG_OUTPUT.println((String)"After  change: " + getSensorAddressHexString(address));
  // DEBUG_OUTPUT.println((String)"Should be:     " + hexString);
}


void loadSettingsStructFromEeprom(struct TempSensorAddressesStruct &tempSensorsAddressesToLoad)
{
  EepromSettingsStruct eepromSettings;

  EEPROM.begin(EEPROM_ALOCATION_SIZE);
  int loadedBytes = EEPROMAnythingRead(0, reinterpret_cast<char*>(&eepromSettings), sizeof(eepromSettings));
  EEPROM.end();

  tempSensorsAddressesToLoad = eepromSettings.tempSensorAddresses;

  if(MAIN_DEBUG) DEBUG_OUTPUT.println(sE("From EEPROM loaded bytes: ") + loadedBytes);
}


void saveTempSensorsStructToEeprom(struct TempSensorAddressesStruct &tempSensorsAddressesToSave)
{
  eepromSettings_global.tempSensorAddresses = tempSensorsAddressesToSave;

  EEPROM.begin(EEPROM_ALOCATION_SIZE);//Size of alocated space
  // write a 0 to all 512 bytes of the EEPROM
  for (int i = 0; i < EEPROM_ALOCATION_SIZE; i++)
    EEPROM.write(i, 0);
  EEPROM.end();  


  EEPROM.begin(EEPROM_ALOCATION_SIZE);//Size of alocated space
  int loadedBytes = EEPROMAnythingWrite(0, reinterpret_cast<char*>(&eepromSettings_global), sizeof(eepromSettings_global));
  EEPROM.end();

  if(MAIN_DEBUG) DEBUG_OUTPUT.println(sE("To EEPROM Saved bytes: ") + loadedBytes);
}



void saveGlobalSettingsStructToEeprom()
{
  //CLEARING EEPROM - write a 0 to all 512 bytes of the EEPROM
  EEPROM.begin(EEPROM_ALOCATION_SIZE);//Size of alocated space
  for (int i = 0; i < EEPROM_ALOCATION_SIZE; i++)
    EEPROM.write(i, 0);
  EEPROM.end();  


  EEPROM.begin(EEPROM_ALOCATION_SIZE);//Size of alocated space
  int savedBytes = EEPROMAnythingWrite(0, reinterpret_cast<char*>(&eepromSettings_global), sizeof(eepromSettings_global));
  EEPROM.end();

  if(MAIN_DEBUG) DEBUG_OUTPUT.println(sE("To EEPROM Saved bytes: ") + savedBytes);
}


void loadGlobalSettingsStructFromEeprom()
{
  EEPROM.begin(EEPROM_ALOCATION_SIZE);
  int loadedBytes = EEPROMAnythingRead(0, reinterpret_cast<char*>(&eepromSettings_global), sizeof(eepromSettings_global));
  EEPROM.end();

  if(MAIN_DEBUG) DEBUG_OUTPUT.println(sE("From EEPROM loaded bytes: ") + loadedBytes);
}


////////////////////////////////////////////////////////
//  PRINTING / STRINGIFYING SENSOR ADDRESSES
////////////////////////////////////////////////////////

void stringifySensorAddressToHexString(DeviceAddress deviceAddress, String &hexString)
{
  hexString = getSensorAddressHexString(deviceAddress);
}

String getSensorAddressHexString(const DeviceAddress deviceAddress)
{
  char buf[20];
  sprintf(buf,"%02X%02X%02X%02X%02X%02X%02X%02X",deviceAddress[0],deviceAddress[1],deviceAddress[2],deviceAddress[3],deviceAddress[4],deviceAddress[5],deviceAddress[6],deviceAddress[7]);
  return (String)buf;
}


// function to print a device address
void printAddressHex(const DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) 
      DEBUG_OUTPUT.print(E("0"));
    DEBUG_OUTPUT.print(deviceAddress[i], HEX);
  }
  // DEBUG_OUTPUT.println();
}

void printAddressDec(const DeviceAddress deviceAddress)
{
  String address = "";
  for (uint8_t i = 0; i < 8; i++)
  {
       // zero pad the address if necessary
       // if (deviceAddress[i] < 16) if(TEMPERATURE_DEBUG) DEBUG_OUTPUT.print("0");
    DEBUG_OUTPUT.print(deviceAddress[i], DEC);
    DEBUG_OUTPUT.print(" ");
  }
  // if(TEMPERATURE_DEBUG) DEBUG_OUTPUT.println("");
}

// bool validateTempSensorAddress(const DeviceAddress deviceAddress, uint8_t index) {  char indexString[2];  itoa(index, indexString, 2);  return validateTempSensorAddress(deviceAddress, indexString);}
bool isTempSensorAddressValid(struct TempSensorStruct sensor)
{
  if (OneWire::crc8(sensor.address, 7) != sensor.address[7] || sensor.address[0] == 0)
  {
    if(TEMPERATURE_DEBUG) {DEBUG_OUTPUT.print(E("!!WARNING: Given address: ")); printAddressHex(sensor.address); DEBUG_OUTPUT.print(E(" of sensor: ")); DEBUG_OUTPUT.print(sensor.sensorName); DEBUG_OUTPUT.println(E(" is not valid!"));}
    return false;
  }
  return true;
}


bool isTempSensorConnectedAndPrintInfo(struct TempSensorStruct sensor)
{
  bool sensorConnected = DS18B20.isConnected(sensor.address);
  if(!sensorConnected)
    DEBUG_OUTPUT.print(E("!!!ERROR: "));
  if(!sensorConnected || TEMPERATURE_DEBUG){ DEBUG_OUTPUT.print(sensor.sensorName); DEBUG_OUTPUT.print((String)E(" sensor of address: ") + getSensorAddressHexString(sensor.address)); }

  if(!sensorConnected)
  {
    DEBUG_OUTPUT.println(E(" is not connected!"));
    return false;
  }

  if(TEMPERATURE_DEBUG)
  {
    DEBUG_OUTPUT.print(E(" Current temp: "));
    DS18B20.requestTemperaturesByAddress(sensor.address);
    DEBUG_OUTPUT.print(DS18B20.getTempC(sensor.address));
    DEBUG_OUTPUT.println(E("°C"));
  }
  return true;
}

////////////////////////////////////////////////////////
//  WORK WITH EEPROM
////////////////////////////////////////////////////////

// Write any data structure or variable to EEPROM
int EEPROMAnythingWrite(int startingPosition, char *dataCharToSave, int lengthOfDataInBytes)
{
  for (int i = 0; i < lengthOfDataInBytes; i++)
  {
    EEPROM.write(startingPosition + i, *dataCharToSave);
    dataCharToSave++;
  }
  return startingPosition + lengthOfDataInBytes;
}
 
// Read any data structure or variable from EEPROM
int EEPROMAnythingRead(int startingPosition, char *dataCharToSave, int lengthOfDataInBytes)
{
  for (int i = 0; i < lengthOfDataInBytes; i++)
  {
    *dataCharToSave = EEPROM.read(startingPosition + i);
    dataCharToSave++;
  }
  return startingPosition + lengthOfDataInBytes;
}
