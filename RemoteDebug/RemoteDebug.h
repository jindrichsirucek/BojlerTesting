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
	bool isThereErrorMessage() {return _isThereErrorMessage;}
	String getLastErrorMessage() {_isThereErrorMessage = false; return _lastErrorWarningMessage;} //if(_lastErrorRepetition > 1) {_lastErrorRepetition = 1; return (_lastErrorWarningMessage + " (" + _lastErrorRepetition + "x)");} else
	File getLastRuntimeLogAsFile();

	bool isThereWarningMessage() {return _isThereWarningMessage;}
	String getLastWarningMessage() {_isThereWarningMessage = false; return getLastErrorMessage();}
	File getWarningsAsFile() {_eraseWarningsFile = true; return SPIFFS.open(_warningsFileName, "a");}
	// void resetErrorLog() {_isThereErrorLog = false; SPIFFS.remove(_loggingFileName + "A"); SPIFFS.remove(_loggingFileName + "B");}

	void setResetCmdEnabled(bool enable);

	void setHelpProjectsCmds(String help);
	void setCallBackProjectCmds(void (*callback)());
	String getLastCommand();

	void showTime(bool show);
	void showProfiler(bool show, uint32_t minTime = 0);
	void showDebugLevel(bool show);
	void showColors(bool show);

	void setFilter(String filter);
	void setNoFilter();

	bool isActive(uint8_t debugLevel = DEBUG);

	// Print

	virtual size_t write(uint8_t);

    // Debug levels

	static const uint8_t VERBOSE = 0;
	static const uint8_t DEBUG = 1;
	static const uint8_t INFO = 2;
	static const uint8_t WARNING = 3;
	static const uint8_t ERROR = 4;

private:

	// Variables

	String _hostName = "";					// Host name

	bool _connected = false;				// Client is connected ?

	uint8_t _clientDebugLevel = DEBUG;		// Level setted by user in telnet
	uint8_t _lastDebugLevel = DEBUG;		// Last Level setted by active()

	bool _showTime = false;				// Show time in millis

	bool _showProfiler = false;			// Show time between messages
	uint32_t _minTimeShowProfiler = 0;		// Minimal time to show profiler

	bool _showDebugLevel = true;			// Show debug Level

	bool _showColors = false;			// Show colors

	bool _serialEnabled = false;			// Send to serial too (not recommended)

	
	bool _logingToFileEnabled = false;
	const String _loggingFileName = "/log";
	const String _warningsFileName;
	bool _logingFileFirstOrSecond = 0;
	bool _isThereErrorMessage = false;
	bool _isThereWarningMessage = false;
	bool _eraseWarningsFile = false;
	String _lastErrorWarningMessage;
	uint8_t _lastErrorRepetition = 1;

	
	bool _resetCommandEnabled = false;	// Command in telnet to reset ESP8266

	bool _newLine = true;				// New line write ?

	String _command = "";					// Command received
	uint32_t _lastTimeCommand = millis();	// Last time command received
	String _helpProjectCmds = "";			// Help of comands setted by project (sketch)
	void (*_callbackProjectCmds)();			// Callable for projects commands

	String _filter = "";					// Filter
	bool _filterActive = false;

	// Privates

	void showHelp();
	void processCommand();
	String formatNumber(uint32_t value, uint8_t size, char insert='0');
	String appendFileToAnother(String AFilePath, String  BFilePath);

};

#endif
