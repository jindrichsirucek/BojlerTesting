////////
// Header for RemoteDebug
///////

#ifndef RemoteDebug_h
#define RemoteDebug_h

#include "Arduino.h"
#include "Print.h"
#include "FS.h"

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

// Port for telnet

#define TELNET_PORT 23

// Maximun time for inactivity (em miliseconds)
// Default: 10 minutes
// Comment it if you not want this

// #define MAX_TIME_INACTIVE 600000

// Buffered print write to telnet -> length of buffer

#define BUFFER_PRINT 150

// Class

class RemoteDebug: public Print
{
	public:
		void begin(String hostName);
		void stop();
		void handle();
		void setSerialEnabled(bool enable);
		void setLogFileEnabled(bool enable);
		File getLastRuntimeLogAsFile();


//Oneliners
     	String getLastErrorMessage() {_isThereErrorMessage = false; return "";} //if(_lastErrorRepetition > 1) {_lastErrorRepetition = 1; return (_lastErrorWarningMessage + " (" + _lastErrorRepetition + "x)");} else

     	bool isThereWarningMessage() {bool returnValue = _isThereWarningMessage; _isThereWarningMessage = false; return returnValue;}
     	bool isThereErrorMessage() {bool returnValue = _isThereErrorMessage; _isThereErrorMessage = false; return returnValue;}
	
     	File getErrorFile() {return SPIFFS.open(_warningsFileName, ("r"));}
     	bool removeErrorLogFile() {_isThereWarningMessage = false; _isThereErrorMessage = false; return SPIFFS.remove(_warningsFileName);}
	    // void clearLogFiles() { SPIFFS.remove(_loggingFileName + 0); SPIFFS.remove(_loggingFileName + 1);};
//END Oneliners


     	void setResetCmdEnabled(bool enable);
     	void setHelpProjectsCmds(String help);
     	void setCallBackProjectCmds(void (*callback)());
	

	// Print
     	virtual size_t write(uint8_t);

private:

	// Variables

	String _hostName = "";					// Host name

	bool _connected = false;				// Client is connected ?
	bool _serialEnabled = false;			// Send to serial too (not recommended)

	
	bool _logingToFileEnabled = false;
	const char* _loggingFileName = ("/debugLog_");
	const char* _warningsFileName = ("/err.log");
	bool _logingFileFirstOrSecond = 0;
	bool _isThereErrorMessage = false;
	bool _isThereWarningMessage = false;

	bool _resetCommandEnabled = false;	// Command in telnet to reset ESP8266

	bool _newLine = true;				// New line write ?

	String _command = "";					// Command received
	uint32_t _lastTimeCommand = millis();	// Last time command received
	String _helpProjectCmds = "";			// Help of comands setted by project (sketch)
	void (*_callbackProjectCmds)();			// Callable for projects commands

	// Privates

	void showHelp();
	void processCommand();
	String formatNumber(uint32_t value, uint8_t size, char insert='0');
	String appendFileToAnother(String AFilePath, String  BFilePath);
};

#endif
