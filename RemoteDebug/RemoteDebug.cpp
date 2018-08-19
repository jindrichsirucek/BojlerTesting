////////
// Libraries Arduino
//
// Library: Remote debug - debug over telnet - for Esp8266 (NodeMCU)
// Author: Joao Lopes
// Tanks: Example of TelnetServer code in http://www.rudiswiki.de/wiki9/WiFiTelnetServer
//
// Versions:
//    - 0.9.0 Beta 1 - August 2016
//    - 0.9.1 Beta 2 - Octuber 2016
//    - 1.0.0 RC - January 2017
//
//  TODO: - Page HTML for begin/stop Telnet server
//        - Authentications
///////

#define VERSION "1.0.0"
#include <Arduino.h>
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include "RemoteDebug.h"
#include "FS.h"

extern void restartEsp(String);
extern bool logWarningMessage(String fireEventName);

// Telnet server

WiFiServer telnetServer(TELNET_PORT);
WiFiClient telnetClient;

// Buffer of print write to telnet

String bufferPrint = "";

// Initialize the telnet server

void RemoteDebug::begin (String hostName) {

    // Initialize server telnet

    telnetServer.begin();
    telnetServer.setNoDelay(true);

    // Reserve space to buffer of print writes

    bufferPrint.reserve(BUFFER_PRINT);
    //REMOVED:_lastErrorWarningMessage.reserve(BUFFER_PRINT);

    // Host name of this device

    _hostName = hostName;
}

// Stop the server

void RemoteDebug::stop () {

    // Stop Client

    if (telnetClient && telnetClient.connected()) {
        telnetClient.stop();
    }

    // Stop server

    telnetServer.stop();
}

// Handle the connection (in begin of loop in sketch)

void RemoteDebug::handle() {
    if(_serialEnabled)
      Serial.flush();
//uint32_t timeBegin = millis();

    // look for Client connect trial

    if (telnetServer.hasClient()) {

      if (!telnetClient || !telnetClient.connected()) {

        if (telnetClient) { // Close the last connect - only one supported

          telnetClient.stop();

        }

        // Get telnet client

        telnetClient = telnetServer.available();

        telnetClient.flush();  // clear input buffer, else you get strange characters

        _lastTimeCommand = millis(); // To mark time for inactivity

        // Show the initial message

        showHelp ();

        // File fileLog = SPIFFS.open(_loggingFileName + ((String)(_logingFileFirstOrSecond)), "r"); //cant be used, because sending last log file during boot sequence connects files together
        File fileLog = getLastRuntimeLogAsFile(); //it shows ALL saved hisory in log files when telnet connects
        while(fileLog.available())
          telnetClient.println(fileLog.readStringUntil('\n'));
        fileLog.close();
        telnetClient.println(F("^^^^^^^^^^  LOADED FROM LOG FILES  ^^^^^^^^^^"));

        // Empty buffer in

        while(telnetClient.available()) {
            telnetClient.read();
        }

      }
    }

    // Is client connected ? (to reduce overhead in active)

    _connected = (telnetClient && telnetClient.connected());

    // Get command over telnet

    if (_connected) {

        while(telnetClient.available()) {  // get data from Client

            // Get character

            char character = telnetClient.read();

            // Newline or CR

            if (character == '\n' || character == '\r') {

                // Process the command

                if (_command.length() > 0) {

                    processCommand();

                }

                _command = ""; // Init it for next command

            } else if (isPrintable(character)) {

                // Concat
                _command.concat(character);

            }
        }

#ifdef MAX_TIME_INACTIVE

        // Inactivit - close connection if not received commands from user in telnet
        // For reduce overheads

        if ((millis() - _lastTimeCommand) > MAX_TIME_INACTIVE) {
            telnetClient.println(F("* Closing session by inactivity"));
            telnetClient.stop();
            _connected = false;
        }
#endif

    }
//DV("*handle time: ", (millis() - timeBegin));
}

// Send to serial too  (not recommended)

void RemoteDebug::setSerialEnabled(bool enable) {
    _serialEnabled = enable;
}

// Allow ESP reset over telnet client

void RemoteDebug::setResetCmdEnabled(bool enable) {
    _resetCommandEnabled = enable;
}

