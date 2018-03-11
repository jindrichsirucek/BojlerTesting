
#include <ArduinoJson.h>// https://github.com/bblanchon/ArduinoJson

bool isFileReceivedOk()
{
  String inputJsonString = responseText_global;
  // (".errorMessage");

  DynamicJsonBuffer  jsonBuffer;

  JsonObject& root = jsonBuffer.parseObject(inputJsonString);

  // Test if parsing succeeds.
  return root.success();
}




//void doNecesaryActionsUponResponse(String inputJsonString) 
bool doNecesaryActionsUponResponse()
{
  if (MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:doNecesaryActionsUponResponse()"));
  
  String inputJsonString = responseText_global;  
  DynamicJsonBuffer  jsonBuffer;

  JsonObject& root = jsonBuffer.parseObject(inputJsonString);

  // Test if parsing succeeds.
  if (!root.success())
  {
    showServiceMessage(E("Response Error!"));

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
  
  parsedHttpResponses_notErrorCount++;
  showServiceMessage(E("Response OK!"));

  // Most of the time, you can rely on the implicit casts.
  // In other case, you can do root["time"].as<long>();
  byte topHeatingTemp = root[E("topHeatingTemp")];
  byte lowDropingTemp = root[E("lowDropingTemp")];
  bool powerOff = root[E("powerOff")];
  byte lastTemperature = root[E("lastTemperature")];
  String heatingControl = root[E("heatingControl")];
  // String nowTime = root[E("nowTime")];
  // String nowDate = root[E("nowDate" )];
  bool resetNode = root[E("resetNode")];
  String syncTime = root[E("syncTime")];

  if(INTERNET_COMMUNICATION_DEBUG) {root.prettyPrintTo(DEBUG_OUTPUT);  DEBUG_OUTPUT.println();}

  //----------------Procesing----------------  
if(getObjectAskingForResponse().indexOf(cE("begining state update")) != -1);
    synchronizeTimeByResponse(syncTime);
  
  nodeStatusUpdateTime_global = root[E("nodeStatusUpdateTime")];

  if (heatingControl == E("ARDUINO") && getTempControleStyleStringName() != E("ARDUINO"))
    setTempControleStyle(ARDUINO_STYLE_CONTROL);
  else if (heatingControl == E("MANUAL") && getTempControleStyleStringName() != E("MANUAL"))
    setTempControleStyle(MANUAL_STYLE_CONTROL);
  else if (heatingControl == E("OFF") && getTempControleStyleStringName() != E("OFF"))
    setTempControleStyle(OFF_STYLE_CONTROL);

  if(resetNode)
    restartEsp();

  topHeatingTemp_global = topHeatingTemp; 
  lowDropingTemp_global = lowDropingTemp;
  
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





