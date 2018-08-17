//HeatingControl.ino
////////////////////////////////////////////////////////
//BETTER UNDERSTANDING NAMES DEFINITIONS
////////////////////////////////////////////////////////

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
  pinMode(HEATING_SWITCH_ON_SSR_PIN, OUTPUT);

  //tohle tady je aby to neposílalo po restartu heating start, když už heating jede
  GLOBAL.TEMP.heatingState = isElectricityConnected();
  //OChrana relé před nehchráněným spínáním při naskočení elektřiny
  if(isElectricityConnected() == false)
    setHeatingRelayOpen(SET_HEATING_RELAY_CONNECTED);

  yield();
}


void controlHeating_loop()
{
  if (MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:controlHeating_loop(temp): ") + GLOBAL.TEMP.sensors[BOJLER].temp + E("(°C)"));

  if(getTempControleStyle() == BOILER_CONTROL_OFF && isBoilerHeatingOn() == true)
    return turnOffBoilerHeating();

  if(getTempControleStyle() != BOILER_CONTROL_PROGRAMATIC)
  {
    if(RELAY_DEBUG) DEBUG_OUTPUT.println(sE("RELAY_DEBUG: NO heating control on: ") + getTempControleStyleStringName() + E(", returning.."));
    return;
  }

  if(RELAY_DEBUG) DEBUG_OUTPUT.println(sE("GLOBAL.TEMP.lowDroping: ") + GLOBAL.TEMP.lowDroping);
  if(GLOBAL.TEMP.sensors[BOJLER].temp < GLOBAL.TEMP.lowDroping)
    if(isBoilerHeatingOn() == false)
      turnOnBoilerHeating();

  if(RELAY_DEBUG) DEBUG_OUTPUT.println(sE("GLOBAL.TEMP.topHeating: ") + GLOBAL.TEMP.topHeating);
  if(GLOBAL.TEMP.sensors[BOJLER].temp >= GLOBAL.TEMP.topHeating)
    if(isBoilerHeatingOn() == true)
      turnOffBoilerHeating();
}


void turnOnBoilerHeating()
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:turnOnBoilerHeating()"));

  if(getTempControleStyle() == BOILER_CONTROL_OFF)
  {//There has to be brackets {} otherwise it skips else statemnet
  	if(RELAY_DEBUG) DEBUG_OUTPUT.println(sE("RELAY_DEBUG: NO heating control on: ") + getTempControleStyleStringName() + E(", returning.."));
    return;
  }
  else
  {
    if(isElectricityConnected())
      setHeatingRelayOpen(SET_HEATING_RELAY_CONNECTED);
    GLOBAL.TEMP.heatingState = true;
    logNewNodeState(E("Heating: set ON"));
  }
}

void turnOffBoilerHeating()
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:turnOffBoilerHeating()"));
  
  if(getTempControleStyle() == BOILER_CONTROL_MANUAL)
  {//There has to be brackets {} otherwise it skips else statemnet
  	if(RELAY_DEBUG) DEBUG_OUTPUT.println(sE("RELAY_DEBUG: NO heating control on: ") + getTempControleStyleStringName() + E(", returning.."));
    return;
  }
  else
  {
    if(isElectricityConnected())
      setHeatingRelayOpen(SET_HEATING_RELAY_DISCONECTED);
    GLOBAL.TEMP.heatingState = false;
    logNewNodeState(E("Heating: set OFF"));
  }
}


void setTempControleStyle(byte newState)  //1 - arduino(programatic), 2 - Manual(thermostat), 0 - Off eletkricity)
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:setTempControleStyle(newState): ") + (String)TEMP_CONTROL_STYLE[newState]);

  if(newState != getTempControleStyle())
  {
    if(RELAY_DEBUG) DEBUG_OUTPUT.println(sE("---->RELAY_DEBUG: Setting new style of TempControl: ") + TEMP_CONTROL_STYLE[newState]);
    
    GLOBAL.TEMP.boilerControlStyle = newState;    
    logNewNodeState(sE("HeatingControl: ") + TEMP_CONTROL_STYLE[newState]);
    
    if(newState == BOILER_CONTROL_OFF)
    	turnOffBoilerHeating();

    if(newState == BOILER_CONTROL_MANUAL)
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
  return GLOBAL.TEMP.heatingState;
	// return !isBoilerHeatingRelayOpen();
}

bool isBoilerInHeatingProcessNow()
{
  return lastElectricCurrentState_global;
}

void setLastHeatedTemp(float temp)
{
  GLOBAL.TEMP.lastHeated = temp;
}

////////////////////////////////////////////////////////
//RELAY CONTROL
////////////////////////////////////////////////////////

bool setHeatingRelayOpen(bool newState)
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:setHeatingRelayOpen(newState): ") + (String)newState);
  if(RELAY_DEBUG) DEBUG_OUTPUT.println(sE("RELAY_DEBUG: BoilerHeatingRelay: ") + ((isBoilerHeatingRelayOpen()) ? E("ON_RELAY_STATE") : E("OFF_RELAY_STATE")));
  if(RELAY_DEBUG) DEBUG_OUTPUT.println(sE("---->RELAY_DEBUG: BoilerHeatingRelay: ") + ((isBoilerHeatingRelayOpen()) ? E("ON_RELAY_STATE") : E("OFF_RELAY_STATE")) + E(" at temp: ") + GLOBAL.TEMP.sensors[BOJLER].temp);

  digitalWrite(HEATING_SWITCH_ON_SSR_PIN, HIGH);
  
  //check if SSR really works
  if(isElectricityConnected() && isThereCurrentTimeouted(400) == false)
    logNewStateWithEmail(sE("@!!!!HARDWARE ERROR: SSR: Can't turn ON!"));

  digitalWrite(HEATING_SWITCH_OFF_RELAY_PIN, (newState) ? HIGH : LOW);

  delay(500);
  digitalWrite(HEATING_SWITCH_ON_SSR_PIN, LOW);
  delay(100);

  //Check success!
  if(newState == SET_HEATING_RELAY_DISCONECTED && isThereElectricCurrent())
  {
    // kontrola zda neprochází proud po vypnutí ohřevu (SSR or relay FAIL) email + zapnout relé, aby skrz SSR nešlo 10A a nevznítilo se horkem
    setHeatingRelayOpen(false);
    logNewStateWithEmail(sE("@!!!!HARDWARE ERROR: Heating relay: Can't turn OFF!"));
    return false;
  }
  //Check success!
  if(newState == SET_HEATING_RELAY_CONNECTED && isElectricityConnected() && isThereElectricCurrent() == false)
  {
    logNewStateWithEmail(sE("@!!!!HARDWARE ERROR: Heating relay: Can't turn ON!"));
    return false;
  }
  return true;
}


bool isBoilerHeatingRelayOpen()
{
  return digitalRead(HEATING_SWITCH_OFF_RELAY_PIN) == HIGH;
}







