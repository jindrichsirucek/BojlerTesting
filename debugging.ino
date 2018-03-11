//debugging.ino


// ///TODO needs tests - i dont know how to extract form flash, how to use it
// const char gFavicon01[][17] INFLASH  =
// {
//     "DEFAULT",
//     "HARDWARE_WDT",
//     "EXCEPTION",
//     "SOFT_WDT",
//     "SOFT_RESTART",
//     "DEEP_SLEEP_AWAKE",
//     "EXTERNAL_REASON"
// };


#define BM_WDT_SOFTWARE 0
#define BM_WDT_HARDWARE 1
#define BM_ESP_RESTART 2
#define BM_ESP_RESET 3

#define BOOT_MODE BM_WDT_SOFTWARE

struct bootflags
{
  unsigned char raw_rst_cause : 4;
  unsigned char raw_bootdevice : 4;
  unsigned char raw_bootmode : 4;

  unsigned char rst_normal_boot : 1;
  unsigned char rst_reset_pin : 1;
  unsigned char rst_watchdog : 1;

  unsigned char bootdevice_ram : 1;
  unsigned char bootdevice_flash : 1;
};

struct bootflags bootmode_detect(void) 
{
  int reset_reason, bootmode;
  asm (
    "movi %0, 0x60000600\n\t"
    "movi %1, 0x60000200\n\t"
    "l32i %0, %0, 0x114\n\t"
    "l32i %1, %1, 0x118\n\t"
    : "+r" (reset_reason), "+r" (bootmode) /* Outputs */
    : /* Inputs (none) */
    : "memory" /* Clobbered */
  );

  struct bootflags flags;

  flags.raw_rst_cause = (reset_reason & 0xF);
  flags.raw_bootdevice = ((bootmode >> 0x10) & 0x7);
  flags.raw_bootmode = ((bootmode >> 0x1D) & 0x7);

  flags.rst_normal_boot = flags.raw_rst_cause == 0x1;
  flags.rst_reset_pin = flags.raw_rst_cause == 0x2;
  flags.rst_watchdog = flags.raw_rst_cause == 0x4;

  flags.bootdevice_ram = flags.raw_bootdevice == 0x1;
  flags.bootdevice_flash = flags.raw_bootdevice == 0x3;

  return flags;
}

//https://sigmdel.ca/michel/program/esp8266/arduino/watchdogs_en.html
//                  ESP.   ESP.  softw  hardw
//                restart reset   WDT    WDT
// boot mode       (3,6)  (3,6)  (3,6)  (3,6)
// rst cause         2      2      2      4
// rinfo->reason     4      4      3      1
// rfinfo->exccaus   0      0      4      4
// raw_rst_cause     2      2      2      4
// raw_bootdevice    3      3      3      3
// raw_bootmode      6      6      6      6

void printResetInfo()
{
  rst_info* rinfo = ESP.getResetInfoPtr();
  
  DEBUG_OUTPUT.printf(cE("rinfo->reason:   %d, %s\n"), rinfo->reason, ESP.getResetReason().c_str());
  DEBUG_OUTPUT.printf(cE("rinfo->exccause: %d\n"), rinfo->exccause);
  DEBUG_OUTPUT.printf(cE("rinfo->epc1:     %d\n"), rinfo->epc1);
  DEBUG_OUTPUT.printf(cE("rinfo->epc2:     %d\n"), rinfo->epc2);
  DEBUG_OUTPUT.printf(cE("rinfo->epc3:     %d\n"), rinfo->epc3);
  DEBUG_OUTPUT.printf(cE("rinfo->excvaddr: %d\n"), rinfo->excvaddr);
  DEBUG_OUTPUT.printf(cE("rinfo->depc:     %d\n"), rinfo->depc);

  struct bootflags bflags = bootmode_detect();

  DEBUG_OUTPUT.printf(cE("\nbootflags.raw_rst_cause: %d\n"), bflags.raw_rst_cause);
  DEBUG_OUTPUT.printf(cE("bootflags.raw_bootdevice: %d\n"), bflags.raw_bootdevice);
  DEBUG_OUTPUT.printf(cE("bootflags.raw_bootmode: %d\n"), bflags.raw_bootmode);
  DEBUG_OUTPUT.printf(cE("bootflags.rst_normal_boot: %d\n"), bflags.rst_normal_boot);
  DEBUG_OUTPUT.printf(cE("bootflags.rst_reset_pin: %d\n"), bflags.rst_reset_pin);
  DEBUG_OUTPUT.printf(cE("bootflags.rst_watchdog: %d\n"), bflags.rst_watchdog);
  DEBUG_OUTPUT.printf(cE("bootflags.bootdevice_ram: %d\n"), bflags.bootdevice_ram);
  DEBUG_OUTPUT.printf(cE("bootflags.bootdevice_flash: %d\n"), bflags.bootdevice_flash);

  if (bflags.raw_bootdevice == 1) {
    DEBUG_OUTPUT.println(E("The sketch has just been uploaded over the serial link to the ESP8266"));
    DEBUG_OUTPUT.println(E("Beware: the device will freeze after it reboots in the following step."));
    DEBUG_OUTPUT.println(E("It will be necessary to manually reset the device or to power cycle it"));
    DEBUG_OUTPUT.println(E("and thereafter the ESP8266 will continuously reboot."));
  }
}


