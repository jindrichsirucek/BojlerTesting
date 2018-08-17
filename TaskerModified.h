#pragma once
/*
* Tasker for Arduino - cooperative task scheduler
* Copyright (c) 2017  Jindřich Širůček
*/  

#include "define.h"
#define TASKER_DEBUG false

// #ifdef DEBUG_OUTPUT_FOR_TASKER
//   #define DEBUG_OUTPUT DEBUG_OUTPUT_FOR_TASKER
// #else
  // #define DEBUG_OUTPUT Serial
// #endif

// #ifdef TASKER_MAX_TASKS_FOR_TASKER
//   #define TASKER_MAX_TASKS TASKER_MAX_TASKS_FOR_TASKER
// #else
  #define TASKER_MAX_TASKS 15
// #endif


#define TASKER_SKIP_NEVER true
#define TASKER_SKIP_WHEN_NEEDED false

typedef void (*TaskCallback)(int);
typedef void (*TaskCallbackWithoutParam)();

class Tasker
{
public:
  Tasker(Print &normalDebugOutput = Serial, Print &errorDebugOutput = Serial);
  // void setDebugOutput(Print &Print){DEBUG_OUTPUT = &Print;};
  // void setErrorDebugOutput(Print &Print){ERROR_OUTPUT = &Print;};
  bool setTimeout(TaskCallback funcName, unsigned long interval, bool neverSkip, int param = 0);
  bool setInterval(TaskCallback funcName, unsigned long interval, bool neverSkip, int param = 0);
  bool setRepeated(TaskCallback funcName, unsigned long interval, unsigned int repeat, bool neverSkip, int param = 0);
  void setOutroTask(TaskCallbackWithoutParam funcName, bool enabled = true) {outroTask = funcName;  _isOutroTaskEnabled = enabled;}
  void loop(void);
  void run(void) { loop(); }
  void runTask(byte *taskIndex);

private:
  struct TASK {
    TaskCallback call;
    int param;
    unsigned long interval;
    unsigned long nextRunTime;
    unsigned int repeat;
    bool neverSkip;
  };

  TASK tasks[TASKER_MAX_TASKS];
  TaskCallbackWithoutParam outroTask;
  bool _isOutroTaskEnabled = false;
  byte _celkovyPocetUloh = 0;
  Print *DEBUG_OUTPUT;
  Print *ERROR_OUTPUT;
};

#ifdef TASKER_DEBUG
  String getUpTimeInside()
  {
    long timeInMilliseconds = millis();
    byte hoursFromLastSend = timeInMilliseconds / 1000 / 60 / 60;
    byte minutesFromLastSend = timeInMilliseconds / 1000 / 60 - hoursFromLastSend * 60;
    byte secondsFromLastSend = timeInMilliseconds / 1000 - minutesFromLastSend * 60 - hoursFromLastSend * 60 * 60;

    return ((hoursFromLastSend <10 ) ? "0" : "") + (String)hoursFromLastSend + ":" + ((minutesFromLastSend <10 ) ? "0" : "") + (String)minutesFromLastSend + ":" + ((secondsFromLastSend <10 ) ? "0" : "") + (String)secondsFromLastSend;
  }
#endif

void yield_debug_tasker()
{
  // if(TASKER_DEBUG)
  // {
  //   unsigned long beforeYieldTime = millis();
  //   yield();
  //   if((millis() - beforeYieldTime)>2)
  //   DEBUG_OUTPUT->println("yielded time: " + (String)(millis() - beforeYieldTime) + "ms");    
  // }
  // else
    yield();
}


Tasker::Tasker(Print &errorDebugOutput, Print &normalDebugOutput)
{
  ERROR_OUTPUT = &errorDebugOutput;
  DEBUG_OUTPUT = &normalDebugOutput;
}

bool Tasker::setTimeout(TaskCallback funcName, unsigned long interval, bool neverSkip, int param)
{
  return setRepeated(funcName, interval, 1, neverSkip, param);
}

