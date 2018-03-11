#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>


void OTA_setup()
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:OTA_setup()"));

  DEBUG_OUTPUT.printf(cE("Sketch size: %ukB | Free size: %ukB\n"), ESP.getSketchSize() / 1000, ESP.getFreeSketchSpace() / 1000);

 // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(NODE_NAME);

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
    //   showServiceMessage(((String)"Progress: " + (String)(progress / (total / 100))+ "%"));

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

void OTA_begin()
{
  ArduinoOTA.begin();
}

void OTA_loop(int)
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug(RESET_SPACE_COUNT_DEBUG) + E("F:OTA_loop()"));
  ArduinoOTA.handle();
}

void silentOTA_loop(int)
{
  {pinMode(2, OUTPUT); digitalWrite(2, LOW); delay(20); digitalWrite(2, HIGH);}
  ArduinoOTA.handle();
}
