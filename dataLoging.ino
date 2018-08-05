#include "FS.h"

// Chip ID
// IP adresa
// nodeName

uint16_t getNumberOfNewestLogFile(uint16_t newestFileLogNumber=0);
uint16_t getNumberOfOldestLogFile(uint16_t minimumFileIndex=0);


struct SensorDataStructure {
   String time;
   String fireEventName;
   String bojlerTemp;
   String pipeTemp;
   String roomTemp;
   String insideFlowTemp;
   String current;
   String waterFlow;
   String test1Value;
   String test2Value;
   String test3Value;
   String objectAskingForResponse;
   String heatingState;
   String controlState;
   String nodeInfoString;
};
SensorDataStructure nodeStateLeadingValues_global;

bool logNewNodeState(String fireEventName)
{
  if (MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:logNewNodeState(String fireEventName): \"") + fireEventName + "\"");

  if(!isWaterFlowingLastStateFlag_global && DISPLAY_LOG_MESSAGES)
    displayServiceMessage(fireEventName);
  
  setPendingEventToSend(true);

  SensorDataStructure newState;
  newState.time = getNowTimeDateString();//.c_str()
  newState.bojlerTemp = (isTemperatureCorrectMeasurment(GLOBAL.TEMP.sensors[BOJLER].temp)) ? (String)GLOBAL.TEMP.sensors[BOJLER].temp : E("");
  newState.pipeTemp = (isTemperatureCorrectMeasurment(GLOBAL.TEMP.sensors[PIPE].temp)) ? (String)GLOBAL.TEMP.sensors[PIPE].temp : E("");
  newState.roomTemp = (isTemperatureCorrectMeasurment(GLOBAL.TEMP.sensors[ROOM_TEMP].temp)) ? (String)GLOBAL.TEMP.sensors[ROOM_TEMP].temp : E("");
  newState.insideFlowTemp = (isTemperatureCorrectMeasurment(GLOBAL.TEMP.sensors[INSIDE_FLOW].temp)) ? (String)GLOBAL.TEMP.sensors[INSIDE_FLOW].temp : E("");

  newState.current = lastCurrentMeasurmentText_global; //(lastElectricCurrentState_global != false) ? E("1024") : E("");
  newState.waterFlow = (waterFlowSensorCount_ISR_global != 0) ? (String)convertWaterFlowSensorImpulsesToLitres(waterFlowSensorCount_ISR_global) : E("");
  
  newState.test1Value = "";//getNodeVccString();
  newState.test2Value = GLOBAL.nodeInBootSequence? "" : getSpareWaterInLittres();
  newState.test3Value = isElectricityConnected()==1 ? E("1") : E("0");

  newState.fireEventName = fireEventName;
  newState.controlState = getTempControleStyleStringName();
  newState.heatingState = (isBoilerHeatingOn()) ? E("1") : E("0");
  newState.objectAskingForResponse = "";
  newState.nodeInfoString = getSystemStateInfo();

  return saveNewNodeState(newState);
}



bool saveNewNodeState(struct SensorDataStructure &newState)
{
  if(DATA_LOGGING_DEBUG) DEBUG_OUTPUT.printf(cE("getNumberOfNewestLogFile(): %d,\nlogFileRowsCount_global: %d \n"), getNumberOfNewestLogFile(), logFileRowsCount_global);

  if(getNumberOfNewestLogFile() == 0 && logFileRowsCount_global == 0) //žádné soubory v pořadí
  {
    saveNewNodeStateIntoTempString(newState);
    logFileRowsCount_global++;
    
    if(DATA_LOGGING_DEBUG) DEBUG_OUTPUT.println(sE("Last row saved only in RAM: ") + lastNodeStateTempString_global);
    return true;
  }
  //Prints previous temp string into file (than will save current newState to temp string)
  bool success = flushTemporaryStringNodeStateIntoCsvFile();
  if(success)
  {
    saveNewNodeStateIntoTempString(newState);
    logFileRowsCount_global++;
  }  
  return success;
}


