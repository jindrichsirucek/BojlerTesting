
const char *const WL_STATUS_MAP_NAMES[] =
{
    /* 0 */"WL_IDLE_STATUS", 
    /* 1 */"WL_NO_SSID_AVAIL", 
    /* 2 */"WL_SCAN_COMPLETED", 
    /* 3 */"WL_CONNECTED", 
    /* 4 */"WL_CONNECT_FAILED", 
    /* 5 */"WL_CONNECTION_LOST", 
    /* 6 */"WL_DISCONNECTED"
};

struct WiFiCredential 
{
  const char *ssid;
  const char *pass;
};


bool isWifiConnected()
{
  yield_debug();
  if(WIFI_DEBUG) DEBUG_OUTPUT.printf(cE("Wifi status: (%d)"), WiFi.status());
  return (WiFi.status() == WL_CONNECTED);
}


bool autoWifiConnect()
{
  if(!UPLOADING_DATA_MODULE_ENABLED) { DEBUG_OUTPUT.println(E("UPLOADING_DATA_MODULE is DISABLED")); return false;  }

  if(isWifiConnected())
  {
    if(WIFI_DEBUG) DEBUG_OUTPUT.println(sE("Wifi is already connected.."));
    return true;
  }
  if(GLOBAL.nodeInBootSequence) 
    displayServiceLine(cE("Init: WiFi"));
  displayServiceMessage(E("WiFi: Searching"));

  // turnWifiOn();
  WiFi.persistent(true); //to save EEPROM http://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/generic-class.html
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA); //not disconnecting Softap actually
  WiFi.hostname(sE("ESP-Node: ") + E(NODE_NAME));

  if(GLOBAL.connectToLastRememberedWifi)
  {
    if(wifiConnectToLastNetwork())
    {
      GLOBAL.connectToLastRememberedWifi = true;
      return true;
    }
    else
      GLOBAL.connectToLastRememberedWifi = false;
  }
  yield_debug();
  return wifiConnectBySavedCredentials();
}


bool wifiConnectBySavedCredentials()
{

  WiFiCredential wifiCredentials[] = {
    {"DS1","daniel12"},
    {"Konfer_net_7","lydia456"},
    {"Regiojet - zluty",""},
    {"CDWiFi",""},
    {"UPC3246803","PDFDFKXG"},//Ostrava
    {"Siruckovi","eAAAdB99DD64fe"},
    {"APJindra","cargocargo"},
    {"UPC3049010","RXYDNHRD"},
    {"UPC3049010-Jindra","RXYDNHRD"},
    {"UPC1918277","XBCXWMFC"},//Hřiště Pellicova
  };//The most prefereable at the end!

  uint8_t n = WiFi.scanNetworks();
  yield_debug();
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(sE("Scanning available WiFi networks.. Found: ")+n + E(" : "));
  
  //Sort by signal strength
  uint8_t sortedByRSSI[n];
  for (int i = 0; i < n; i++)
    sortedByRSSI[i] = i;
  
  for (int i = 0; i < n; i++) 
    for (int j = i + 1; j < n; j++)
      if (WiFi.RSSI(sortedByRSSI[j]) > WiFi.RSSI(sortedByRSSI[i])) 
        std::swap(sortedByRSSI[i], sortedByRSSI[j]);  

  for (int i = 0; i < n; i++) DEBUG_OUTPUT.println((String)WiFi.RSSI(sortedByRSSI[i])+ E("dBi  ") + WiFi.SSID(sortedByRSSI[i]));

  yield_debug();
  // if(MAIN_DEBUG) DEBUG_OUTPUT.println(n);
  uint8_t c = SIZE_OF_LOCAL_ARRAY(wifiCredentials);
  while(--c)
  {
    if(WIFI_DEBUG) DEBUG_OUTPUT.print(sE("Searching for: ") + wifiCredentials[c].ssid);
    for (int i = 0; i < n; ++i)
    {
      if(WiFi.SSID(sortedByRSSI[i]) == wifiCredentials[c].ssid)
      {
        if(WIFI_DEBUG) DEBUG_OUTPUT.println(sE(" - Found!"));
        if(wifiConnectTo(wifiCredentials[c].ssid, wifiCredentials[c].pass))
          return true;
        else
          break;//"for" loop
      }
      yield_debug();
    }
    if(WIFI_DEBUG) DEBUG_OUTPUT.println();
  }
  ERROR_OUTPUT.print(E("!!!Error: Could not connect to any WiFi network("));
  for (int i = 0; i < n; i++) ERROR_OUTPUT.print((String)WiFi.SSID(sortedByRSSI[i])+ "(" +WiFi.RSSI(sortedByRSSI[i])+ ")"+ ", ");
  ERROR_OUTPUT.println(E(")!"));
  return false;
}


