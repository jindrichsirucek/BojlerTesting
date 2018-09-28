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

String getUpTime(bool withSeconds)
{
  return formatTimeToString(millis(), withSeconds);
}

String formatTimeToString(uint32_t timeInMilliseconds)
{
  return formatTimeToString(timeInMilliseconds, false);
}

String formatTimeToString(uint32_t timeInMilliseconds, bool withSeconds)
{
  //Rolover: http://forum.arduino.cc/index.php?topic=42211.0
  uint32_t timeInSeconds = timeInMilliseconds/1000;
  uint8_t days = elapsedDays(timeInSeconds);
  uint8_t hours = numberOfHours(timeInSeconds);
  uint8_t minutes = numberOfMinutes(timeInSeconds);
  uint8_t seconds = numberOfSeconds(timeInSeconds);

  String separator = F(":");
  String leadingZeroSymbol = F("0");
  if(withSeconds)
    return hours+=days*24, ((hours <10 ) ? leadingZeroSymbol : "") + (String)(hours) + separator + ((minutes <10 ) ? leadingZeroSymbol : "") + (String)minutes + separator + ((seconds <10 ) ? leadingZeroSymbol : "") + (String)seconds;
  else
    return (String)days + F("d") + hours + F("h") + minutes + F("m");
}