void saveNewNodeStateIntoTempString(struct SensorDataStructure &newState)
{
  lastNodeStateTempString_global = "";
  const char* delimiter = ";";

  String* newInputDataPointers[] = {&newState.time, &newState.fireEventName, &newState.bojlerTemp, &newState.pipeTemp, &newState.roomTemp, &newState.insideFlowTemp, &newState.current, &newState.waterFlow, &newState.test1Value, &newState.test2Value, &newState.test3Value, &newState.objectAskingForResponse, &newState.heatingState, &newState.controlState, &newState.nodeInfoString};
  String* leadingInputDataPointers[] = {&nodeStateLeadingValues_global.time, &nodeStateLeadingValues_global.fireEventName, &nodeStateLeadingValues_global.bojlerTemp, &nodeStateLeadingValues_global.pipeTemp, &nodeStateLeadingValues_global.roomTemp, &nodeStateLeadingValues_global.insideFlowTemp, &nodeStateLeadingValues_global.current, &nodeStateLeadingValues_global.waterFlow, &nodeStateLeadingValues_global.test1Value, &nodeStateLeadingValues_global.test2Value, &nodeStateLeadingValues_global.test3Value, &nodeStateLeadingValues_global.objectAskingForResponse, &nodeStateLeadingValues_global.heatingState, &nodeStateLeadingValues_global.controlState, &nodeStateLeadingValues_global.nodeInfoString};
  
  for(uint8_t i = 0; i < SIZE_OF_LOCAL_ARRAY(newInputDataPointers); i++)
  {
    // DEBUG_OUTPUT.printf("%d:\n", i);
    // DEBUG_OUTPUT.printf("newValue:");
    // DEBUG_OUTPUT.println(*newInputDataPointers[i]);
    // DEBUG_OUTPUT.printf("oldValue:");
    // DEBUG_OUTPUT.println(*leadingInputDataPointers[i]);

    //If empty value, if first line of file, if new value is different from leading one THAN save current value ELSE save "#"
    if(*newInputDataPointers[i] == "" || logFileRowsCount_global == 0 || *newInputDataPointers[i] != *leadingInputDataPointers[i])
    {
      lastNodeStateTempString_global += *newInputDataPointers[i];
      *leadingInputDataPointers[i] = *newInputDataPointers[i];
    }
    else
    {
      // DEBUG_OUTPUT.printf("#define copyValue\n");
      lastNodeStateTempString_global += E("#");
    }

    if(i+1 < SIZE_OF_LOCAL_ARRAY(newInputDataPointers))
      lastNodeStateTempString_global += delimiter;

    // DEBUG_OUTPUT.print("lastNodeStateTempString_global:");
    // DEBUG_OUTPUT.println(lastNodeStateTempString_global);
  }
}



bool flushTemporaryStringNodeStateIntoCsvFile()
{
  if(lastNodeStateTempString_global.length() == 0)
  {
    if (DATA_LOGGING_DEBUG) ERROR_OUTPUT.println(E("Empty lastNodeStateTempString_global! NOT saving to file:"));
    return true;
  }

  uint32_t freeSpace = getFreeSpaceSPIFFS();
  uint8_t attempt = 10;
  
  while(attempt--)
  {
    File f = openCurentLogFileForApending();
    if(f)
    {
      if(freeSpace < 40000)
      {
        if (SHOW_ERROR_DEBUG_MESSAGES) ERROR_OUTPUT.printf(cE("!!!Error: There is NOT enough space! Free bytes: %d\n"), freeSpace);
        return false;
      }
      // if (DATA_LOGGING_DEBUG) DEBUG_OUTPUT.println(newState.time + delimiter + newState.bojlerTemp + delimiter + newState.testValue + delimiter + newState.pipeTemp + delimiter + newState.waterFlow + delimiter + newState.current + delimiter + newState.fireEventName + delimiter + newState.controlState + delimiter + newState.heatingState);
      uint32_t previousFileSize = f.size();
      
      f.println(lastNodeStateTempString_global);

      if(f.size() > previousFileSize)
      {
        resetflowCount();
        if (DATA_LOGGING_DEBUG) DEBUG_OUTPUT.printf(cE("New line saved into file: %s\n"),f.name());
        f.close();
        if (DATA_LOGGING_DEBUG) DEBUG_OUTPUT.println(getContentOfFile(getLogFileNameByLogNumber(curentLogNumber_global)));        
        
        return true;
      }
    }
  }
  if (SHOW_ERROR_DEBUG_MESSAGES) ERROR_OUTPUT.printf(cE("!!!Error: Problem saving data to Log file! Maybe NOT enough space? FS free bytes: %d\n"), freeSpace);
  formatSpiffs();
  logNewNodeState(E("!!!Error: FormatedSpiffs - Error during saving a file."));
  return false;
}



