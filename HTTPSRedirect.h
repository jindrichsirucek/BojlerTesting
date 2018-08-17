#pragma once

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

#ifndef HTTPS_REDIRECT_DEBUG
  #define HTTPS_REDIRECT_DEBUG false
#endif

#define LE(string_literal) (reinterpret_cast<const __FlashStringHelper *>(((__extension__({static const char __c[]     __attribute__((section(".irom.text.template")))     = ((string_literal)); &__c[0];})))))
#define sLE(s) ((String)LE(s))

static const char connectionCloseString[] PROGMEM = "Connection: close\r\n\r\n";
static const char HTTPString[] PROGMEM = " HTTP/1.1\r\n";
static const char acceptString[] PROGMEM = "Accept: */*\r\n";
static const char conntentTypeString[] PROGMEM = "Content-Type: text/html; charset=us-ascii\r\n";
static const char contentLengthString[] PROGMEM = "Content-Length: ";
static const char NEW_LINE_String[] PROGMEM = "\r\n";



typedef uint8_t (*CallBackFunction)(uint8_t);

class HTTPSRedirect : public WiFiClientSecure {
private:
   CallBackFunction animationProgressCallbackFunction;
   uint8_t animationProgressPosition = 0;
   bool isAnimationCallBackFunctionInitialized_flag = false;
   void animateProgress() {if(isAnimationCallBackFunctionInitialized_flag) animationProgressPosition = (*(animationProgressCallbackFunction))(animationProgressPosition);}

public:
   HTTPSRedirect(int port = 443) : m_port(port)
   { }
   void setAnimationProgressCallback(CallBackFunction func) {animationProgressCallbackFunction = func; isAnimationCallBackFunctionInitialized_flag = true;}

   bool GET(const String &host, const String &url)
   {
      if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.print(__PRETTY_FUNCTION__);
      if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.println(sLE("!!!Free Heap: ") + ESP.getFreeHeap());

      if (establishConncetionWithServer(host, m_port) == false) return false;

      if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.print(LE("requesting URL: "));
      if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.println(url.substring(0,50));

      print(LE("GET ")); print(url); print(FPSTR(HTTPString));
      print(LE("Host: ")); print(host); print(FPSTR(NEW_LINE_String));
      print(FPSTR(acceptString));
      print(FPSTR(connectionCloseString));

      if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.print(LE("Sending GET request.."));
      while (available() == 0) 
      {
         if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.print(LE("."));
         delay(200);
         animateProgress();
      }
      if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.println(LE("Sent!"));
      return processHeader();
   }


   bool POST_STRING(const String &host, const String &url, String stringToSend)
   {
      if(establishConncetionWithServer(host, m_port) == false)
         return false;

      setNoDelay(true);
      printHeaderPost(host, url, stringToSend.length());
      print(stringToSend);
      return readResponseAfterPostSent();
   }


   bool POST_FILE(const String &host, const String &url, File fileToSend)
   {
      if(establishConncetionWithServer(host, m_port) == false)
         return false;

      setNoDelay(true);
      printHeaderPost(host, url, fileToSend.size());
      fileToSend.seek(0, SeekSet);
      // printFileToClient(fileToSend);
      write(fileToSend);
      return readResponseAfterPostSent();
   }


protected:
   int m_port;

   bool establishConncetionWithServer(const String &host, const int &port)
   {        
      if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.print(__PRETTY_FUNCTION__);
      if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.println(sLE("!!!Free Heap: ")+ ESP.getFreeHeap());

      yield();
      animateProgress();

      if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.print(LE("connecting to "));
      if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.print(host);

      bool success = connect(host.c_str(), port);
      if (!success) 
      {
         if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.println(LE(" - NOT connected!"));
         return false;
      }
      if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.println(LE(" - Succesfully connected!"));
      return true;
   }


   void printHeaderPost(const String &host, const String &url, uint32_t postSize)
   {
      
      print(sLE("POST ") + url); print(FPSTR(HTTPString));
      print(sLE("Host: ") + host); print(FPSTR(NEW_LINE_String));
      print(FPSTR(acceptString));
         // "Content-Type: text/html\r\n"+//; charset=utf-8 +
      print(FPSTR(conntentTypeString));
      print(FPSTR(contentLengthString)); print(postSize); print(FPSTR(NEW_LINE_String));
      print(FPSTR(connectionCloseString));
   }

