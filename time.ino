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
  return formatTimeToString(millis());
}


String formatTimeToString(uint32_t timeInMilliseconds)
{
  byte hoursFromLastSend = timeInMilliseconds / 1000 / 60 / 60;
  byte minutesFromLastSend = timeInMilliseconds / 1000 / 60 - hoursFromLastSend * 60;
  byte secondsFromLastSend = timeInMilliseconds / 1000 - minutesFromLastSend * 60 - hoursFromLastSend * 60 * 60;

  return ((hoursFromLastSend <10 ) ? "0" : "") + (String)hoursFromLastSend + ":" + ((minutesFromLastSend <10 ) ? "0" : "") + (String)minutesFromLastSend + ":" + ((secondsFromLastSend <10 ) ? "0" : "") + (String)secondsFromLastSend;
}