bool wifiConnectToLastNetwork()
{
  if(WiFi.SSID() == '\0')
    return false;

  if(MAIN_DEBUG) DEBUG_OUTPUT.print(sE("WiFi: trying to connect to last network:\n") + WiFi.SSID());
  //Set static address saved in eeprom from last time, it should be faster than DHCP
  EepromSettingsStruct settingsStruct = loadGlobalSettingsStructFromEeprom();
  if(false && settingsStruct.wifi.initialized && (WiFi.SSID() == (String)settingsStruct.wifi.ssid))
  {
    WiFi.config(settingsStruct.wifi.ipAddress, settingsStruct.wifi.gatewayAddress, settingsStruct.wifi.maskAddress);
    if(MAIN_DEBUG) DEBUG_OUTPUT.print(sE(" (Static IP from EEPROM: ") + settingsStruct.wifi.ipAddress.toString() + E(")"));
  }
  else
    if(MAIN_DEBUG) DEBUG_OUTPUT.print(E(" (DHCP)"));

  wifi_station_set_auto_connect(true);
  wifi_station_connect();
  yield_debug();
  
  uint8_t animationProgressCounter = 0;
  uint8_t attempt = 30;
  while (--attempt)
  {
    animationProgressCounter = animateWiFiProgressSymbol(animationProgressCounter);
    
    if(isWifiConnected())
      return onWiFiSuccesfullyConnected();

    if(WiFi.status() == WL_CONNECT_FAILED)
      break;

    delay(300);  
  }
  
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(sE("\nUnsuccesfull! (") + WiFi.SSID() + E(")"));
  return false;
}


void turnWifiOn()
{
    if(MAIN_DEBUG) DEBUG_OUTPUT.println(E("WiFi Radio: Turning ON"));
    wifi_fpm_do_wakeup();
    wifi_fpm_close();
    wifi_set_opmode(STATION_MODE);
    wifi_station_connect();
    yield_debug();
}

#define FPM_SLEEP_MAX_TIME 0xFFFFFFF
void turnWifiOff() 
{
    if(MAIN_DEBUG) DEBUG_OUTPUT.println(E("WiFi Radio: Turning OFF"));
    // client.disconnect();
    wifi_station_disconnect();
    wifi_set_opmode(NULL_MODE);
    wifi_set_sleep_type(MODEM_SLEEP_T);
    wifi_fpm_open();
    wifi_fpm_do_sleep(FPM_SLEEP_MAX_TIME);
    yield_debug();
}



bool wifiConnectTo(char const* ssid, char const* pass)
{
  displayServiceMessage(sE("Con:") + ssid);
  if(MAIN_DEBUG) DEBUG_OUTPUT.printf(cE("Wifi connecting to: %s (DHCP)"), ssid);

  //Force to use DHCP server
  wifi_station_dhcpc_start();
  if(*pass == '\0')
    WiFi.begin(ssid);
  else
    WiFi.begin(ssid, pass);
  uint8_t attempt = 50;
  uint8_t animationProgressCounter = 0;
  while (--attempt)
  {
    delay(500);
    if(attempt%10==0) if(MAIN_DEBUG) DEBUG_OUTPUT.print(E("."));
    
    animationProgressCounter = animateWiFiProgressSymbol(animationProgressCounter);

    if(WiFi.status() == WL_CONNECTED)
      break; //Succesfully connected

    if(WiFi.status() == WL_CONNECT_FAILED)
      attempt = 1; //Falied to connect, attempt = 1 ends loop imediately
  }
  
  if(attempt)
    return onWiFiSuccesfullyConnected();

  if(MAIN_DEBUG) DEBUG_OUTPUT.println(sE("\n!!Warning: WIFi NOT connected to: ") + ssid + E(" because: ") + WL_STATUS_MAP_NAMES[WiFi.status()]);
  logNewNodeState(sE("!!Warning: WIFi NOT connected: ") + ssid + E(" : ") + WL_STATUS_MAP_NAMES[WiFi.status()]);
  return false;
}

