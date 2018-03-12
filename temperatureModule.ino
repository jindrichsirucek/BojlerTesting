//Setings definitions
#define CORRECT_TEMP_SAMPLES_COUNT 3 // number of samples from which count average temperature
#define MAX_TEMPERATURE_SAMPLES_COUNT 7 // number of trials to get correct temperature samples
#define MAX_ACCEPTED_TEMP_SAMPLES_DIFFERENCE 1 //Maximal accepted temp difference between two measurments, in degrees of celsius // eliminates wrong measurments

void oneWireBus_setup()
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:oneWireBus_setup()"));
  pinMode(ONE_WIRE_BUS_PIN, INPUT_PULLUP);           // set pin to INPUT_PULLUP, to avoid using pullup resistor
  DS18B20.begin();
}

void temperature_setup()
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:temperature_setup()"));
   if(DS18B20.isParasitePowerMode())
     DEBUG_OUTPUT.println(E("PARASITIC mode: Temperature senors"));
  loadTempSensorAddressesFromEeprom();
  temperature_loop(0);
}


bool readTemperatures()
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:readTemperatures(): "));

  // if(TEMPERATURE_DEBUG) DEBUG_OUTPUT.print((String)"Pipe temp: " + (String)pipeTemp + (String)" sensor address: ");
  // if(TEMPERATURE_DEBUG) printAddressHex(tempSensorsAddresses.pipe);
  // if(TEMPERATURE_DEBUG) DEBUG_OUTPUT.print(E("\nBojler temp sensor address: "));
  // if(TEMPERATURE_DEBUG) printAddressHex(tempSensorsAddresses.bojler);
  // if(TEMPERATURE_DEBUG) DEBUG_OUTPUT.println();

  float tempSamples[MAX_TEMP_SENSORS_COUNT][CORRECT_TEMP_SAMPLES_COUNT];
  uint8_t goodTempSampleCount[MAX_TEMP_SENSORS_COUNT] = { 0 };

  DS18B20.requestTemperatures();
  uint8_t cnt = MAX_TEMPERATURE_SAMPLES_COUNT;
  while(cnt--)
  {
    //Display animations during measuring temp
    animateActiveSymbol_taskerLoop(0);
    uint8_t sensorMeasurmentDoneCount = 0;
    for(uint8_t sensorIndex = 0;sensorIndex < MAX_TEMP_SENSORS_COUNT;sensorIndex++)
    {
      if(goodTempSampleCount[sensorIndex] == CORRECT_TEMP_SAMPLES_COUNT || tempSensors[sensorIndex].sensorConnected == false)
      {
        sensorMeasurmentDoneCount++;
        if(sensorMeasurmentDoneCount == MAX_TEMP_SENSORS_COUNT)
        {
          cnt = 0;//enough good temp samples for ALL sensors
          break;
        }
        else
          continue;//enough good temp samples for this sensor
      }

      if(tempSensors[sensorIndex].sensorConnected)
      {
        float currentTempSampleValue = DS18B20.getTempC(tempSensors[sensorIndex].address);

        if(TEMPERATURE_DEBUG) DEBUG_OUTPUT.print((String)(MAX_TEMPERATURE_SAMPLES_COUNT - cnt) + E(". ")+getSensorAddressHexString(tempSensors[sensorIndex].address)+E(" ")+tempSensors[sensorIndex].sensorName+E(" temp trial: ") + currentTempSampleValue);

        if(isTemperatureCorrectMeasurment(currentTempSampleValue))
        {
          if(goodTempSampleCount[sensorIndex] == 0) //Get initial temp Sample
          {
            tempSamples[sensorIndex][goodTempSampleCount[sensorIndex]] = currentTempSampleValue;
            goodTempSampleCount[sensorIndex]++;
            if(TEMPERATURE_DEBUG) DEBUG_OUTPUT.println();
          }
          else if(abs(currentTempSampleValue - tempSamples[sensorIndex][goodTempSampleCount[sensorIndex] - 1]) < MAX_ACCEPTED_TEMP_SAMPLES_DIFFERENCE)
          {
            if(TEMPERATURE_DEBUG) DEBUG_OUTPUT.print(E(" Temp difference: "));
            if(TEMPERATURE_DEBUG) DEBUG_OUTPUT.println(abs(currentTempSampleValue - tempSamples[sensorIndex][goodTempSampleCount[sensorIndex] - 1]));

            tempSamples[sensorIndex][goodTempSampleCount[sensorIndex]] = currentTempSampleValue;
            goodTempSampleCount[sensorIndex]++;
          }
          else
            if(TEMPERATURE_DEBUG) DEBUG_OUTPUT.printf(cE("Bad TEMP sample: current:(%d) last good:(%d)\n"), (int)(currentTempSampleValue * 1000) / 1, (int)(tempSamples[sensorIndex][goodTempSampleCount[sensorIndex] - 1] * 1000) / 1);
        }
        else
          if(TEMPERATURE_DEBUG) DEBUG_OUTPUT.println();
        // if(SHOW_WARNING_DEBUG_MESSAGES) DEBUG_OUTPUT.printf(String(E("!!Warning: %d measurment unsuccessful!!\n")).c_str(), (MAX_TEMPERATURE_SAMPLES_COUNT - cnt - CORRECT_TEMP_SAMPLES_COUNT));
      }
    }
    DS18B20.requestTemperatures();
    yield_debug();
  }
  
  bool allMeasurmentSuccessful = true;
  for(uint8_t sensorIndex = 0;sensorIndex < MAX_TEMP_SENSORS_COUNT;sensorIndex++)
  {
    //Sensor connected && enough correct samples
    if(tempSensors[sensorIndex].sensorConnected)
    {
      if(goodTempSampleCount[sensorIndex] == CORRECT_TEMP_SAMPLES_COUNT)
      {
        float sensorTempAverage = 0;
        for(uint8_t i = 0; i < CORRECT_TEMP_SAMPLES_COUNT; i++)
        sensorTempAverage += tempSamples[sensorIndex][i];

        sensorTempAverage = sensorTempAverage / CORRECT_TEMP_SAMPLES_COUNT;
        // if(sensorTempAverage < 1)                //   if(SHOW_ERROR_DEBUG_MESSAGES) DEBUG_OUTPUT.println(E("!!!Error: Temp measurment unsuccessful!! Temp avarege is smaller than zero"));
        if(TEMPERATURE_DEBUG) DEBUG_OUTPUT.println(sE("OK: ")+tempSensors[sensorIndex].sensorName + E(": avg.temp: ") + sensorTempAverage);
        tempSensors[sensorIndex].temp = sensorTempAverage;
      }
      else
      {
        if(SHOW_ERROR_DEBUG_MESSAGES) DEBUG_OUTPUT.println((String)tempSensors[sensorIndex].sensorName + E(": !!!Error: Temp measurment unsuccessful!!"));
        allMeasurmentSuccessful = false;
        tempSensors[sensorIndex].temp = ERROR_TEMP_VALUE_MINUS_127;
      }
    }
  }

  return allMeasurmentSuccessful;
}


bool isTemperatureSensorWorking()
{
  return isTemperatureCorrectMeasurment(lastTemp_global);
}


bool isTemperatureCorrectMeasurment(float tempToCheck)
{
  return (tempToCheck >= 1 && tempToCheck != ERROR_TEMP_VALUE_PLUS_85);
}
