//internet_comunication.ino
#include "HTTPSRedirect.h"
;

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
  displayServiceMessage(sE("Sending:") + fileToSent.name());

  const String host = DATA_SERVER_HOST_ADDRESS;
  String url = DATA_SERVER_SCRIPT_URL + E("?");

  appendUriNodeIdentification(url);
  url += cE("&logFileName=") + URLEncode(fileToSent.name());

  if(INTERNET_COMMUNICATION_DEBUG) DEBUG_OUTPUT.println(sE("Sending request: ") + host + url + E("\nFile to send: ") + fileToSent.name());

  HTTPSRedirect client;
  client.setAnimationProgressCallback(animateWiFiProgressSymbol);

  if (!client.POST_FILE(host, url, fileToSent))
  {
      client.stop();      
      displayServiceMessage(E("File NOT sent!"));
      return false;
  }
  else
  {
    displayServiceMessage(E("File sent!"));
  }

  //It drops html structure during google script erros
  // client.readStringUntil('max-width:600px');
  responseText_global = client.readStringUntil('\n');
  
  client.stop();
  if(INTERNET_COMMUNICATION_DEBUG) DEBUG_OUTPUT.println(sE("Response text: ") + responseText_global);
  return true;
}


bool sendNewNodeStateByPostString(String dataToSend)
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:sendNewNodeStateByPostString(String dataToSend): ") + dataToSend);
  String headerWithDataString = sE("time;fireEventName;bojlerTemp;pipeTemp;roomTemp;insideFlowTemp;current;waterFlow;test1Value;test2Value;test3Value;objectAskingForResponse;heatingState;controlState;nodeInfoString") + "\r\n" + dataToSend + "\r\n";

  displayServiceMessage(E("Data: Posting"));

  const String host = DATA_SERVER_HOST_ADDRESS;
  String url = DATA_SERVER_SCRIPT_URL + "?";

  appendUriNodeIdentification(url);
  url +=  E("&logFileName=tempString");

  if(INTERNET_COMMUNICATION_DEBUG) DEBUG_OUTPUT.println(sE("Sending request: ") + host + url + E("\nData to send: tempString"));

  HTTPSRedirect client;
  client.setAnimationProgressCallback(animateWiFiProgressSymbol);

  if (!client.POST_STRING(host, url, headerWithDataString))
  {
      client.stop();      
      displayServiceMessage(E("Data: NOT sent!"));
      return false;
  }
  else
    displayServiceMessage(E("Data: Sent!"));

  //It drops html structure during google script erros
  // client.readStringUntil('max-width:600px');
  responseText_global = client.readStringUntil('\n');
  
  client.stop();
  if(INTERNET_COMMUNICATION_DEBUG) DEBUG_OUTPUT.println(sE("Response text: ") + responseText_global);
  return true;
}


bool uploadDebugLogFileWithGetParams(String uriParamsEncoded)
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:uploadDebugLogFile(String uriParamsEncoded) :") + uriParamsEncoded);
  return sendGetParamsWithPostFile(uriParamsEncoded, RemoteDebug.getLastRuntimeLogAsFile());
}


bool sendGetParamsWithPostFile(String uriParamsEncoded, File fileToSend)
{
  if(!UPLOADING_DATA_MODULE_ENABLED) { DEBUG_OUTPUT.println(E("UPLOADING_DATA_MODULE is DISABLED")); return false;  }

  const String host = DATA_SERVER_HOST_ADDRESS;
  String url = DATA_SERVER_SCRIPT_URL;
  appendUriNodeIdentification(url);
  url += uriParamsEncoded;

  if(INTERNET_COMMUNICATION_DEBUG) DEBUG_OUTPUT.println(sE("Sending request: ") + host + url + E("\nFile to send: ") + fileToSend.name());
  HTTPSRedirect client;
  client.setAnimationProgressCallback(animateWiFiProgressSymbol);

  bool successfullySent = client.POST_FILE(host, url, fileToSend);
  // client.readStringUntil('max-width:600px');
  responseText_global = client.readStringUntil('\n');
  client.stop();
  if(INTERNET_COMMUNICATION_DEBUG) DEBUG_OUTPUT.println(sE("Response text: ") + responseText_global);

  displayServiceMessage(E("Event sent!"));
  return successfullySent;
}


void appendUriNodeIdentification(String &baseUrl)
{
  //append node identification
  baseUrl += sE("?chipId=") + (ESP.getChipId()) + E("&ip=") + URLEncode(WiFi.localIP().toString()) + E("&nodeName=") + URLEncode(E(NODE_NAME)) + E("&macAddress=") + URLEncode(WiFi.macAddress());
  baseUrl += sE("&timeStamp=") + URLEncode(getNowTimeDateString());
  baseUrl += sE("&GLOBAL.nodeStatusUpdateTime=") + (String)GLOBAL.nodeStatusUpdateTime;
  baseUrl += sE("&compilationDate=") + URLEncode(COMPILATION_DATE);
  baseUrl += sE("&sketchMd5=") + ESP.getSketchMD5();
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

