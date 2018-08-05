//SPIFFS.ino
#include "FS.h"


void SPIFFS_setup()
{
   Serial.print(E("\nSPIFFS loading: "));
   yield();
   bool spiffsLoaded = SPIFFS.begin();
   Serial.println((spiffsLoaded) ? E("OK!") : E("NOT loaded!"));
   if(spiffsLoaded == false)
   {
     Serial.println(E("SPIFFS not loaded - formating and restarting ESP"));
     formatSpiffs();
     ESP.restart();
   }

   FSInfo fs_info;
   SPIFFS.info(fs_info);

   if(SPIFFS_DEBUG)
   {
      Serial.print(E("ChipRealSize: "));
      Serial.println(ESP.getFlashChipRealSize());
      Serial.print(E("blockSize: "));
      Serial.println(fs_info.blockSize);
      Serial.print(E("pageSize: "));
      Serial.println(fs_info.pageSize);
      Serial.print(E("maxOpenFiles: "));
      Serial.println(fs_info.maxOpenFiles);
      Serial.print(E("maxPathLength: "));
      Serial.println(fs_info.maxPathLength);
   }

   Serial.print(E("Total space: "));
   Serial.print(fs_info.totalBytes/1024);
   Serial.print(E("kB  Used: "));
   Serial.print(fs_info.usedBytes/1024);
   Serial.print(E("kB  Free: "));
   Serial.print((fs_info.totalBytes-fs_info.usedBytes)/1024);
   Serial.println(E("kB"));
   Serial.println();

   Serial.println(E("/(ROOT)"));
   Dir dir = SPIFFS.openDir("/");
   dir.next();
   while(true)
   {
      String line = dir.fileName() + E(" - size: ") + dir.fileSize();
      bool isLastItem = !dir.next();
      Serial.println(String(isLastItem ? E(" ╚⟹") : E(" ╠⟹")) + line);
      if(isLastItem) 
        break;
      yield();  
   }

   
   if(SPIFFS_DEBUG)
     printAllSpiffsFiles();
   Serial.println();
   Serial.println();
   yield();
}


void printAllSpiffsFiles()
{
   String divider = E("  --------------  ");
   Dir dir = SPIFFS.openDir("/");
   while(dir.next())
   {
      Serial.println(divider + dir.fileName() + E(" - size: ") + dir.fileSize() +  divider + E("'"));

      if(dir.fileSize() < MAXIM_FILE_LOG_SIZE)
      {
         File f = dir.openFile("r");
         while(yield(), f.available())
         Serial.println(f.readStringUntil('\n'));
         f.close();
      }
      else
      Serial.println(E("__ Too big fileSize to list file content! __"));

      Serial.println("'\n"+divider + divider + divider + "\n"+divider + divider + divider + "\n"+divider + divider + divider + "\n");
   }
}


uint32_t getFreeSpaceSPIFFS()
{
   FSInfo fs_info;
   SPIFFS.info(fs_info);
   return fs_info.totalBytes-fs_info.usedBytes;
}


bool formatSpiffs()
{
   DEBUG_OUTPUT.print(E("Formating File System.."));
   bool success = SPIFFS.format();
   DEBUG_OUTPUT.println(success ? E("Done!"):E("!!!Error ocured."));
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






