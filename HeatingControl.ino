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
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + F("F:relayBoard_setup()"));
  pinMode(HEATING_SWITCH_OFF_RELAY_PIN, OUTPUT);

  //!!!! LOW - turn on a relay, HIGH it realeses!! !!!
  digitalWrite(HEATING_SWITCH_OFF_RELAY_PIN, HIGH);//default state
}


void controlHeating_loop(float namerenaTeplotaVBojleru)
{
  if (MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + F("F:controlHeating_loop(namerenaTeplotaVBojleru): ") + (String)namerenaTeplotaVBojleru + "(°C)");

  if(getTempControleStyle() != ARDUINO_STYLE_CONTROL)
  {
    if(RELAY_DEBUG) DEBUG_OUTPUT.println("RELAY_DEBUG: NO heating control on: " + getTempControleStyleStringName() + ", returning..");
    return;
  }

  if (namerenaTeplotaVBojleru < lowDropingTemp_global)
    if(isBoilerHeatingOn() == false)
      turnOnBoilerHeating();

  if(namerenaTeplotaVBojleru >= topHeatingTemp_global)
    if(isBoilerHeatingOn() == true)
      turnOffBoilerHeating();
}


void turnOnBoilerHeating()
{
  i2cBus_setup(); // to reset display
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + F("F:turnOnBoilerHeating()"));
  
  if(getTempControleStyle() == OFF_STYLE_CONTROL)
  {
  	if(RELAY_DEBUG) DEBUG_OUTPUT.println("RELAY_DEBUG: NO heating control on: " + getTempControleStyleStringName() + ", returning..");
  	return;
  }
  else  
  {
    if(setOpenBoilerHeatingRelay(false))
      logNewNodeState(F("Heating: set ON"));
  }
}

void turnOffBoilerHeating()
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + F("F:turnOffBoilerHeating()"));
  
  if(getTempControleStyle() == MANUAL_STYLE_CONTROL)
  {
  	if(RELAY_DEBUG) DEBUG_OUTPUT.println("RELAY_DEBUG: NO heating control on: " + getTempControleStyleStringName() + ", returning..");
  	return;
  }
  else
  	if(setOpenBoilerHeatingRelay(true))
	    logNewNodeState(F("Heating: set OFF"));
}


void setTempControleStyle(byte newState)  //1 - arduino(programatic), 2 - Manual(thermostat), 0 - Off eletkricity)
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + F("F:setTempControleStyle(newState): ") + (String)TEMP_CONTROL_STYLE[newState]);

  if(newState != getTempControleStyle())
  {
    if(RELAY_DEBUG) DEBUG_OUTPUT.println("---->RELAY_DEBUG: Setting new style of TempControl: " + (String)TEMP_CONTROL_STYLE[newState]);
    
    boilerTempControlStyle_global = newState;    
    logNewNodeState((String)"TempControl_" + (String)TEMP_CONTROL_STYLE[newState]);
    
    if(newState == OFF_STYLE_CONTROL)
    	turnOffBoilerHeating();

	if(newState == MANUAL_STYLE_CONTROL)
		turnOnBoilerHeating();
  }
  else
    if(RELAY_DEBUG) DEBUG_OUTPUT.println("---->RELAY_DEBUG: Already set ProgramaticTempControl: " + (String)TEMP_CONTROL_STYLE[newState]);
}


byte getTempControleStyle()
{
  return boilerTempControlStyle_global;
}


String getTempControleStyleStringName()
{
  return TEMP_CONTROL_STYLE[boilerTempControlStyle_global];
}

bool isBoilerHeatingOn()
{
	return !isBoilerHeatingRelayOpen();
}


////////////////////////////////////////////////////////
//RELAY CONTROL
////////////////////////////////////////////////////////
bool setOpenBoilerHeatingRelay(bool newState)
{
  bool relayStateChanged = false;

  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + F("F:setOpenBoilerHeatingRelay(newState): ") + (String)newState);

  if(RELAY_DEBUG) DEBUG_OUTPUT.println("RELAY_DEBUG: BoilerHeatingRelay: " + (String)((isBoilerHeatingRelayOpen()) ? "ON_RELAY_STATE" : "OFF_RELAY_STATE"));
  if(isBoilerHeatingRelayOpen() ^ newState)
  {
    if(RELAY_DEBUG) DEBUG_OUTPUT.println("---->RELAY_DEBUG: BoilerHeatingRelay: " + (String)((isBoilerHeatingRelayOpen()) ? "ON_RELAY_STATE" : "OFF_RELAY_STATE") + " at temp: " + (String)lastTemp_global);
    digitalWrite(HEATING_SWITCH_OFF_RELAY_PIN, (newState) ? LOW : HIGH);
    relayStateChanged = true;
  }
  return relayStateChanged;
}


bool isBoilerHeatingRelayOpen()
{
  return (digitalRead(HEATING_SWITCH_OFF_RELAY_PIN)) ? false : true;
}