// Set help for commands over telnet setted by sketch

void RemoteDebug::setHelpProjectsCmds(String help) {
    _helpProjectCmds = help;
}

// Set callback of sketch function to process project messages

void RemoteDebug::setCallBackProjectCmds(void (*callback)()) {
    _callbackProjectCmds = callback;
}

// Print

size_t RemoteDebug::write(uint8_t character) 
{
    size_t ret = 0;
    
    if (_serialEnabled) { // Echo to serial
        ret = Serial.write(character);
    }
    
    // New line writted before ?
    if (_newLine) {
        String show = "";
        if (show != "") {

            String send = F("(");
            send.concat(show);
            send.concat(F(") "));

            // Write to telnet buffered
            if (_connected || _serialEnabled) {  // send data to Client
                bufferPrint = send;
            }
        }
        _newLine = false;
    }

    // Print ?
    bool doPrint = false;
    // New line ?
    if (character == '\n') {
        bufferPrint.concat(F("\r")); // Para clientes windows - 29/01/17
        _newLine = true;
        doPrint = true;
    } else if (bufferPrint.length() == BUFFER_PRINT) { // Limit of buffer
        doPrint = true;
    }

    // Write to telnet Buffered
    bufferPrint.concat((char)character);
    
    // Send the characters buffered by print.h
    if (doPrint) { // Print the buffer
        {
            // Write to telnet Buffered
            if (_connected) {  // send data to Client
                telnetClient.print(bufferPrint);
            }

            if (_logingToFileEnabled) 
            {
               const char* charString = bufferPrint.c_str();
               for(uint8_t i = 0 ; i <3; i++)
               {
                 if(charString[i] == '\0')
                   break;
                 if(charString[i] != '!')
                   break;
                 if(i == 1)
                 {
                   _isThereWarningMessage = true;
                   bufferPrint.replace(F("!!"),"");
                   bufferPrint.replace(F("\n"),"");
                   bufferPrint.replace(F("\r"),"");
                   
                   File errorFile = SPIFFS.open(_warningsFileName, ("a"));
                   if(errorFile.size() < 5000)
                   {
                     errorFile.print(bufferPrint);
                     errorFile.print('\r');
                   }
                   errorFile.close();
                 }
                 if(i == 2)
                    _isThereErrorMessage = true;
                }

               File loggingFile = SPIFFS.open((_loggingFileName + (String)_logingFileFirstOrSecond), "a");
               // Serial.println((String)"_logingFileFirstOrSecond: " + _logingFileFirstOrSecond + "loggingToFile: " + (_loggingFileName + String((_logingFileFirstOrSecond))) + " FileSize: " + loggingFile.size());
               // loggingFile.print((String)((_logingFileFirstOrSecond) ? "A - " : "B - ") + bufferPrint);
               
               loggingFile.print(bufferPrint);

               if(loggingFile.size() > 5000)
               {  
                 _logingFileFirstOrSecond = !_logingFileFirstOrSecond;
                 // String fileNameToDelete = loggingFile.name();
                 loggingFile.close();
                 // Serial.println((String)"Removing file" + fileNameToDelete);
                 // SPIFFS.remove(fileNameToDelete);
                 SPIFFS.remove(_loggingFileName + String(_logingFileFirstOrSecond));
               }
               else
                 loggingFile.close();
            }
        }
        // Empty the buffer
        bufferPrint = "";
    }

    return ret;
}


void RemoteDebug::setLogFileEnabled(bool enable) 
{
    _logingToFileEnabled = enable;
    getLastRuntimeLogAsFile().close();//connects log into one file ready to send, so it can start logging new session to fresh empty file
}


