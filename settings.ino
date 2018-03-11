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

#include <Arduino.h>
#include <EEPROM.h>
#include "DisplayMenu.h"

OneWire oneWire(ONE_WIRE_BUS_PIN);
DallasTemperature DS18B20(&oneWire);

struct TempSensorAddressesStruct {
  DeviceAddress bojler;
  DeviceAddress pipe;
  DeviceAddress roomTemp;
  DeviceAddress insideFlow;
};
TempSensorAddressesStruct tempSensorsAddresses;

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
  showServiceMessage(E("RESETING SETTINGS"));
  
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(sE("Found temp sensors: ") + tempSensorsCount);
  uint8_t index = 0;
  while(index < tempSensorsCount)
  {
    DS18B20.getAddress(tempSensors[index].address, index);
    tempSensors[index].sensorConnected = true;
    index++;
  }

  //TODO
  // Serial.println("clearing eeprom");
  // for (int i = 0; i < 96; ++i) { EEPROM.write(i, 0); }
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
      eepromListOfAddreses[s][i] = tempSensors[s].address[i];

    DEBUG_OUTPUT.println(sE("tempSensors[")+s+E("]: ") + getSensorAddressHexString(tempSensors[s].address) + sE(" eeprom Address: ") + getSensorAddressHexString(eepromListOfAddreses[s]));
  }
  DEBUG_OUTPUT.println(E("What is going to be saved to memmory: "));
  DEBUG_OUTPUT.println(sE("bojler: ") + getSensorAddressHexString(tempSensorsAddresses.bojler));
  DEBUG_OUTPUT.println(sE("pipe: ") + getSensorAddressHexString(tempSensorsAddresses.pipe));
  DEBUG_OUTPUT.println(sE("roomTemp: ") + getSensorAddressHexString(tempSensorsAddresses.roomTemp));
  DEBUG_OUTPUT.println(sE("insideFlow: ") + getSensorAddressHexString(tempSensorsAddresses.insideFlow));

  saveTempSensorsStructToEeprom(sensorAddresses);
  loadTempSensorAddressesFromEeprom();
}


