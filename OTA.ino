#include <ArduinoOTA.h>
#include <ESP8266httpUpdate.h>
// #if OTA_MODULE_ENABLED

void OTA_begin()
{
  if(wifiConnect())
    ArduinoOTA.begin();
}

void OTA_loop(int)
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug(RESET_SPACE_COUNT_DEBUG) + E("F:OTA_loop()"));
  ArduinoOTA.handle();
}

void silentOTA_loop(int)
{
  {pinMode(2, OUTPUT); digitalWrite(2, LOW); delay(20); digitalWrite(2, HIGH);} //??? co to je?
  ArduinoOTA.handle();
}

void OTA_setup()
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug(RESET_SPACE_COUNT_DEBUG) + E("F:OTA_setup()"));

  DEBUG_OUTPUT.printf(cE("Sketch size: %ukB | Free size: %ukB\n"), ESP.getSketchSize() / 1000, ESP.getFreeSketchSpace() / 1000);

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(cE(NODE_NAME));

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    DEBUG_OUTPUT.println(F("OTA_Started"));
    logNewNodeState(F("OTA: Started"));
    //quickSendNewEventInfo("OTA_Started");

    });
  ArduinoOTA.onEnd([]() {
    DEBUG_OUTPUT.println(F("OTA_Finished"));
    logNewNodeState(F("OTA: Success"));
    // quickSendNewEventInfo("OTA_Finished");
    });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    if((millis()) % 60 < 20) DEBUG_OUTPUT.printf(cE("Progress: %u%%\r\n"), (progress / (total / 100)));
    blinkNotificationLed(10); //blink for 10ms
    
    // if((progress / (total / 100)) % 10 == 0)
    //   displayServiceMessage(((String)"Progress: " + (String)(progress / (total / 100))+ "%"));

    });
    ArduinoOTA.onError([](ota_error_t error) {
      DEBUG_OUTPUT.printf(cE("Error[%u]: "), error);
      if (error == OTA_AUTH_ERROR)          { DEBUG_OUTPUT.println();  logNewNodeState(E("OTA: AuthFailed"));}
      else if (error == OTA_BEGIN_ERROR)    { DEBUG_OUTPUT.println();  logNewNodeState(E("OTA: BeginFailed"));}
      else if (error == OTA_CONNECT_ERROR)  { DEBUG_OUTPUT.println();  logNewNodeState(E("OTA: ConnectFailed"));}
      else if (error == OTA_RECEIVE_ERROR)  { DEBUG_OUTPUT.println();  logNewNodeState(E("OTA: ReceiveFailed"));}
      else if (error == OTA_END_ERROR)      { DEBUG_OUTPUT.println();  logNewNodeState(E("OTA: EndFailed"));}
    OTA_begin();//To OTA recover? last time it ddint recover after fail upload
    });

    //PHP - uploader example
    //http://arduino-esp8266.readthedocs.io/en/latest/ota_updates/readme.html#updater-class
    OTA_begin();

    DEBUG_OUTPUT.print(E("OTA is ready at IP address: "));
    DEBUG_OUTPUT.println(WiFi.localIP());
    yield_debug();
  }

  
// #endif

void checkUpdatesFromHttpServer()
  {
    if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug(RESET_SPACE_COUNT_DEBUG) + E("F:checkUpdatesFromHttpServer()"));

    String updateUrl = sE("http://jindra.cekuj.net/esp8266/update.php?macAddress=")+URLEncode(WiFi.macAddress())+sE("&md5=")+ESP.getSketchMD5()+sE("&compilationDate=")+URLEncode((COMPILATION_DATE));
    // DEBUG_OUTPUT.println(sE("Checking HTTP Update from: ") + updateUrl);
    
    ESPhttpUpdate.rebootOnUpdate(false);
    displayServiceLine(cE("Init: Updating.."));
    switch(ESPhttpUpdate.update(updateUrl))
    {
      case HTTP_UPDATE_FAILED:
      logNewNodeState(sE("!!!HTTP Update: Error(") + ESPhttpUpdate.getLastError() + E("): ") + ESPhttpUpdate.getLastErrorString().c_str());
      break;

      case HTTP_UPDATE_NO_UPDATES:
      DEBUG_OUTPUT.println(sE("HTTP Update: NO Update, Current md5: ") + ESP.getSketchMD5());
      break;

      case HTTP_UPDATE_OK:
      logNewNodeState(E("HTTP Update: Success"));
      restartEsp(E("HTTP Update"));
      break;
    }
  };



  // void getUpdateFromHttpServer()
  // {
  //   DEBUG_OUTPUT.println(sE("MD5 CurrentSketch: ") + ESP.getSketchMD5());
  //   if(!isFileExist("httpUpdate.adr"))
  //   return;

  //   String updateUrl = getContentOfFile("httpUpdate.adr");
  //   if(updateUrl == "")
  //   return;

  //   DEBUG_OUTPUT.println(sE("Updating over HTTP from: ") + updateUrl);
  //   logNewNodeState(F("HTTP Update: Started"));
  //   ESPhttpUpdate.rebootOnUpdate(false);

  //   switch(ESPhttpUpdate.update(updateUrl)) 
  //   {
  //     case HTTP_UPDATE_FAILED:
  //     logNewNodeState(sE("HTTP Update: Error(%d): %s") + ESPhttpUpdate.getLastError() + ESPhttpUpdate.getLastErrorString().c_str());
  //     break;

  //     case HTTP_UPDATE_NO_UPDATES:
  //     logNewNodeState(F("HTTP Update: NO Update"));
  //     break;

  //     case HTTP_UPDATE_OK:
  //     logNewNodeState(F("HTTP Update: Success"));
  //     deleteFileByName("httpUpdate.adr");
  //     break;
  //   }
  //   restartEsp();
  // };


// void saveNewUpdateUrlToFlashMemory(String url)
// {
//   logNewNodeState(F("HTTP Update: New update found"));
//   saveTextToFile(url, "httpUpdate.adr");
//   restartEsp();
// }



// class HttpUpdaterApplication
// {
//   // extern bool logNewNodeState();
//   public:
//   HttpUpdaterApplication(){};
// };

