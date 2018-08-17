
// #define PULSES_PER_LITER_WATER_SENSOR 660 // model: G3/4 (smaller) - FS300A - 330 pulses per liter
#define PULSES_PER_LITER_WATER_SENSOR 369*2 // model: G1/2 (bigger) - YF-S201 - 450 pulses per liter *2 becouse measuring both rising and falling edges. 369 changed value to precise measuring
//recounted during test with CHANGE - 369 * 2

void waterFlowSensor_setup()
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug(RESET_SPACE_COUNT_DEBUG) + E("F:waterFlowSensor_setup()"));

  pinMode(WATER_FLOW_SENSOR_PIN, INPUT_PULLUP);
  // attachInterrupt(digitalPinToInterrupt(WATER_FLOW_SENSOR_PIN), ISR_flowCount, RISING); // Setup Interrupt  // see http://arduino.cc/en/Reference/attachInterrupt
  attachInterrupt(digitalPinToInterrupt(WATER_FLOW_SENSOR_PIN), ISR_flowCount, CHANGE); // 2 times more impulses for more acurate measurment
  sei(); // Enable interrupts
  yield();
}

float readFlowInLitres()
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:readFlowInLitres()"));

  if(waterFlowSensorCount_ISR_global > lastWaterFlowSensorCount_global)
  {
    lastWaterFlowSensorCount_global = waterFlowSensorCount_ISR_global;
    float spotreba = convertWaterFlowSensorImpulsesToLitres(lastWaterFlowSensorCount_global);

    if(WATER_FLOW_DEBUG) DEBUG_OUTPUT.println(sE("Otacky: ") + waterFlowSensorCount_ISR_global);
    if(WATER_FLOW_DEBUG) DEBUG_OUTPUT.print(E("Spotreba: "));
    if(WATER_FLOW_DEBUG) DEBUG_OUTPUT.print(spotreba, 4);
    if(WATER_FLOW_DEBUG) DEBUG_OUTPUT.println(E(" L"));
    if(WATER_FLOW_DEBUG) DEBUG_OUTPUT.print(E("Celková spotřeba v litrech: "));
    if(WATER_FLOW_DEBUG) DEBUG_OUTPUT.println(waterFlowDisplay_global/PULSES_PER_LITER_WATER_SENSOR);

    return spotreba;
  }
  return 0;
}


String getSpareHotWaterString()
{
  char bufferCharConversion[10]; //temporarily holds data from vals
  dtostrf(getSpareHotWater(), 1, 1, bufferCharConversion);  //first num is mininum width, second is precision
  return (String)bufferCharConversion;
}

float getSpareHotWater()
{
  return convertWaterFlowSensorImpulsesToLitres((float)BOILER_SIZE_LITRES*PULSES_PER_LITER_WATER_SENSOR-(waterFlowDisplay_global + lastWaterFlowSensorCount_global + waterFlowSensorCount_ISR_global));
}

#define COMFORT_TEMPERATURE 40
uint16_t getSpareComfortWater()
{
  if(!isTemperatureCorrectMeasurment(GLOBAL.TEMP.lastHeated) || GLOBAL.TEMP.lastHeated < COMFORT_TEMPERATURE)
  return 0;

  float coldWaterTemparature = 20.0;
  float hotWaterLitres = getSpareHotWater();
  uint16_t result;

  result = (hotWaterLitres * (GLOBAL.TEMP.lastHeated - COMFORT_TEMPERATURE) / (COMFORT_TEMPERATURE - coldWaterTemparature)) + hotWaterLitres;
  return result;
}


void ICACHE_RAM_ATTR ISR_flowCount()                  // Interrupt function
{
  waterFlowSensorCount_ISR_global++;
}

/*
void disableInterupts()                  // Interrupt function
{
  detachInterrupt(digitalPinToInterrupt(WATER_FLOW_SENSOR_PIN));
  detachedTime_global = millis();
}

void enableInterupts()                  // Interrupt function
{
  unsigned long timeDiff = millis() - detachedTime_global;

  unsigned long odPoslednihoRestartuPocitadlaUbehlo = millis() - lastWaterFlowResetTime_global;
  float prumernyPocetPulsuZaMilisekundu = odPoslednihoRestartuPocitadlaUbehlo / lastWaterFlowSensorCount_global;

  //waterFlowSensorCount_ISR_global += timeDiff * prumernyPocetPulsuZaMilisekundu;
  attachInterrupt(digitalPinToInterrupt(WATER_FLOW_SENSOR_PIN), ISR_flowCount, RISING); // Setup Interrupt  // see http://arduino.cc/en/Reference/attachInterrupt  

}

*/

bool isWaterFlowingRightNow()
{//isWaterFlowingFlag_global;
  uint16_t actualISR = waterFlowSensorCount_ISR_global;
  delay(150);
  return actualISR != waterFlowSensorCount_ISR_global;
}

void resetflowCount()
{
  waterFlowDisplay_global += waterFlowSensorCount_ISR_global;
  waterFlowSensorCount_ISR_global = 0;
  lastWaterFlowSensorCount_global = 0;
}

float convertWaterFlowSensorImpulsesToLitres(float count)
{
  float floatFlow = count;
  floatFlow /= PULSES_PER_LITER_WATER_SENSOR;

  return floatFlow;
}

//Display function only
float getCurentWaterFlowInLitresPerMinute()
{
  uint16_t measuredImpulsesPerOneSecond = waterFlowSensorCount_ISR_global;
  delay(500);
  measuredImpulsesPerOneSecond = waterFlowSensorCount_ISR_global - measuredImpulsesPerOneSecond;

  float waterFlowInLitresPerMinute = convertWaterFlowSensorImpulsesToLitres(measuredImpulsesPerOneSecond) * 120;
  
  return waterFlowInLitresPerMinute;
}


int getSpareMinutesOfHotWater(float waterFlowInLitresPerMinute)
{
  float spareHotWaterInLitres = BOILER_SIZE_LITRES - convertWaterFlowSensorImpulsesToLitres(waterFlowDisplay_global);
  
  float spareTimeOfShoweringImMinutes = spareHotWaterInLitres / waterFlowInLitresPerMinute;

  int minutesToColdWater = trunc(spareTimeOfShoweringImMinutes);
  //int secondsToColdWater = (spareTimeOfShoweringImMinutes - minutesToColdWater ) * 60;

  return minutesToColdWater;
}



