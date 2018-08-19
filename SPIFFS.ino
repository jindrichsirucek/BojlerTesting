//SPIFFS.ino
#include "FS.h"


void SPIFFS_setup()
{
   Serial.print(E("\nSPIFFS loading: "));

   if(!ESP.checkFlashConfig())
    Serial.println(sE("Wrong Flash config: ESP.checkFlashConfig() = false"));
  

   String realSize = String(ESP.getFlashChipRealSize());
   String ideSize = String(ESP.getFlashChipSize());
   if(!realSize.equals(ideSize))
    Serial.println("Flash incorrectly configured, SPIFFS cannot start, IDE size: " + ideSize + ", real size: " + realSize), ESP.restart();

   yield();
   bool spiffsLoaded = SPIFFS.begin();
   Serial.println((spiffsLoaded) ? E("OK!") : E("NOT loaded!"));
   if(spiffsLoaded == false)
   {
     Serial.println(E("SPIFFS not loaded - formating and restarting ESP"));
     formatSpiffs();
     ESP.restart();
   }

   // spiffs_object_index_consistency_check();

   spiffsConsistencyCheck();

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


bool spiffsConsistencyCheck()
{
   //SPIFFS CONSISTENCY CHECK
   Serial.println(E("SPIFFS reading consistency check: "));

   // Check each file for reading firt line
   Dir dir = SPIFFS.openDir("/");
   while(dir.next())
   {
      yield();
      File f = dir.openFile("r");
      if(f.size() == 0)
      {
         Serial.println((String)f.name() + E(" - size: ") + f.size() + E(" - Deleting!!"));
         removeFileOrFormatSPIFSS(f);
         continue;
      }

      if(!f.available())
      {
         Serial.println((String)f.name() + E(" - size: ") + f.size() + E(" - NOT AVAILABLE - Deleting!!"));
         removeFileOrFormatSPIFSS(f);
         continue;
      }

      String loadedChar = String(f.readStringUntil('\n'));
      // Serial.println(sE("length: ") + loadedChar.length());
      if(loadedChar.length() >= 1)
      {
        Serial.println((String)f.name() + E(" - size: ") + f.size() + E(" - OK") + E(" (") + loadedChar + E(")"));
      }
      else
      {
        Serial.println((String)f.name() + E(" - size: ") + f.size() + E(" - LENGTH ZERO - Deleting!!"));
        removeFileOrFormatSPIFSS(f);
        continue;
     }
     f.close();
   }

   Serial.print(E("SPIFFS writing consistency check: "));
   File writeTestFile = SPIFFS.open("/testFile","w");
   if(writeTestFile)
   {
      String randomNumber = (String)random(1000000);
      writeTestFile.println(randomNumber);
      writeTestFile.close();

      SPIFFS.end();
      delay(100);
      SPIFFS.begin();

      writeTestFile = SPIFFS.open("/testFile","r");
      String readString = writeTestFile.readStringUntil('\n');

      Serial.print(sE("(") + randomNumber + "&" + readString + E("): "));

      if(randomNumber.equals(readString) != 0 || SPIFFS.remove("/testFile") == false)
      {
         Serial.println(E("Problem!! Formating SPIFFS.."));
         SPIFFS.format();
      }
      else
        Serial.println(E("OK!"));

      return true;
   }
   else
   {
      Serial.println(E("Problem!! Formating SPIFFS.."));
      SPIFFS.format();
   }
   return false;
}

bool removeFileOrFormatSPIFSS(File f)
{
   String fileName = f.name();
   f.close();
   if(SPIFFS.remove(fileName))
     return true;
   else
   {
      SPIFFS.format();
      restartEsp(E("SPIFFS check error"));
   }
   return false;
}

void printAllSpiffsFiles()
{
   String divider = E("  --------------  ");
   Dir dir = SPIFFS.openDir("/");
   while(dir.next())
   {
      Serial.println(divider + dir.fileName() + E(" - size: ") + dir.fileSize() +  divider + E("'"));

      if(dir.fileSize() > 0 && dir.fileSize() < MAXIM_FILE_LOG_SIZE)
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