   //USed with older core libraries
   // void printFileToClient(File fileToSend)
   // {
   //    uint32_t startTime = millis();
   //    if (HTTPS_REDIRECT_DEBUG) { DEBUG_OUTPUT.printf(cE("Sending file: '%s' of size: %d B\n"), fileToSend.name(), fileToSend.size());}

   //    uint16_t bufSize = 1760; 
   //    byte clientBuf[bufSize];
   //    uint16_t clientCount = 0;
   //    uint32_t alreadySent = 0;

   //    fileToSend.seek(0, SeekSet);
   //    while (fileToSend.available() && alreadySent < fileToSend.size())
   //    {
   //       clientBuf[clientCount] = fileToSend.read();
   //       clientCount++;
   //       alreadySent++;

   //       if (clientCount > bufSize-1)
   //       {          
   //          write((const uint8_t *)clientBuf, bufSize);
   //          clientCount = 0;
   //          if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.print(LE("."));
   //          animateProgress();
   //       }             
   //    } 

   // // final < bufSize byte cleanup packet
   //    if (clientCount > 0) write((const uint8_t *)clientBuf, clientCount);
   //    if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.printf(cE("Sent, in: %d seconds\n"), ((millis() - startTime)/1000));
   // close the file:
   //    fileToSend.close();
   // }

   bool readResponseAfterPostSent()
   {
      // if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.println((""));
      if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.print(LE("Reading response.."));

      uint32_t startTime = millis();

      uint8_t attempt = 100;
      while (available() == 0 && attempt > 0) 
      {
         if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.printf(sLE(".%d").c_str(),available());
         delay(200);
         animateProgress();
         attempt--;
      }
      if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.print((attempt == 0) ? LE("Timeout") : LE("Received"));
      if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.printf(sLE(", in: %d miliseconds\n").c_str(), ((millis() - startTime)/1));

      return processHeader();
   }


   bool processHeader()
   {
      if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.print(__PRETTY_FUNCTION__);
      if (true) Serial.println(sLE("  HTTPSRedirect Free Heap: ") + ESP.getFreeHeap());

      while (available()) 
      {
         if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.println(LE("1"));
         String line = readStringUntil('\n');
         if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.println(line);
         if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.println(LE("2"));
   // if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.println("line: " + line);
         if (line.startsWith(sLE("HTTP/1.1 302 Moved Temporarily")))
         {
            if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.println(LE("3"));
            return redirect();
         }
         else if (line == "\r") {
            if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.println(LE("4"));
            return true;
         }
         if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.println(LE("5"));
      }
      if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.println(LE("!!Redirect NOT found!"));
      if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.println(LE("6"));
      return false;
   }

   bool redirect()
   {
      if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.print(__PRETTY_FUNCTION__);        
      if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.println(sLE("!!!Free Heap: ") + ESP.getFreeHeap());

      String locationStr = LE("Location: ");
      while (connected()) 
      {
         String line = readStringUntil('\n');
         if (line.startsWith(locationStr)) {
            String address = line.substring(locationStr.length());
            String redirHost, redirUrl;
            parseUrl(address, redirHost, redirUrl);

            return GET(redirHost, redirUrl);
         }
         else if (line == "\r") {
            if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.println();
            return false;
         }
      }

      return false;
   }

   void parseUrl(const String &address, String &host, String &url)
   {
   //if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.print();
      if (HTTPS_REDIRECT_DEBUG) DEBUG_OUTPUT.printf("%s !Heap(%u)\n", String(__PRETTY_FUNCTION__).c_str(), ESP.getFreeHeap());

      int protocolEnd = address.indexOf(LE("://"));

      int hostBegin = protocolEnd;
      if (protocolEnd != 0) {
         hostBegin += 3;
      }

      int urlBegin = address.indexOf('/', hostBegin);
      host = address.substring(hostBegin, urlBegin);
      url = address.substring(urlBegin);
   }
};


