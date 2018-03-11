const char *const WL_STATUS_MAP_NAMES[] =
{
    /* 0 */"WL_IDLE_STATUS", 
    /* 1 */"WL_NO_SSID_AVAIL", 
    /* 2 */"WL_SCAN_COMPLETED", 
    /* 3 */"WL_CONNECTED", 
    /* 4 */"WL_CONNECT_FAILED", 
    /* 5 */"WL_CONNECTION_LOST", 
    /* 6 */"WL_DISCONNECTED4" 
};


bool wifiConnectToLastNetwork()
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.print(sE("WiFi: trying to connect to last network: ") + WiFi.SSID());
  wifi_station_set_auto_connect(true);
  wifi_station_connect();
  WiFi.waitForConnectResult();

  if(isWifiConnected())
    return onWiFiSuccesfullyConnected();
  else
    if(MAIN_DEBUG) DEBUG_OUTPUT.println(E(" - Unsuccesfull!"));
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
    if(MAIN_DEBUG) DEBUG_OUTPUT.println(E("Diconnecting client and wifi"));
    // client.disconnect();
    wifi_station_disconnect();
    wifi_set_opmode(NULL_MODE);
    wifi_set_sleep_type(MODEM_SLEEP_T);
    wifi_fpm_open();
    wifi_fpm_do_sleep(FPM_SLEEP_MAX_TIME);
    yield_debug();
}

// void printAvailableWifiNetworks()
// {
//   Serial.println("scanning networks");
//   int n = WiFi.scanNetworks();
//   Serial.println("scan done");
//   if(n == 0)
//     Serial.println("no networks found");
//   else
//   {
//      Serial.print(n);
//      Serial.println(" networks found");
//      for (int i = 0; i < n; ++i)
//        Serial.println(WiFi.SSID(i));
//   }
// }

struct WiFiCredential{
  const char *ssid;
  const char *pass;
};

bool wifiConnect()
{
  if(!UPLOADING_DATA_MODULE_ENABLED) { DEBUG_OUTPUT.println(E("UPLOADING_DATA_MODULE is DISABLED")); return false;  }

  if(isWifiConnected())
  {
    if(WIFI_DEBUG) DEBUG_OUTPUT.println(sE("Wifi is already connected.."));
    return true;
  }

  WiFi.mode(WIFI_STA);
  WiFi.hostname(sE("ESP-Node: ") + NODE_NAME);

  if(wifiConnectToLastNetwork())
    return true;
  
  WiFiCredential savedWifiCredentials[] = {
    {"DS1","daniel12"},
    {"Konfer_net_7","lydia456"},
    {"UPC3246803","PDFDFKXG"},
    {"APJindra","cargocargo"},
    {"Siruckovi","eAAAdB99DD64fe"},
    {"UPC3049010","RXYDNHRD"},
  };//The most prefereable at the end!


  if(WIFI_DEBUG) if(WIFI_DEBUG) DEBUG_OUTPUT.print(E("Scanning available WiFinetworks.. Found: "));
  yield_debug();
  uint8_t n = WiFi.scanNetworks();
  yield_debug();
  if(WIFI_DEBUG) DEBUG_OUTPUT.println(n);
  uint8_t c = SIZE_OF_LOACAL_ARRAY(savedWifiCredentials);
  while(--c)
  {
    if(WIFI_DEBUG) DEBUG_OUTPUT.print(sE("Searching for: ") + savedWifiCredentials[c].ssid);
    for (int i = 0; i < n; ++i)
    {
      if(WiFi.SSID(i) == savedWifiCredentials[c].ssid)
      {
        if(WIFI_DEBUG) DEBUG_OUTPUT.println(sE(" - Found!"));
        if(wifiConnectTo(savedWifiCredentials[c].ssid, savedWifiCredentials[c].pass))
          return true;
        else
          break;//for lopp
      }
      yield_debug();
    }
    if(WIFI_DEBUG) DEBUG_OUTPUT.println();
  }
  return false;
}



bool wifiConnectTo(char const* ssid, char const* pass)
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.printf(cE("Wifi connecting to: %s..\n"), ssid);
  showServiceMessage(sE("Con:") + ssid);

  WiFi.begin(ssid, pass);
  uint8_t attempt = 200;
  uint8_t animationProgressCounter = 0;
  while (--attempt)
  {
    delay(100);
    if(MAIN_DEBUG) DEBUG_OUTPUT.print(E("."));
    animationProgressCounter = animateWiFiProgressSymbol(animationProgressCounter);

    if(WiFi.status() == WL_CONNECTED)
      break; //Succesfully connected

    if(WiFi.status() == WL_CONNECT_FAILED)
      attempt = 1; //Falied to connect, attempt = 1 ends loop imediately
  }
  
  if(attempt)
    return onWiFiSuccesfullyConnected();

  if(MAIN_DEBUG) DEBUG_OUTPUT.println(sE("\n!!Warning: WIFi NOT connected to: ") + ssid + E(" because: ") + WL_STATUS_MAP_NAMES[WiFi.status()]);
  logNewNodeState(sE("\n!!Warning: WIFi NOT connected: ") + ssid + E(" : ") + WL_STATUS_MAP_NAMES[WiFi.status()]);
  return false;
}

bool onWiFiSuccesfullyConnected()
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(sE("\nConnected to: ") + WiFi.SSID());
  displayIPAddress();
  logNewNodeState(sE("WiFi: connected to: ") + WiFi.SSID());
  return true;
}

bool isWifiConnected()
{
  yield_debug();
  if(WIFI_DEBUG) DEBUG_OUTPUT.printf(cE("Wifi status: (%d)"), WiFi.status());
  return (WiFi.status() == WL_CONNECTED);
}




void displayIPAddress()
{
  String IPString;
  getIPAddresIntoString(IPString);
  showServiceMessage(IPString);
  delay(1000);
}


// bool wifiConnectToLastNetwork()
// {
//   if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:WiFi Connecting to previous setting: ") + WiFi.SSID());
//   showServiceMessage(sE("Con: ") + WiFi.SSID());

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


  