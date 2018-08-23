//time.ino
#include <TimeLib.h>

String getNowTimeDateString()
{
  return getNowDateString() + " " + getNowTimeStringWithSeconds();
}


String getNowTimeString()
{
  // digital clock display of the time
  String timeDateString = "";  
  if(hour() < 10)
    timeDateString += "0";
  timeDateString += hour();
  timeDateString += ":";
  if(minute() < 10)
    timeDateString += "0";
  timeDateString += minute();
  // timeDateString += ":";
  // if(second() < 10)
  //   timeDateString += "0";
  // timeDateString += second(); 

  return timeDateString;   
}

String getNowTimeStringWithSeconds()
{
  // digital clock display of the time
  String timeDateString = getNowTimeString();
  timeDateString += ":";
  if(second() < 10)
    timeDateString += "0";
  timeDateString += second(); 

  return timeDateString;   
}


String getNowDateString() 
{
  // digital clock display of the time
  String timeDateString = "";  
  timeDateString += day();
  timeDateString += ".";
  timeDateString += month();
  timeDateString += ".";
  timeDateString += year();

  return timeDateString;
}

void synchronizeTimeByResponse(String &syncTime)
{
	setTime(syncTime.substring(0,2).toInt(),syncTime.substring(3,5).toInt(),syncTime.substring(6,8).toInt(),syncTime.substring(9,11).toInt(),syncTime.substring(12,14).toInt(),syncTime.substring(15,19).toInt());
}



String getUpTime()
{
  return formatTimeToString(millis(), false);
}

String formatTimeToString(uint32_t timeInMilliseconds)
{
  return formatTimeToString(timeInMilliseconds, true);
}

String formatTimeToString(uint32_t timeInMilliseconds, bool withSeconds)
{
  uint8_t days = 0;
  if(!withSeconds)
    days = timeInMilliseconds / 1000 / 60 / 60 / 24;

  uint8_t hours = timeInMilliseconds / 1000 / 60 / 60 - days * 24;
  uint8_t minutes = timeInMilliseconds / 1000 / 60 - hours * 60;
  uint8_t seconds = timeInMilliseconds / 1000 - minutes * 60 - hours * 60 * 60;
  String separator = F(":");
  String leadingZeroSymbol = F("0");
  if(withSeconds)
    return ((hours <10 ) ? leadingZeroSymbol : "") + (String)hours + separator + ((minutes <10 ) ? leadingZeroSymbol : "") + (String)minutes + separator + ((seconds <10 ) ? leadingZeroSymbol : "") + (String)seconds;
  else
    return (String)days + F("d") + hours + F("h") + minutes + F("m");
}