bool Tasker::setInterval(TaskCallback funcName, unsigned long interval, bool neverSkip, int param)
{
  return setRepeated(funcName, interval, 0, neverSkip, param);
}

bool Tasker::setRepeated(TaskCallback funcName, unsigned long interval, unsigned int repeat, bool neverSkip, int param)
{
  if(_celkovyPocetUloh >= TASKER_MAX_TASKS || interval == 0)
  {
    ERROR_OUTPUT->println(E("!!!Error: Tasker: Task was not added! No more space for new task."));
    return false;
  }
  TASK &t = tasks[_celkovyPocetUloh];
  t.call = funcName;
  t.interval = interval;
  t.param = param;
  t.nextRunTime = millis() + interval;
  t.repeat = repeat;
  t.neverSkip = neverSkip;
  _celkovyPocetUloh++;
  return true;
}


#ifdef TASKER_DEBUG
  String LOOP_FUNCTIONS[] =
  {
    "uploadLogFile_loop",
    "silentOTA_loop",
    "displayData_loop",
    "temperature_loop",
    "waterFlow_loop",
    "current_loop",
    "checkSystemState_loop"
  };
#endif

void Tasker::runTask(byte *taskIndexReference)
{
  byte taskIndex = *taskIndexReference;
  #ifdef TASKER_DEBUG
    if (TASKER_DEBUG) DEBUG_OUTPUT->println(getUpTimeInside() + E(" TASKER: Running index: ") + taskIndex + E(" task name: ")+LOOP_FUNCTIONS[taskIndex]+E(" (")+ ((millis() - tasks[taskIndex].nextRunTime ==0 )? cE("!ontime") : (sE("-")+(millis() - tasks[taskIndex].nextRunTime)+E("ms") )) + E(")"));
  #endif

  (*(tasks[taskIndex].call))(tasks[taskIndex].param);//skutečné spuštění funkce
  
  if(--tasks[taskIndex].repeat > 0)
    tasks[taskIndex].nextRunTime = millis() + tasks[taskIndex].interval;// t.nextRunTime += t.interval;
  else
  {
    if (TASKER_DEBUG) DEBUG_OUTPUT->println(getUpTimeInside() + E(" TASKER: Removing task of index: ") + (String)taskIndex);
    // drop the finished task by removing its slot
    memmove(tasks+taskIndex, tasks+taskIndex+1, sizeof(TASK)*(_celkovyPocetUloh-taskIndex-1));
    (*taskIndexReference)--;//decrement pointer value
    _celkovyPocetUloh--;
  }
  
  if(_isOutroTaskEnabled)
    (*(outroTask))();//spuštění outro funkce
}


void Tasker::loop(void)
{
  byte normalTaskIndex = 0;
  unsigned long now = millis();

  while(true)
  {
    yield_debug_tasker();
    
    now = millis();

//Nejdříve projede všechny prioritní úkoly a zjistí zda je některý z nich potřeba udělat
    byte prioritizedTaskIndex = 0;
    while(prioritizedTaskIndex < _celkovyPocetUloh)
    {
      if(tasks[prioritizedTaskIndex].neverSkip == true)
      {
        if(now >= tasks[prioritizedTaskIndex].nextRunTime) 
        {
          runTask(&prioritizedTaskIndex);
          now = millis();
        }
      }
      prioritizedTaskIndex++;
    }
//pokud je to normální úloha
    if(tasks[normalTaskIndex].neverSkip == false)
    {
      if(now >= tasks[normalTaskIndex].nextRunTime) 
        {
          runTask(&normalTaskIndex);
          now = millis();
        }
    }
    //přeskoč na další úkol, případně vynuluj počítadlo pořadí úloh - začínáme opět od první úlohy
    normalTaskIndex = ((normalTaskIndex+1) >= _celkovyPocetUloh) ? 0 : normalTaskIndex+1; 
  }
}




