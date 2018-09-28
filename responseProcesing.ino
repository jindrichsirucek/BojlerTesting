
#include <ArduinoJson.h>// https://github.com/bblanchon/ArduinoJson
#include <TimeLib.h>

bool isLastSavedServerResponseOk() {return isLastSavedServerResponseOk(getLastResponseString());}
bool isLastSavedServerResponseOk(String responseText)
{
  DynamicJsonBuffer  jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(responseText);

  // Test if parsing succeeds.
  if (!root.success())
  {
    displayServiceMessage(E("Response Error!"));

    notParsedHttpResponses_errorCount++; 
    totalErrorCount_global++;
    if(SHOW_ERROR_DEBUG_MESSAGES) DEBUG_OUTPUT.println(E("responseText: "));
    if(SHOW_ERROR_DEBUG_MESSAGES) DEBUG_OUTPUT.println(responseText);
    if(SHOW_ERROR_DEBUG_MESSAGES) DEBUG_OUTPUT.println();
    if(SHOW_ERROR_DEBUG_MESSAGES) DEBUG_OUTPUT.println((String)notParsedHttpResponses_errorCount + E(" responses not parsed, ") + parsedHttpResponses_notErrorCount + E(" parsed OK!"));
    if(INTERNET_COMMUNICATION_DEBUG) DEBUG_OUTPUT.println(sE("NOT Parsed response Text:") + responseText);

    return false;
  }
  // Test if parsing succeeds and no error.
  return root.success()  && !root[E("dataSavingError")];
}

String getLastResponseString()
{
   return isFileExist(SETTINGS_FILENAME)? getContentOfFile(getLastResponseFileStream(("r"))) : "";
}


bool doNecesaryActionsUponResponse()
{
  if (MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:doNecesaryActionsUponResponse()"));
  
  String responseText = getLastResponseString();
  if(!isLastSavedServerResponseOk(responseText))
    return false;

  DEBUG_OUTPUT.println(E("responseText: "));
  DEBUG_OUTPUT.println(responseText);
  
  
  DynamicJsonBuffer  jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(responseText);
  
  // Most of the time, you can rely on the implicit casts.
  // In other case, you can do root["time"].as<long>();
  uint8_t topHeatingTemp = root[E("topHeatingTemp")];
  uint8_t lowDropingTemp = root[E("lowDropingTemp")];
  uint8_t lastHeatedTemperature = root[E("lastHeatedTemperature")];
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
    
    //BOOT sequence settings
    if(GLOBAL.nodeInBootSequence && isWifiConnected())
    {
      synchronizeTimeByResponse(syncTime);
 
      if(waterFlowDisplay_global == 0 && root[E("lastSpareLitres")] != "")
        waterFlowDisplay_global = ((float)BOILER_SIZE_LITRES - (float)root[E("lastSpareLitres")]) * PULSES_PER_LITER_WATER_SENSOR;
      
      if(lastHeatedTemperature)
        setLastHeatedTemp(lastHeatedTemperature);
      else
        DEBUG_OUTPUT.println(sE("!!WARNING: Recieved wrong lastHeatedTemperature: ") + lastHeatedTemperature);

      //Heating, relay set correct starting state
      if(isTemperatureCorrectMeasurment(GLOBAL.TEMP.sensors[BOJLER].temp) && GLOBAL.TEMP.sensors[BOJLER].temp >= topHeatingTemp)
      {  
        if(lastElectricCurrentState_global == true)
        {
          lastElectricCurrentState_global = false;
          setHeatingRelayOpen(SET_HEATING_RELAY_DISCONECTED); //Pull relay pin when heating is of and there is electricity
        }
        if(GLOBAL.TEMP.heatingState == true)
          GLOBAL.TEMP.heatingState = false;
      }
    }
    else if(/*year() != 1970 && */minute() < 58 && abs(minute() - syncTime.substring(3,5).toInt()) > 1) //More than a minutte drift, not first time sync, and not 59 minute
    {
      logNewStateWithEmail(sE("@Time: Synced (") + hour()+ "," + minute() + "," + second() + "," + day() + "," + month() + "," + year() + "/" + syncTime + ")");
      synchronizeTimeByResponse(syncTime);
    }

    if(resetNode)
      restartEsp(E("onResponse Demand"));
  }

  if(GLOBAL.nodeStatusUpdateTime != root[E("nodeStatusUpdateTime")])
  {
    GLOBAL.nodeStatusUpdateTime = root[E("nodeStatusUpdateTime")];
    logNewNodeState(sE("Settings: update Time changed to: ") + GLOBAL.nodeStatusUpdateTime/MIN + E("min"));
  }

  if (heatingControl == E("ARDUINO") && getTempControleStyleStringName() != E("ARDUINO"))
    setTempControleStyle(BOILER_CONTROL_PROGRAMATIC);
  else if (heatingControl == E("MANUAL") && getTempControleStyleStringName() != E("MANUAL"))
    setTempControleStyle(BOILER_CONTROL_MANUAL);
  else if (heatingControl == E("OFF") && getTempControleStyleStringName() != E("OFF"))
    setTempControleStyle(BOILER_CONTROL_OFF);

  
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

  return true;
}


// void addObjectAskingForResponse(String object)
// {
//   if (WAITING_FOR_RESPONSES_MODULE_ENABLED)
//     objectAskingForResponse_global = ((objectAskingForResponse_global == cE("")) ? sE("") : sE(" ")) + object;
// }


// bool isSomebodyAskingForResponse()
// {
//   return (objectAskingForResponse_global != sE("")) ? true : false;
// }


// String getObjectAskingForResponse()
// {
//   return objectAskingForResponse_global;
// }

// void resetObjectAskingForResponse()
// {
//   objectAskingForResponse_global = E("");
// }

// void stressAskingForResponse()
// {
//   objectAskingForResponse_global = sE("!") + objectAskingForResponse_global;
// }