bool logWarningMessage(String fireEventName)
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:logNewNodeState(String fireEventName): ") + fireEventName);
  displayServiceMessage(fireEventName);

  SensorDataStructure newState;
  newState.time = getNowTimeDateString();
  newState.fireEventName = fireEventName;

  return saveNewNodeState(newState);
}


bool logNewErrorState(String errorMessage)
{
  if (MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:logNewErrorState(String errorMessage): ") + errorMessage);
  displayServiceMessage(errorMessage);
  errorMessage =  sE("&quickEvent=") + URLEncode(errorMessage);
  return sendGetParamsWithPostFile(errorMessage, RemoteDebug.getLastRuntimeLogAsFile());
}


// int a = 1;
// int *b = &a;
// *b += 1;
//now a == 2 (and b's value is a pointer to the location of a in memory)


bool sendAllLogFiles()
{
  if (MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:sendAllLogFiles()")); uint8_t curentDebuggerLevel = getDebuggerSpacesCount();

  if(!isWifiConnected())
  {
    if(MAIN_DEBUG) { DEBUG_OUTPUT.println(E("Data not send: WiFi NOT connected! Returning..")); }
    return false;
  }

  uint16_t logFileIndex = getNumberOfOldestLogFile();
  if (DATA_LOGGING_DEBUG) DEBUG_OUTPUT.printf(cE("logFileIndex: %d - getNumberOfOldestLogFile()\n"), logFileIndex);

  if(logFileIndex == 0 && logFileRowsCount_global == 0)
  {
    if(MAIN_DEBUG) { DEBUG_OUTPUT.println(E("!!Warning: There is no log file to send.")); }
    return false;
  }

  bool success = false;
  //node state ssaved only in temp string
  if(logFileIndex == 0)
  {
    success = sendNewNodeStateByPostString(lastNodeStateTempString_global);
  }
  else//Actual files to send
  {
    //First save last state form Temp String
    if (DATA_LOGGING_DEBUG) DEBUG_OUTPUT.printf(cE("Saving lastNodeStateTempString_global into newest logfile(%d) before uploading\n"), logFileIndex);
    flushTemporaryStringNodeStateIntoCsvFile();
    lastNodeStateTempString_global = "";

    while (logFileIndex != 0)
    {
      if (MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug(curentDebuggerLevel) + E("F:LOOP sendAllLogFiles(): ") + getLogFileNameByLogNumber(logFileIndex));

      File f = openLogFileForReadingByNumber(logFileIndex);
      if(f)
      {
        const char* fileName = f.name();
        uint8_t attempts = 3+1;
        while(--attempts)
        {
          bool dataSentOk = uploadLogFile(f);
          if(dataSentOk)
          {
            if(isFileReceivedOk())
            {
              deleteFileByName(fileName);
              success = true;
              break;
            }
            else
            {
              if(SHOW_ERROR_DEBUG_MESSAGES) ERROR_OUTPUT.printf(cE("!!Warning: File was NOT procesed by google script. File '%s' size %d\n"), f.name(), f.size());
              if(attempts == 1)//Last attempt
              {
                if(SHOW_ERROR_DEBUG_MESSAGES) ERROR_OUTPUT.printf(cE("!!!Error: File was NOT procesed by google script repeatedly - Deleting file. File '%s' size %d\n"), f.name(), f.size());
                deleteFileByName(fileName);
                break;
              }
            }
          }
          else
          DEBUG_OUTPUT.printf(cE("!!Warning: File was NOT sent. File '%s' size %d. Repeating.. \n"), f.name(), f.size());
        }
        if(attempts == 0)
        {
          success = false;
          if (SHOW_ERROR_DEBUG_MESSAGES) ERROR_OUTPUT.printf(cE("!!!Error: File was NOT possible to send repeatedly. File '%s' size %d\n"), f.name(), f.size());
          wifiConnectBySavedCredentials();
        }
      }    
      logFileIndex = getNumberOfOldestLogFile(logFileIndex);
    }
  }
  curentLogNumber_global = 0;
  logFileRowsCount_global = 0;//reset rows counter
  setPendingEventToSend(false);
  return success;
}



////////////////////////////////////////////////////////
//  WORK WITH FILES
////////////////////////////////////////////////////////
File createNewEventLogFile(uint16_t newFileNumber=1)
{
  if (MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:createNewEventLogFile(uint16_t newFileNumber): ") + newFileNumber);  
  if (DATA_LOGGING_DEBUG) DEBUG_OUTPUT.printf(cE("Creating NEW Log file: '%s'. Free space: %dkB\n"),getLogFileNameByLogNumber(newFileNumber).c_str(), (getFreeSpaceSPIFFS()/1000));

  File f = openLogFileByNumber(newFileNumber, "w");
  
  if(f)
  {
    if (DATA_LOGGING_DEBUG) DEBUG_OUTPUT.printf(cE("Adding a header to file: '%s'.\n"),getLogFileNameByLogNumber(newFileNumber).c_str());
    f.println(E("time;fireEventName;bojlerTemp;pipeTemp;roomTemp;insideFlowTemp;current;waterFlow;test1Value;test2Value;test3Value;objectAskingForResponse;heatingState;controlState;nodeInfoString"));
    curentLogNumber_global = newFileNumber;
    logFileRowsCount_global = 0;//reset rows counter
  }
  else
    if (SHOW_ERROR_DEBUG_MESSAGES) DEBUG_OUTPUT.printf(cE("!!!Error: New file is NOT created corectly. File '%s' size %d\n"), f.name(), f.size());

  return f;
}
////////////////////////////////////////////////////////
//////////////////////////   DELETE   //////////////////
////////////////////////////////////////////////////////
bool deleteEventLogFileByNumber(uint16_t logNumber)
{
  if (MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:deleteEventLogFileByNumber(uint16_t logNumber): '") + getLogFileNameByLogNumber(logNumber).c_str() + "'");
  return SPIFFS.remove(getLogFileNameByLogNumber(logNumber));
}


bool deleteFileByName(const char* fileName)
{
  if (MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:deleteFileByName(const char* fileName): '") + fileName + E("'"));
  return SPIFFS.remove(fileName);
}


bool deleteCurentEventLogFile()
{
  if (MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:deleteCurentEventLogFile()"));
  return SPIFFS.remove(getLogFileNameByLogNumber(curentLogNumber_global));
}

////////////////////////////////////////////////////////
///////////////////////   OPEN   ///////////////////////
////////////////////////////////////////////////////////
File openLogFileForReadingByNumber(uint16_t logNumber)
{
  if (MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:openLogFileForReadingByNumber(uint16_t logNumber) : ") + logNumber);
  
  File f = openLogFileByNumber(logNumber, "r");
  if(!f)
    if (DATA_LOGGING_DEBUG) DEBUG_OUTPUT.printf(cE("!!!Error: File NOT exists: '%s'\n"),f.name());
  else
    if (DATA_LOGGING_DEBUG) DEBUG_OUTPUT.printf(cE("Opening file: '%s' for reading of size: %d B\n"),f.name(), f.size());
  return f;
}


File openOldestLogFileForReading()
{
  if (MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:openOldestLogFileForReading()"));
  return openLogFileByNumber(getNumberOfOldestLogFile(), "r");
}


File openCurentLogFileForApending()
{
  if (MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:openCurentLogFileForApending() curentLogNumber_global: ") + curentLogNumber_global);
  File f; Dir dir;
  if(curentLogNumber_global == 0)//0 - inicializační hodnota
  {
    curentLogNumber_global = getNumberOfNewestLogFile();
    if(curentLogNumber_global == 0)//žádný soubor
    {
      if (DATA_LOGGING_DEBUG) DEBUG_OUTPUT.printf(cE("No log file yet! Creating new Log File: %s\n"), getLogFileNameByLogNumber(1).c_str());
      f = createNewEventLogFile(1);
    }
    else
    {
      if (DATA_LOGGING_DEBUG) DEBUG_OUTPUT.printf(cE("Found a log file from previous session: %s\n"), getLogFileNameByLogNumber(1).c_str());
      f = openLogFileByNumber(curentLogNumber_global, "a");
    }
  }
  else
    f = openLogFileByNumber(curentLogNumber_global, "a");

  if(!f)//Pokud se nepodařilo otevřít soubor
  {          
    if (DATA_LOGGING_DEBUG) DEBUG_OUTPUT.printf(cE("!There was a problem to open file: '%s'. Creating new Log File: %s\n"),getLogFileNameByLogNumber(curentLogNumber_global).c_str(), getLogFileNameByLogNumber(curentLogNumber_global+1).c_str());
    curentLogNumber_global = curentLogNumber_global + 1;
    createNewEventLogFile(curentLogNumber_global);
  }

  if(f.size() > MAXIM_FILE_LOG_SIZE)
  {
    curentLogNumber_global = curentLogNumber_global + 1;
    if (DATA_LOGGING_DEBUG) DEBUG_OUTPUT.printf(cE("!File is oversize, name: '%s' size: %d B Creating new Log File: %s\n"),f.name(), f.size(), getLogFileNameByLogNumber(curentLogNumber_global).c_str());
    f = createNewEventLogFile(curentLogNumber_global);
  }  

  if(!f)
  {
    // Serial.println(6);
    if (SHOW_ERROR_DEBUG_MESSAGES) DEBUG_OUTPUT.println(E("!!!Error: Could NOT open any log file! Data will not be saved!"));
    formatSpiffs();
    logNewNodeState(E("FormatedSpiffs - opening problem"));
  }
  else
  {
    if (DATA_LOGGING_DEBUG) DEBUG_OUTPUT.printf(cE("Opening Curent Logging file name: '%s' of size: %d\n"), getLogFileNameByLogNumber(curentLogNumber_global).c_str(), f.size());
  }
  
  return f;
}


////////////////////////////////////////////////////////
//////////////////   LOG NAMES   ///////////////////////
////////////////////////////////////////////////////////
String getLogFileNameByLogNumber(uint16_t logNumber)
{
  return sE("/Log_") + logNumber + E(".csv");
}


uint16_t getNumberOfOldestLogFile(uint16_t minimumFileIndex)
{
  File f;  Dir dir;

  dir = SPIFFS.openDir("/");
  uint16_t oldestFileLogNumber = -1;
  bool foundFile_flag = false;
  while (dir.next())
  {
    f = dir.openFile("r");
    String fileName = f.name();
    uint16_t parsedLogNumber = atoi(fileName.substring(fileName.indexOf('_')+1,fileName.indexOf('.')).c_str());
    // if (DATA_LOGGING_DEBUG) DEBUG_OUTPUT.printf(cE("Procesing file: '%s' parsedLogNumber: %d\n"),fileName.c_str(), parsedLogNumber);
    if(parsedLogNumber < oldestFileLogNumber  && parsedLogNumber > minimumFileIndex)
    {
      foundFile_flag = true;
      oldestFileLogNumber = parsedLogNumber;
    }

    // if (DATA_LOGGING_DEBUG) DEBUG_OUTPUT.printf(cE("File name: '%s' of size: %d B\n"),f.name(), f.size());
    yield_debug();
  }
  return (foundFile_flag) ? oldestFileLogNumber : 0;
}


uint16_t getNumberOfNewestLogFile(uint16_t newestFileLogNumber)
{
  File f;  Dir dir;

  dir = SPIFFS.openDir("/");  
  while (dir.next())
  {
    f = dir.openFile("r");
    String fileName = f.name();
    uint16_t parsedLogNumber = atoi(fileName.substring(fileName.indexOf('_')+1,fileName.indexOf('.')).c_str());
    if(newestFileLogNumber < parsedLogNumber)
      newestFileLogNumber = parsedLogNumber;

    // if (DATA_LOGGING_DEBUG) DEBUG_OUTPUT.printf(cE("File name: '%s' of size: %d B\n"),f.name(), f.size());
    yield_debug();
  }
  return newestFileLogNumber;
}

File openLogFileByNumber(uint16_t logNumber, const char* mode)
{
  return openFile(getLogFileNameByLogNumber(logNumber), mode);
}

///////////////////////////////////////////////////////
///////////////////////////////////////////////////////

void saveReceivedBoilerStateToFile()
{
  if (MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:saveReceivedBoilerStateToFile()"));

  File savedSettingsFile = openFile(SETTINGS_FILENAME, "w");
  savedSettingsFile.println(responseText_global);
  savedSettingsFile.close();
}


bool loadLastSavedBoilerStateFromFile()
{
  if (MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:loadLastSavedBoilerStateFromFile()"));

  if(SPIFFS.exists(SETTINGS_FILENAME))  
  {
    responseText_global = getContentOfFile(SETTINGS_FILENAME);
    if (MAIN_DEBUG) DEBUG_OUTPUT.println(responseText_global);
    return true;
  }
  if (MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:loadLastSavedBoilerStateFromFile() - Settings file not found!"));
  return false;
}

////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
String getContentOfFile(String path)
{
  if (MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:getContentOfFile()"));
  
  File f = openFile(path,"r");

  String fileContent = E("");
  while(f.available())
    fileContent += f.readStringUntil('\n');

  f.close();
  return fileContent;
}

void saveTextToFile(String text, String path)
{
  if (MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:saveStringToFile()"));
  
  File f = openFile(path,"w");
  f.print(text);
  f.close();
}

bool isFileExist(String path)
{
  return SPIFFS.exists(path);
}


File openFile(String fileName, const char* mode) {return openFile(fileName.c_str(), mode);}
File openFile(const char* fileName, const char* mode)
{
  //TODO cE makes problem, porbably buffer internconnections??\
  if (MAIN_DEBUG) DEBUG_OUTPUT.printf(xE("%sF:openFile(fileName='%s', mode='%s')"), getUpTimeDebug().c_str(), fileName, mode);
  File file;

   if(!strcmp(mode, "r") || !strcmp(mode, "r+"))
      if(!SPIFFS.exists(fileName))
      {
        if (MAIN_DEBUG) DEBUG_OUTPUT.println();
        if (SHOW_ERROR_DEBUG_MESSAGES) DEBUG_OUTPUT.printf(cE("\n!!!Error:Cannot open file: '%s' its NOT exists!\n"), fileName);

         return file;
      }

   file = SPIFFS.open(fileName, mode);
   if (!file) 
     if (SHOW_ERROR_DEBUG_MESSAGES) DEBUG_OUTPUT.printf(cE("\n!!!Error: Opening of file: '%s' has failed!\n"), fileName);
   else
     if (DATA_LOGGING_DEBUG) DEBUG_OUTPUT.printf(xE("File: '%s' opened, size: %d\n"), fileName, file.size());
   if (MAIN_DEBUG) DEBUG_OUTPUT.printf(cE(" of size: %d\n"), file.size());
   return file;
}