bool onWiFiSuccesfullyConnected()
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(sE("\nWifi: Connected: ") + WiFi.SSID() + E(" (")+ WiFi.RSSI()+ F("dBm)"));
  displayServiceMessage(WiFi.localIP().toString());
  DEBUG_OUTPUT.println(sE("IPAddress: ") + WiFi.localIP().toString());
  DEBUG_OUTPUT.println(sE("macAddress: ") + WiFi.macAddress());
  saveIpAddressesToEepromSettings();
  logNewNodeState(sE("WiFi: connected to: ") + WiFi.SSID() + E(" (")+ WiFi.RSSI()+ E("dBm)"));

  return true;
}


void saveIpAddressesToEepromSettings()
{
  yield_debug();
  if(WIFI_DEBUG) DEBUG_OUTPUT.println("AFTER really connected");
  if(WIFI_DEBUG) DEBUG_OUTPUT.println(WiFi.SSID());
  if(WIFI_DEBUG) DEBUG_OUTPUT.println(WiFi.localIP().toString());
  if(WIFI_DEBUG) DEBUG_OUTPUT.println(WiFi.subnetMask().toString());
  if(WIFI_DEBUG) DEBUG_OUTPUT.println(WiFi.gatewayIP().toString());

  EepromSettingsStruct settingsStruct = loadGlobalSettingsStructFromEeprom();

  if((String)settingsStruct.wifi.ssid != WiFi.SSID() || settingsStruct.wifi.ipAddress != WiFi.localIP() || WiFi.subnetMask() != settingsStruct.wifi.maskAddress || WiFi.gatewayIP() != settingsStruct.wifi.gatewayAddress)
  {
    strcpy(settingsStruct.wifi.ssid, String(WiFi.SSID()).c_str());
    settingsStruct.wifi.ipAddress = WiFi.localIP();
    settingsStruct.wifi.maskAddress = WiFi.subnetMask();
    settingsStruct.wifi.gatewayAddress = WiFi.gatewayIP();
    settingsStruct.wifi.initialized = true;
    saveGlobalSettingsStructToEeprom(settingsStruct);
  }
}


void displayIPAddress()
{
  displayServiceMessage(WiFi.localIP().toString());
  delay(1000);
}

void displayRSSI()
{
  lcdCreateScaleChars();
  while(true)
  {
    while(!wifiConnectBySavedCredentials());
    displayServiceLine(WiFi.SSID());
    displayServiceMessage(String(WiFi.RSSI()) + F("dbm ") + String("\1\2\3\4\5\6").substring(0,-(-90-WiFi.RSSI())/10));
    DEBUG_OUTPUT.println(WiFi.RSSI());
    RemoteDebug.handle();
    delay(10);
  }
}



// bool wifiConnectToLastNetwork()
// {
//   if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:WiFi Connecting to previous setting: ") + WiFi.SSID());
//   displayServiceMessage(sE("Con: ") + WiFi.SSID());

//   WiFi.begin();
//   yield_debug();
//   uint8_t attempt = 200;
//   if(isWifiConnected() == false)
//   {
//     wifi_station_set_auto_connect(true);

//     WiFi.hostname(NODE_NAME);
    
//     uint8_t animationProgressCounter = 0;
//     while (!isWifiConnected() && attempt--)
//     {
//       delay(100);
//       if(MAIN_DEBUG) DEBUG_OUTPUT.print(E("."));
//       animationProgressCounter = animateWiFiProgressSymbol(animationProgressCounter);
//     }
//   }

//   if(attempt)
//   {
//     if(MAIN_DEBUG) DEBUG_OUTPUT.println(E("\nConnected!"));
//     displayIPAddress();
//     return true;
//   }

//   if(MAIN_DEBUG) DEBUG_OUTPUT.println(E("\nNOT connected!"));
//   return false;
// }


// RE: CHOOSE BETWEEN MULTIPLE ACCESS POINTS WITH SAME SSID?
// Basically, you first have to choose an AP, and then pass its BSSID to WiFi.begin:
// WiFi.begin(const char* ssid, const char *passphrase, int32_t channel, uint8_t bssid[6]);
// Note that if you want to pass bssid you also need to set the channel number.
// Getting BSSID and channel number from scan results is possible by calling WiFi.BSSID(index) and WiFi.channel(index).



// Signal Strength
// -30 dBm Amazing    Max achievable signal strength.
// -67 dBm Very Good  Minimum signal strength for applications that require very reliable, timely delivery of data packets. VoIP/VoWiFi, streaming video
// -70 dBm Okay       Minimum signal strength for reliable packet delivery. Email, web
// -80 dBm Not Good   Minimum signal strength for basic connectivity. Packet delivery may be unreliable.  
// -90 dBm Unusable   Approaching or drowning in the noise floor. Any functionality is highly unlikely. 