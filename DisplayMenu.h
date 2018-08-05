#pragma once
/*
* DisplayMenu for Arduino - cooperative task scheduler
* Copyright (c) 2017  Jindřich Širůček
*/  

#define DISPLAY_MENU_DEBUG false
#ifdef DEBUG_OUTPUT
  #define INTERNAL_DEBUG_OUTPUT DEBUG_OUTPUT 
#else
  #define INTERNAL_DEBUG_OUTPUT Serial
#endif
  //#define INTERNAL_DEBUG_OUTPUT debugFile

#ifndef DISPLAY_MENU_MAX_ENTRIES
  #define DISPLAY_MENU_MAX_ENTRIES  10
#endif

typedef void (*FunctionCallback)();

class DisplayMenu
{
public:
  bool addEntry(FunctionCallback funcName, String entryName, bool preSelected = false);
  String move(bool reverse = false);
  String getSelectedEntryName();
  void runSelectedMenuEntry();

private:
  struct DisplayMenuEntriesStruct {
    FunctionCallback call;
    String entryName;
  };

  DisplayMenuEntriesStruct menuEntries[DISPLAY_MENU_MAX_ENTRIES];
  uint8_t _menuEntriesCount = 0;
  uint8_t _selectedEntry = 0;
};


bool DisplayMenu::addEntry(FunctionCallback funcName, String entryName, bool preSelected)
{
  if(_menuEntriesCount >= DISPLAY_MENU_MAX_ENTRIES)
    return false;
  
  menuEntries[_menuEntriesCount].entryName = entryName;
  menuEntries[_menuEntriesCount].call = funcName;

  if(preSelected)
    _selectedEntry = _menuEntriesCount;

  _menuEntriesCount++;
  return true;
}

String DisplayMenu::getSelectedEntryName()
{
  if (DISPLAY_MENU_DEBUG) DEBUG_OUTPUT.printf("MENU: getSelectedEntryName (%d): %s\n",_selectedEntry, menuEntries[_selectedEntry].entryName.c_str());
  return menuEntries[_selectedEntry].entryName;
}

String DisplayMenu::move(bool reverse)
{
  if(reverse)
  {//Move Up (reverese direction)
    if(--_selectedEntry == 0)
      _selectedEntry = _menuEntriesCount-1;
  }
  else
  {//Move down
    if(++_selectedEntry >= _menuEntriesCount)
      _selectedEntry = 0;
  }
  if (DISPLAY_MENU_DEBUG) DEBUG_OUTPUT.printf("MENU: moved to entry: %d name: %s\n",_selectedEntry, menuEntries[_selectedEntry].entryName.c_str());
  return menuEntries[_selectedEntry].entryName;
};

void DisplayMenu::runSelectedMenuEntry()
{
  if (DISPLAY_MENU_DEBUG) INTERNAL_DEBUG_OUTPUT.println((String)F("\nMENU: pressed: ") + menuEntries[_selectedEntry].entryName);
  if(_menuEntriesCount == 0)
    return;

  (*(menuEntries[_selectedEntry].call))();//Function call
}