File RemoteDebug::getLastRuntimeLogAsFile()
{
  File AFile = SPIFFS.open(_loggingFileName + String(1), ("a+"));
  File BFile = SPIFFS.open(_loggingFileName + String(0), ("a+"));
  
  String outputFileName = "";
  if(AFile.size() <= BFile.size())
  {
    BFile.print(F("\n  --  Continuation from previous logFile(s)  -- \n"));
    while(AFile.available())
      BFile.println(AFile.readStringUntil('\n'));
    outputFileName = BFile.name();
    SPIFFS.remove(_loggingFileName + 1);//erase copied log file
    _logingFileFirstOrSecond = 1;
  }
  else
  {
    AFile.print(F("\n  --  Continuation from previous logFile(s)  -- \n"));
    while(BFile.available())
      AFile.println(BFile.readStringUntil('\n'));
    outputFileName = AFile.name();
    SPIFFS.remove(_loggingFileName + 0);//erase copied log file
    _logingFileFirstOrSecond = 0;
  }
  AFile.close();
  BFile.close();
  // Serial.println((String)"DebugOutputFileName: " + outputFileName);
  return SPIFFS.open(outputFileName, ("r"));
}

////// Private

// Show help of commands

void RemoteDebug::showHelp() {

    // Show the initial message

    String help = "";

    help.concat(F("*** Remote debug - over telnet - for ESP8266 (NodeMCU) - version "));
    help.concat(VERSION);
    help.concat(F("\r\n"));
    help.concat(F("* Host name: "));
    help.concat(_hostName);
    help.concat(F(" IP:"));
    help.concat(WiFi.localIP().toString());
    help.concat(F(" Mac address:"));
    help.concat(WiFi.macAddress());
    help.concat(F("\r\n"));
    help.concat(F("* Free Heap RAM: "));
    help.concat(ESP.getFreeHeap());
    help.concat(F("\r\n"));
    help.concat(F("******************************************************\r\n"));
    help.concat(F("* Commands:\r\n"));
    help.concat(F("    ? or help -> display these help of commands\r\n"));
    help.concat(F("    q -> quit (close this connection)\r\n"));
    help.concat(F("    m -> display memory available\r\n"));
    help.concat(F("    R -> Download Response from server\r\n"));
    if (_resetCommandEnabled) {
        help.concat(F("    reset -> reset the ESP8266\r\n"));
    }

    if (_helpProjectCmds != "" && (_callbackProjectCmds)) {
        help.concat(F("\r\n"));
        help.concat(F("    * Project commands:\r\n"));
        String show = "\r\n";
        show.concat(_helpProjectCmds);
        show.replace("\n", "\n    "); // indent this
        help.concat(show);
    }

    help.concat(F("\r\n"));
    help.concat(F("* Please type the command and press enter to execute.(? or h for this help)\r\n"));
    help.concat(F("***\r\n"));

    telnetClient.print(help);
}


// Process user command over telnet
void RemoteDebug::processCommand() {

    telnetClient.print(F("* Debug: Command recevied: "));
    telnetClient.println(_command);

    // Process the command
    if (!strcmp_P(_command.c_str(), PSTR("h")) || !strcmp_P(_command.c_str(), PSTR("?"))) {

        // Show help
        showHelp();

    } else if (!strcmp_P(_command.c_str(), PSTR("q"))) {

        // Quit

        telnetClient.println(F("* Closing telnet connection ..."));

        telnetClient.stop();

    } else if (!strcmp_P(_command.c_str(), PSTR("m"))) {

        telnetClient.print(F("* Free Heap RAM: "));
        telnetClient.println(ESP.getFreeHeap());

    } else if (!strcmp_P(_command.c_str(), PSTR("reset")) && _resetCommandEnabled) {

        telnetClient.println(F("* Reset ..."));

        telnetClient.println(F("* Closing telnet connection ..."));

        telnetClient.println(F("* Resetting the ESP8266 ..."));

        telnetClient.stop();
        telnetServer.stop();

        delay (500);
        // Reset
        restartEsp(F("TelnetReset"));
    }
     else  {
        // Project commands - setted by programmer
        if (_callbackProjectCmds) {
            _callbackProjectCmds();
        }
    }
}

// Format numbers

String RemoteDebug::formatNumber(uint32_t value, uint8_t size, char insert) {

    // Putting zeroes in left

    String ret = "";

    for (uint8_t i=1; i<=size; i++) {
        uint32_t max = pow(10, i);
        if (value < max) {
            for (uint8_t j=(size - i); j>0; j--) {
                ret.concat(insert);
            }
            break;
        }
    }

    ret.concat(value);

    return ret;
}

/////// End
