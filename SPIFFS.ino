//SPIFFS.ino
#include "FS.h"


void SPIFFS_setup()
{
   bool spiffsLoaded = SPIFFS.begin();
   Serial.println((spiffsLoaded) ? "FS OK!" : "FS not loaded");
   if(spiffsLoaded == false)
   {
     logNewErrorState(F("SPIFFS not loaded - formating and restarting ESP"));
     formatSpiffs();
     ESP.reset();
   }


   FSInfo fs_info;
   SPIFFS.info(fs_info);
   getFreeSpaceSPIFFS();

   Serial.print("blockSize: ");
   Serial.println(fs_info.blockSize);
   Serial.print("pageSize: ");
   Serial.println(fs_info.pageSize);
   Serial.print("maxOpenFiles: ");
   Serial.println(fs_info.maxOpenFiles);
   Serial.print("maxPathLength: ");
   Serial.println(fs_info.maxPathLength);
   Serial.print("total space: ");
   Serial.print(fs_info.totalBytes/1000);
   Serial.print("kB  used: ");
   Serial.print(fs_info.usedBytes/1000);
   Serial.print("kB  free: ");
   Serial.print((fs_info.totalBytes-fs_info.usedBytes)/1000);
   Serial.println("kB");
   
   Dir dir = SPIFFS.openDir("/");
   
   while (dir.next()) 
   {
      Serial.print(dir.fileName());
      Serial.print(" - size: ");
      File f = dir.openFile("r");
      Serial.println(f.size());
   }

   Serial.println("");
   Serial.println("");
}

uint32_t getFreeSpaceSPIFFS()
{
   FSInfo fs_info;
   SPIFFS.info(fs_info);
   return fs_info.totalBytes-fs_info.usedBytes;
}


bool formatSpiffs()
{
   DEBUG_OUTPUT.print("Formating File System..");
   bool success = SPIFFS.format();
   DEBUG_OUTPUT.println(success ? "Done!":"!!!Error ocured.");
   curentLogNumber_global = 0;
   return success;
}
// void printDebugFile_loop(int)
// {

//    debugFile.close();

//    Serial.println("Opening File");
//    debugFile = openFile("/test.txt", "r");
//    Serial.println(debugFile);


//    debugFile.setTimeout(4000);

//    while(debugFile.available())
//    Serial.println(debugFile.readStringUntil('\n'));

//    debugFile.close();

//    debugFile = openFile("/test.txt", "a");

// }

// void logEventIntoFile(String postDataToSave, String getDataToSave)
// {
//    DynamicJsonBuffer  jsonBuffer;

//    JsonObject& root = jsonBuffer.createObject();

//    root["getData"] = getDataToSave;
//    root["postData"] = postDataToSave;

//    root.printTo(Serial);

//    File eventLogFile = openFile("/test.txt", "a");
//    eventLogFile.println("New line: ");
//    Serial.print("saving to file: ");
//    root.printTo(eventLogFile);
//    Serial.println("");
//    // eventLogFile.close();


//    // Serial.println("Opening file: ");
//    // eventLogFile = openFile("/test.txt", "r");

//    // while(eventLogFile.available())
//    // Serial.println(eventLogFile.readStringUntil('\n'));


//    // eventLogFile.close();
// }






