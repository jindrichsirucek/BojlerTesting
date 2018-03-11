//internet_comunication.ino
#include "HTTPSRedirect.h"
HTTPSRedirect client;

bool uploadLogFile(File fileToSent)
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:uploadLogFile(File fileToSent) :") + fileToSent.name());
  responseText_global = "";
  if(!UPLOADING_DATA_MODULE_ENABLED) { DEBUG_OUTPUT.println(E("UPLOADING_DATA_MODULE is DISABLED")); return false;  }
  
  const uint32_t fileSize = fileToSent.size();
  if(fileToSent.size() > (MAXIM_FILE_LOG_SIZE + 3000))
  {
    if (SHOW_ERROR_DEBUG_MESSAGES) DEBUG_OUTPUT.printf(cE("!!!Error: Maximum size of file is exceeded. File '%s' size %d is bigger than MAXIM_FILE_LOG_SIZE(%d) File will not be sent!\n"), fileToSent.name(), fileToSent.size(), MAXIM_FILE_LOG_SIZE);
    return false;
  }
  showServiceMessage(sE("Sending:") + fileToSent.name());

  const String host = DATA_SERVER_HOST_ADDRESS;
  String url = DATA_SERVER_SCRIPT_URL + "?";

  appendUriNodeIdentification(url);
  url +=  "&logFileName=" + URLEncode(fileToSent.name());

  if(INTERNET_COMMUNICATION_DEBUG) DEBUG_OUTPUT.println(sE("Sending request: ") + host + url + "\nFile to send: " + fileToSent.name());

  // HTTPSRedirect client;
  client.setAnimationProgressCallback(animateWiFiProgressSymbol);

  if (!client.POST_FILE(host, url, fileToSent))
  {
      client.stop();      
      showServiceMessage("File NOT sent!");
      return false;
  }
  else
  {
    showServiceMessage("File sent!");
  }

  //It drops html structure during google script erros
  // client.readStringUntil('max-width:600px');
  responseText_global = client.readStringUntil('\n');
  
  client.stop();
  if(INTERNET_COMMUNICATION_DEBUG) DEBUG_OUTPUT.println(sE("Response text: ") + responseText_global);
  return true;
}


bool sendQuickEventWithPostString(String fireEventName, String message)
{

}


bool uploadDebugLogFileWithGetParams(String uriParamsEncoded)
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:uploadDebugLogFile(String uriParamsEncoded) :") + uriParamsEncoded);
  return sendGetParamsWithPostFile(uriParamsEncoded, RemoteDebug.getLastRuntimeLogAsFile());
}


bool sendGetParamsWithPostFile(String uriParamsEncoded, File fileToSent)
{
  if(!UPLOADING_DATA_MODULE_ENABLED) { DEBUG_OUTPUT.println(E("UPLOADING_DATA_MODULE is DISABLED")); return false;  }

  const String host = DATA_SERVER_HOST_ADDRESS;
  String url = DATA_SERVER_SCRIPT_URL;
  appendUriNodeIdentification(url);
  url += uriParamsEncoded;

  if(INTERNET_COMMUNICATION_DEBUG) DEBUG_OUTPUT.println(sE("Sending request: ") + host + url + cE("\nFile to send: ") + fileToSent.name());
  // HTTPSRedirect client;
  client.setAnimationProgressCallback(animateWiFiProgressSymbol);

  bool successfullySent = client.POST_FILE(host, url, fileToSent);
  // client.readStringUntil('max-width:600px');
  responseText_global = client.readStringUntil('\n');
  client.stop();
  if(INTERNET_COMMUNICATION_DEBUG) DEBUG_OUTPUT.println(sE("Response text: ") + responseText_global);

  showServiceMessage(E("Event sent!"));
  return successfullySent;
}


void appendUriNodeIdentification(String &baseUrl)
{
//append node identification
  String IPString;
  getIPAddresIntoString(IPString);

  baseUrl += sE("?chipId=") + (ESP.getChipId()) + E("&ip=") + URLEncode(IPString) + E("&nodeName=") + URLEncode(NODE_NAME) + E("&macAddress=") + URLEncode(WiFi.macAddress());
  baseUrl += sE("&timeStamp=") + URLEncode(getNowTimeDateString());
  baseUrl += sE("&nodeStatusUpdateTime_global=") + (String)nodeStatusUpdateTime_global;
}


void getIPAddresIntoString(String &IPString)
{
//append node identification
  char buffer[50];
  IPAddress ip = WiFi.localIP();
  sprintf(buffer, cE("%d.%d.%d.%d"), ip[0], ip[1], ip[2], ip[3]);
  // return String(address[0]) + "." + String(address[1]) + "." + String(address[2]) + "." + String(address[3]);
  IPString = (String)buffer;
}


////////////////////////////////////////////////////////
////////////////////////////////////////////////////////


String URLEncode(String msg){return URLEncode(msg.c_str());}
String URLEncode(const char* msg)
{
  const char *hex = cE("0123456789abcdef");
  String encodedMsg = E("");

  while(*msg != '\0') {
    if( ('a' <= *msg && *msg <= 'z')
         || ('A' <= *msg && *msg <= 'Z')
         || ('0' <= *msg && *msg <= '9') ) {
      encodedMsg += *msg;
    } else {
      encodedMsg += '%';
      encodedMsg += hex[*msg >> 4];
      encodedMsg += hex[*msg & 15];
    }
    msg++;
  }
  return encodedMsg;
}

////////////////////////////////////////////////////////
////////////////////////////////////////////////////////

