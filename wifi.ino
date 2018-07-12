
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


bool wifiConnectToLastNetwork()
{
  if(WiFi.SSID() != '\0')
    if(MAIN_DEBUG) DEBUG_OUTPUT.println(sE("WiFi: trying to connect to last network: ") + WiFi.SSID());
  else
    return false;
  wifi_station_set_auto_connect(true);
  wifi_station_connect();
  // WiFi.begin();
  yield_debug();
  // WiFi.waitForConnectResult();
  uint8_t animationProgressCounter = 0;
  uint8_t attempt = 100;
  while (--attempt)
  {
    delay(100);
    if(MAIN_DEBUG) DEBUG_OUTPUT.print(E("."));
    animationProgressCounter = animateWiFiProgressSymbol(animationProgressCounter);

    if(isWifiConnected())
      return onWiFiSuccesfullyConnected();

    if(WiFi.status() == WL_CONNECT_FAILED)
      break;
  }
  
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(E("\n - Unsuccesfull!"));
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
  if(GLOBAL.nodeInBootSequence) 
    displayServiceLine(cE("Init: WiFi"));
  displayServiceMessage(E("WiFi: Searching"));

  // turnWifiOn();
  WiFi.mode(WIFI_STA);
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
  };//The most prefereable at the end!

  if(MAIN_DEBUG) DEBUG_OUTPUT.print(E("Scanning available WiFinetworks.. Found: "));
  yield_debug();
  uint8_t n = WiFi.scanNetworks();
  
  //Sort by signal strength
  uint8_t indices[n];
  for (int i = 0; i < n; i++)
    indices[i] = i;
  
  for (int i = 0; i < n; i++) 
    for (int j = i + 1; j < n; j++)
      if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i])) 
        std::swap(indices[i], indices[j]);  

  for (int i = 0; i < n; i++) DEBUG_OUTPUT.print((String)WiFi.SSID(indices[i])+ "(" +WiFi.RSSI(indices[i])+ ")"+ ", ");

  yield_debug();
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(n);
  uint8_t c = SIZE_OF_LOCAL_ARRAY(wifiCredentials);
  while(--c)
  {
    if(WIFI_DEBUG) DEBUG_OUTPUT.print(sE("Searching for: ") + wifiCredentials[c].ssid);
    for (int i = 0; i < n; ++i)
    {
      if(WiFi.SSID(indices[i]) == wifiCredentials[c].ssid)
      {
        if(WIFI_DEBUG) DEBUG_OUTPUT.println(sE(" - Found!"));
        if(wifiConnectTo(wifiCredentials[c].ssid, wifiCredentials[c].pass))
          return true;
        else
          break;//"for" lopp
      }
      yield_debug();
    }
    if(WIFI_DEBUG) DEBUG_OUTPUT.println();
  }
  ERROR_OUTPUT.print(E("!!!Error: Could not connect to any WiFi network("));
  for (int i = 0; i < n; i++) ERROR_OUTPUT.print((String)WiFi.SSID(indices[i])+ "(" +WiFi.RSSI(indices[i])+ ")"+ ", ");
  ERROR_OUTPUT.println(E(")!"));
  return false;
}


bool wifiConnectTo(char const* ssid, char const* pass)
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.printf(cE("Wifi connecting to: %s..\n"), ssid);
  displayServiceMessage(sE("Con:") + ssid);

  if(*pass == '\0')
    WiFi.begin(ssid);
  else
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
  logNewNodeState(sE("WiFi: connected to: ") + WiFi.SSID() + F(" (")+ WiFi.RSSI()+ F("dBm)"));
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
  // String IPString;
  // getIPAddresIntoString(IPString);
  DEBUG_OUTPUT.println(WiFi.localIP().toString());
  displayServiceMessage(WiFi.localIP().toString());
  delay(1000);
}

void displayRSSI()
{
  wifiConnect();
  displayServiceLine(WiFi.SSID());
  lcdCreateScaleChars();
  while(delay(10), true)
    displayServiceMessage(String(WiFi.RSSI()) + F("dbm ") + String("\1\2\3\4\5\6").substring(0,-(-90-WiFi.RSSI())/10));
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