void loadTempSensorAddressesFromEeprom()
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:loadTempSensorAddressesFromEeprom(): "));
  TempSensorAddressesStruct sensorAddresses;
  loadTempSensorsStructFromEeprom(sensorAddresses);
  const char *sensorNamesList[MAX_TEMP_SENSORS_COUNT] = {"bojler","pipe","roomTemp","insideFlow"};
  uint8_t *eepromListOfAddreses[MAX_TEMP_SENSORS_COUNT] = {sensorAddresses.bojler, sensorAddresses.pipe, sensorAddresses.roomTemp, sensorAddresses.insideFlow};

  bool resetSavedAddresses = true;
  for(uint8_t s = 0; s < MAX_TEMP_SENSORS_COUNT; s++)
  {
    //Copy device address
    for(uint8_t i = 0; i < 8; i++) tempSensors[s].address[i] = eepromListOfAddreses[s][i];
    
    tempSensors[s].sensorName = sensorNamesList[s];
    if(isTempSensorAddressValid(tempSensors[s]))
    {
      resetSavedAddresses = false;
      if(isTempSensorConnectedAndPrintInfo(tempSensors[s]))
        tempSensors[s].sensorConnected = true;
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
  // DEBUG_OUTPUT.println((String)"bojler: " + getSensorAddressHexString(tempSensors[BOJLER].address) + " real NAME: " + tempSensors[BOJLER].sensorName);
  // DEBUG_OUTPUT.println((String)"pipe: " + getSensorAddressHexString(tempSensors[PIPE].address) + " real NAME: " + tempSensors[PIPE].sensorName);
  // DEBUG_OUTPUT.println((String)"roomTemp: " + getSensorAddressHexString(tempSensors[ROOM_TEMP].address) + " real NAME: " + tempSensors[ROOM_TEMP].sensorName);
  // DEBUG_OUTPUT.println((String)"insideFlow: " + getSensorAddressHexString(tempSensors[INSIDE_FLOW].address) + " real NAME: " + tempSensors[INSIDE_FLOW].sensorName);
  
  DEBUG_OUTPUT.printf(cE("Settings: Updating address for sensor name: %s, new hex address: %s\n"), sensorName, hexString.c_str());

  if(strncmp(sensorName, cE("bojler"), 16) == 0)
    setSensorAddressFromHexString(tempSensors[BOJLER].address, hexString);
  else 
  if(strncmp(sensorName, cE("pipe"), 16) == 0)
    setSensorAddressFromHexString(tempSensors[PIPE].address, hexString);
  else 
  if(strncmp(sensorName, cE("roomTemp"), 16) == 0)
    setSensorAddressFromHexString(tempSensors[ROOM_TEMP].address, hexString);
  else 
  if(strncmp(sensorName, cE("insideFlow"), 16) == 0)
    setSensorAddressFromHexString(tempSensors[INSIDE_FLOW].address, hexString);
  else
    DEBUG_OUTPUT.printf(cE("!!!Error: Temp sensor of name: %s, is not found!!:\n"), sensorName);

}




void loadNodeSettingsAfterBoot()
{
  // loadTempSensorsAddressesSetingsFromEeprom(tempSensors);
}

String stringifyTempSensorAddressesStruct()
{
  String settingsString;
  settingsString.reserve(250);
  settingsString = E("");

  settingsString += cE("&bojlerAddress=") + getSensorAddressHexString(tempSensorsAddresses.bojler);
  settingsString += cE("&pipeAddress=") + getSensorAddressHexString(tempSensorsAddresses.pipe);
  settingsString += cE("&roomTempAddress=") + getSensorAddressHexString(tempSensorsAddresses.roomTemp);
  settingsString += cE("&insideFlowAddress=") + getSensorAddressHexString(tempSensorsAddresses.insideFlow);

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
  turnNotificationLedOn();

  DisplayMenu serviceMenu;
  serviceMenu.addEntry(restartEsp, cE("CANCEL?"));
  serviceMenu.addEntry(resetSavedTempSensorAddresses, cE("DEL SENSOR ADDR?"), PRESELECTED_MENU_ENTRY);
  serviceMenu.addEntry(formatFileSystem, cE("FORMAT SPIFFS?"));
  serviceMenu.addEntry(resetAllWifiSettings, cE("RESET WIFI SETS?"));

  showServiceMessage(serviceMenu.getSelectedEntryName());
  
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
      showServiceMessage(serviceMenu.move());
      holdCounter = 0;
    }
  }
}


void resetAllWifiSettings()
{
  
  showServiceMessage(E("RESETTING WIFI!"));
  WiFi.softAPdisconnect(true);
  WiFi.disconnect(true);
  ESP.eraseConfig();
}

void formatFileSystem()
{
  DEBUG_OUTPUT.print(E("Formating File System.."));
  showServiceMessage(E("FORMATING DISK!"));
  showServiceMessage(SPIFFS.format() ? E("Done!"):E("!!!Error ocured."));
  ESP.reset();
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


void loadTempSensorsStructFromEeprom(struct TempSensorAddressesStruct &tempSensorsAddressesToLoad)
{
  EEPROM.begin(255);
  int loadedBytes = EEPROMAnythingRead(0, reinterpret_cast<char*>(&tempSensorsAddressesToLoad), sizeof(tempSensorsAddressesToLoad));
  EEPROM.end();

  if(MAIN_DEBUG) DEBUG_OUTPUT.println(sE("From EEPROM loaded bytes: ") + loadedBytes);
}


void saveTempSensorsStructToEeprom(struct TempSensorAddressesStruct &tempSensorsAddressesToSave)
{
  uint8_t memmorySize = 255;

  EEPROM.begin(memmorySize);//Size of alocated space
  // write a 0 to all 512 bytes of the EEPROM
  for (int i = 0; i < memmorySize; i++)
    EEPROM.write(i, 0);
  EEPROM.end();  


  EEPROM.begin(memmorySize);//Size of alocated space
  int loadedBytes = EEPROMAnythingWrite(0, reinterpret_cast<char*>(&tempSensorsAddressesToSave), sizeof(tempSensorsAddressesToSave));
  EEPROM.end();

  if(MAIN_DEBUG) DEBUG_OUTPUT.println(sE("To EEPROM Saved bytes: ") + loadedBytes);
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
    if (deviceAddress[i] < 16) DEBUG_OUTPUT.print(E("0"));
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
