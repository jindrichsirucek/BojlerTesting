#include <ArduinoOTA.h>
#include <ESP8266httpUpdate.h>
// #if OTA_MODULE_ENABLED

void OTA_begin()
{
  #if OTA_MODULE_ENABLED
    if(autoWifiConnect())
      ArduinoOTA.begin();
  #endif
}

void OTA_loop(int)
{
  #if OTA_MODULE_ENABLED
    if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug(RESET_SPACE_COUNT_DEBUG) + E("F:OTA_loop()"));
    ArduinoOTA.handle();
  #endif
}

void silentOTA_loop(int)
{
  #if OTA_MODULE_ENABLED
    ArduinoOTA.handle();
  #endif
}

void OTA_setup()
{
  #if OTA_MODULE_ENABLED

    if(!isWifiConnected())
      return;

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

    });
    ArduinoOTA.onEnd([]() {
      DEBUG_OUTPUT.println(F("OTA_Finished"));
      logNewNodeState(F("OTA: Success"));
      restartEsp(E("OTA Update"));
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      if((millis()) % 500 < 20) DEBUG_OUTPUT.printf(cE("Progress: %u%%\r\n"), (progress / (total / 100)));
    blinkNotificationLed(10); //blink for 10ms
    displayServiceMessage(sE("OTA: ") + (progress / (total / 100)) + E("%"));
    
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
  #endif  
}

  
// #endif

bool checkUpdatesFromHttpServer()
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
      // logNewNodeState(E("HTTP Update: Success"));
      saveTextToFile(E("UpdateOk"), MODULE_UPDATE_FILE_NAME);
      restartEsp(E("HTTP Update"));
      return true;
      break;
    }
    return false;
  };