const char *const FLASH_SIZE_MAP_NAMES[] =
{
    "4M_MAP_256_256",
    "2M",
    "8M_MAP_512_512",
    "16M_MAP_512_512",
    "32M_MAP_512_512",
    "16M_MAP_1024_1024",
    "32M_MAP_1024_1024"
};

extern "C" {
  #include "user_interface.h"
}   

String getESPResetInfo()
{
  struct rst_info *pRstInfo;
  pRstInfo = system_get_rst_info();
  if(pRstInfo->reason != 2)
    return sE("Reboot:") + ESP.getResetReason().c_str();
  else
    return (String(ESP.getResetReason().c_str()) + cE(", exccause=0x") + (pRstInfo->exccause, HEX) + cE(", exccause=0x") + (pRstInfo->epc1, HEX) + cE(", exccause=0x") + (pRstInfo->epc2,HEX) + cE(", exccause=0x") + (pRstInfo->epc3, HEX) + cE(", exccause=0x") + (pRstInfo->excvaddr, HEX) + cE(", exccause=0x") + (pRstInfo->depc, HEX));
}

bool isESPLastResetReasonException()
{
  struct rst_info *pRstInfo;
  pRstInfo = system_get_rst_info();
  return (pRstInfo->reason == 2); //Exception
}

String getESPStatusUpdate()
{
  String infoStrin = E("");

  const rst_info * resetInfo = system_get_rst_info();
  infoStrin += E("\nsystem_get_rst_info() reset reason: ");
  infoStrin += ESP.getResetReason().c_str();
  infoStrin += E("\nfreeHeap: ");
  infoStrin += system_get_free_heap_size();
  infoStrin += E("\nsystem_get_os_print(): ");
  infoStrin += system_get_os_print();
  infoStrin += E("\nsystem_get_os_print(): ");
  infoStrin += system_get_os_print();
  infoStrin += E("\nsystem_get_chip_id(): 0x");
  infoStrin += system_get_chip_id();
  infoStrin += E("\nsystem_get_sdk_version(): ");
  infoStrin += system_get_sdk_version();
  infoStrin += E("\nsystem_get_boot_version(): ");
  infoStrin += system_get_boot_version();
  infoStrin += E("\nsystem_get_userbin_addr(): 0x");
  infoStrin += system_get_userbin_addr();
  infoStrin += E("\nsystem_get_boot_mode(): ");
  infoStrin += (system_get_boot_mode() == 0) ? E("\nSYS_BOOT_ENHANCE_MODE") : E("\nSYS_BOOT_NORMAL_MODE");
  infoStrin += E("\nsystem_get_cpu_freq(): ");
  infoStrin += system_get_cpu_freq();
  infoStrin += E("\nsystem_get_flash_size_map(): ");
  infoStrin += sE("FLASH_SIZE_") + FLASH_SIZE_MAP_NAMES[system_get_flash_size_map()];

  return infoStrin;
}


void yield_debug()
{
  if(YIELD_DEBUG)
  {
    unsigned long beforeYieldTime = millis();
    yield();
    uint16_t timeDiff = millis() - beforeYieldTime;
    // if(timeDiff != 0)
    {
      DEBUG_OUTPUT.println(sE("yielded time: ") + timeDiff + cE("ms"));
    }
  }
  else
    yield();
}


String getUpTimeDebug(uint8_t debugSpacesCountToSet)
{
  spaceCountDebug_global = debugSpacesCountToSet;
  return getUpTimeDebug();
}

String getUpTimeDebug()
{
  spaceCountDebug_global++;
  String debuggerSpaces = E("");
  for(int i=0; i<spaceCountDebug_global; i++)
    debuggerSpaces += cE(">");
  //SYMBOLS: http://www.fileformat.info/info/charset/UTF-8/list.htm?start=3072
  return (String)((WiFi.status() == WL_CONNECTED)? "Ψ - " : "⚠ - ") + getNowTimeStringWithSeconds() + cE(" (") + millis() + cE("ms) (") + ESP.getFreeHeap() + cE(")") + debuggerSpaces;
}


#define measureAnalogVoltage() (int)analogRead(A0)
String getNodeVccString()
{
  char bufferCharConversion[10];               //temporarily holds data from vals
  int rawData = ESP.getVcc();//analogRead(A0);
  dtostrf((float)rawData/1000, 3, 3, bufferCharConversion);  //float value lastTemp_global is copied onto buff bufferCharConversion
       
 return (String)bufferCharConversion + cE("V"); // prints voltage
}


void blinkNotificationLed(uint8_t blinkDuration)
{
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  delay(blinkDuration);
  digitalWrite(LED_PIN, HIGH);
  delay(SHORT_BLINK);
}

void turnNotificationLedOn() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
}

void turnNotificationLedOff() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
}


void restartEsp()
{
  //https://github.com/esp8266/Arduino/issues/1722
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:restartEsp()"));
  logNewNodeState(E("ESP: restart"));
  digitalWrite(0, HIGH);//When gpio0 used as output, need to by high before reseting ESP
  yield_debug();
  ESP.restart();
}

bool sendDebugInformationsAfterReset()
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug(RESET_SPACE_COUNT_DEBUG) + E("F:sendDebugInformationsAfterReset()"));
  String uri = getESPResetInfo();
  showServiceMessage(uri);

  uri = sE("&quickEvent=") + URLEncode(uri);
  uri += stringifyTempSensorAddressesStruct();

  return uploadDebugLogFileWithGetParams(uri);
}
