//HeatingControl.ino
////////////////////////////////////////////////////////
//BETTER UNDERSTANDING NAMES DEFINITIONS
////////////////////////////////////////////////////////
#define ON_RELAY_STATE true
#define OFF_RELAY_STATE false


#define OFF_STYLE_CONTROL 0
#define ARDUINO_STYLE_CONTROL 1
#define MANUAL_STYLE_CONTROL 2

const char * const TEMP_CONTROL_STYLE[] =
{
  "OFF",
  "ARDUINO",
  "MANUAL"
};

void relayBoard_setup()
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug(RESET_SPACE_COUNT_DEBUG) + E("F:relayBoard_setup()"));
  pinMode(HEATING_SWITCH_OFF_RELAY_PIN, OUTPUT);

  //!!!! LOW - turn on a relay, HIGH it realeses!! !!!
  digitalWrite(HEATING_SWITCH_OFF_RELAY_PIN, isElectricityConnected()? HIGH : LOW);//default state
  yield();
}


void controlHeating_loop(float namerenaTeplotaVBojleru)
{
  if (MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:controlHeating_loop(namerenaTeplotaVBojleru): ") + namerenaTeplotaVBojleru + E("(°C)"));

  if(getTempControleStyle() != ARDUINO_STYLE_CONTROL)
  {
    if(RELAY_DEBUG) DEBUG_OUTPUT.println(sE("RELAY_DEBUG: NO heating control on: ") + getTempControleStyleStringName() + E(", returning.."));
    return;
  }

  if(RELAY_DEBUG) DEBUG_OUTPUT.println(sE("GLOBAL.TEMP.lowDroping: ") + GLOBAL.TEMP.lowDroping);
  if(namerenaTeplotaVBojleru < GLOBAL.TEMP.lowDroping)
    if(isBoilerHeatingOn() == false)
      turnOnBoilerHeating();

  if(RELAY_DEBUG) DEBUG_OUTPUT.println(sE("GLOBAL.TEMP.topHeating: ") + GLOBAL.TEMP.topHeating);
  if(namerenaTeplotaVBojleru >= GLOBAL.TEMP.topHeating)
    if(isBoilerHeatingOn() == true)
      turnOffBoilerHeating();
}


void turnOnBoilerHeating()
{
  lcd_setup(); // to reset display
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:turnOnBoilerHeating()"));
  // if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:turnOnBoilerHeating()2"));

  if(getTempControleStyle() == OFF_STYLE_CONTROL)
  {//There has to be brackets {} otherwise it skips else statemnet
  	if(RELAY_DEBUG) DEBUG_OUTPUT.println(sE("RELAY_DEBUG: NO heating control on: ") + getTempControleStyleStringName() + E(", returning.."));
  }
  else  
  {
    // if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:turnOnBoilerHeating()3"));
    setOpenBoilerHeatingRelay(false);
    // if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:turnOnBoilerHeating()4"));
  }
  // if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:turnOnBoilerHeating()5"));
}

void turnOffBoilerHeating()
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:turnOffBoilerHeating()"));
  
  if(getTempControleStyle() == MANUAL_STYLE_CONTROL)
  {//There has to be brackets {} otherwise it skips else statemnet
  	if(RELAY_DEBUG) DEBUG_OUTPUT.println(sE("RELAY_DEBUG: NO heating control on: ") + getTempControleStyleStringName() + E(", returning.."));
  }
  else
  	setOpenBoilerHeatingRelay(true);
}


void setTempControleStyle(byte newState)  //1 - arduino(programatic), 2 - Manual(thermostat), 0 - Off eletkricity)
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:setTempControleStyle(newState): ") + (String)TEMP_CONTROL_STYLE[newState]);

  if(newState != getTempControleStyle())
  {
    if(RELAY_DEBUG) DEBUG_OUTPUT.println(sE("---->RELAY_DEBUG: Setting new style of TempControl: ") + TEMP_CONTROL_STYLE[newState]);
    
    GLOBAL.TEMP.boilerControlStyle = newState;    
    logNewNodeState(sE("TempControl_") + TEMP_CONTROL_STYLE[newState]);
    
    if(newState == OFF_STYLE_CONTROL)
    	turnOffBoilerHeating();

    if(newState == MANUAL_STYLE_CONTROL)
      turnOnBoilerHeating();
  }
  else
    if(RELAY_DEBUG) DEBUG_OUTPUT.println(sE("---->RELAY_DEBUG: Already set ProgramaticTempControl: ") + TEMP_CONTROL_STYLE[newState]);
}


byte getTempControleStyle()
{
  return GLOBAL.TEMP.boilerControlStyle;
}


String getTempControleStyleStringName()
{
  return TEMP_CONTROL_STYLE[GLOBAL.TEMP.boilerControlStyle];
}

bool isBoilerHeatingOn()
{
	return !isBoilerHeatingRelayOpen();
}

void setLastHeatedTemp()
{
  GLOBAL.TEMP.lastHeated = GLOBAL.TEMP.sensors[BOJLER].lastTemp;
}

////////////////////////////////////////////////////////
//RELAY CONTROL
////////////////////////////////////////////////////////
bool setOpenBoilerHeatingRelay(bool newState)
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:setOpenBoilerHeatingRelay(newState): ") + (String)newState);
  bool relayStateChanged = false;

  if(RELAY_DEBUG) DEBUG_OUTPUT.println(sE("RELAY_DEBUG: BoilerHeatingRelay: ") + ((isBoilerHeatingRelayOpen()) ? E("ON_RELAY_STATE") : E("OFF_RELAY_STATE")));
  if(isBoilerHeatingRelayOpen() != newState)
  {
    if(RELAY_DEBUG) DEBUG_OUTPUT.println(sE("---->RELAY_DEBUG: BoilerHeatingRelay: ") + ((isBoilerHeatingRelayOpen()) ? E("ON_RELAY_STATE") : E("OFF_RELAY_STATE")) + E(" at temp: ") + GLOBAL.TEMP.sensors[BOJLER].temp);
    digitalWrite(HEATING_SWITCH_OFF_RELAY_PIN, (newState) ? LOW : HIGH);
    relayStateChanged = true;
    logNewNodeState(sE("Heating: set ") + ((newState)? E("OFF") : E("ON")));
  }
  return relayStateChanged;
}


bool isBoilerHeatingRelayOpen()
{
  return (digitalRead(HEATING_SWITCH_OFF_RELAY_PIN)) ? false : true;
}







