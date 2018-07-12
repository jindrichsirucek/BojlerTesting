
#include <ArduinoJson.h>// https://github.com/bblanchon/ArduinoJson
#include <TimeLib.h>

bool isFileReceivedOk()
{
  String inputJsonString = responseText_global;
  // (".errorMessage");

  DynamicJsonBuffer  jsonBuffer;

  JsonObject& root = jsonBuffer.parseObject(inputJsonString);

  // Test if parsing succeeds.
  return root.success();
}



bool doNecesaryActionsUponResponse()
{
  if (MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:doNecesaryActionsUponResponse()"));
  
  String inputJsonString = responseText_global;  
  DynamicJsonBuffer  jsonBuffer;

  JsonObject& root = jsonBuffer.parseObject(inputJsonString);

  // Test if parsing succeeds.
  if (!root.success())
  {
    displayServiceMessage(E("Response Error!"));

    stressAskingForResponse();
    notParsedHttpResponses_errorCount++; 
    totalErrorCount_global++;
    if(SHOW_ERROR_DEBUG_MESSAGES) DEBUG_OUTPUT.println(E("responseText_global: "));
    if(SHOW_ERROR_DEBUG_MESSAGES) DEBUG_OUTPUT.println(responseText_global);
    if(SHOW_ERROR_DEBUG_MESSAGES) DEBUG_OUTPUT.println(E(""));
    if(SHOW_ERROR_DEBUG_MESSAGES) DEBUG_OUTPUT.println((String)notParsedHttpResponses_errorCount + E(" responses not parsed, ") + parsedHttpResponses_notErrorCount + E(" parsed OK!"));
    if(INTERNET_COMMUNICATION_DEBUG) DEBUG_OUTPUT.println(sE("NOT Parsed response Text:") + inputJsonString);

    return false;
  }
  if(true) DEBUG_OUTPUT.println(E("responseText_global: "));
  if(true) DEBUG_OUTPUT.println(responseText_global);
    

  // Most of the time, you can rely on the implicit casts.
  // In other case, you can do root["time"].as<long>();
  byte topHeatingTemp = root[E("topHeatingTemp")];
  byte lowDropingTemp = root[E("lowDropingTemp")];
  byte lastTemperature = root[E("lastTemperature")];
  String heatingControl = root[E("heatingControl")];
  // String nowTime = root[E("nowTime")];
  // String nowDate = root[E("nowDate" )];
  bool resetNode = root[E("resetNode")];
  String syncTime = root[E("syncTime")];
  String updateSketchUrl = root[E("updateSketchUrl")];

  if(INTERNET_COMMUNICATION_DEBUG) {root.prettyPrintTo(DEBUG_OUTPUT);  DEBUG_OUTPUT.println();}

  //----------------Procesing----------------  

  if(isWifiConnected()) //Fresh answer from server (otherwise loaded from file)
  {
    parsedHttpResponses_notErrorCount++;
    displayServiceMessage(E("Response OK!"));
    
    if(GLOBAL.nodeInBootSequence)
    {
      synchronizeTimeByResponse(syncTime);
 
      if(waterFlowDisplay_global == 0 && root[E("lastSpareLitres")] != "")
        waterFlowDisplay_global = ((float)BOILER_SIZE_LITRES - (float)root[E("lastSpareLitres")]) * PULSES_PER_LITER_WATER_SENSOR;
    }
    else if(year() != 1970 && minute() < 58 && abs(minute() - syncTime.substring(3,5).toInt()) > 1) //More than a minutte drift, not first time sync, and not 59 minute
    {
      logNewErrorState(sE("@Time: Synced (") + hour()+ "," + minute() + "," + second() + "," + day() + "," + month() + "," + year() + "/" + syncTime + ")");
      synchronizeTimeByResponse(syncTime);
    }

    if(resetNode)
      restartEsp();

    // if(updateSketchUrl.length() > 0)
    //   saveNewUpdateUrlToFlashMemory(updateSketchUrl);
  }

  if(GLOBAL.nodeStatusUpdateTime != root[E("nodeStatusUpdateTime")])
  {
    GLOBAL.nodeStatusUpdateTime = root[E("nodeStatusUpdateTime")];
    logNewNodeState(sE("Settings: update Time changed to:") + GLOBAL.nodeStatusUpdateTime);
  }

  if (heatingControl == E("ARDUINO") && getTempControleStyleStringName() != E("ARDUINO"))
    setTempControleStyle(ARDUINO_STYLE_CONTROL);
  else if (heatingControl == E("MANUAL") && getTempControleStyleStringName() != E("MANUAL"))
    setTempControleStyle(MANUAL_STYLE_CONTROL);
  else if (heatingControl == E("OFF") && getTempControleStyleStringName() != E("OFF"))
    setTempControleStyle(OFF_STYLE_CONTROL);

  
  GLOBAL.TEMP.topHeating = topHeatingTemp; 
  GLOBAL.TEMP.lowDroping = lowDropingTemp;
  
  if(root[E("newSensorAddresses")])
  {
    JsonObject& sensorNames = root[E("newSensorAddresses")];
    for (auto sensorName : sensorNames){
      updateTempSensorAddressByNameFromHexString(sensorName.key, root[E("newSensorAddresses")][sensorName.key]);
    }
    saveTempSensorAddressesToEeprom();
  }

  if(!root[E("dataSavingError")])
  {
    saveReceivedBoilerStateToFile();
    resetObjectAskingForResponse();
    responseText_global = E("");
    return true;
  }

  return false;
}


void addObjectAskingForResponse(String object)
{
  if (WAITING_FOR_RESPONSES_MODULE_ENABLED)
    objectAskingForResponse_global = ((objectAskingForResponse_global == cE("")) ? sE("") : sE(" ")) + object;
}


bool isSomebodyAskingForResponse()
{
  return (objectAskingForResponse_global != sE("")) ? true : false;
}


String getObjectAskingForResponse()
{
  return objectAskingForResponse_global;
}

void resetObjectAskingForResponse()
{
  objectAskingForResponse_global = E("");
}

void stressAskingForResponse()
{
  objectAskingForResponse_global = sE("!") + objectAskingForResponse_global;
}





