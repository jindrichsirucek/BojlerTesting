void oneWireBus_setup()
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug(RESET_SPACE_COUNT_DEBUG) + E("F:oneWireBus_setup()"));
  pinMode(ONE_WIRE_BUS_PIN, INPUT_PULLUP);           // set pin to INPUT_PULLUP, to avoid using pullup resistor
  DS18B20.begin();
  yield();
}

void temperature_setup()
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug(RESET_SPACE_COUNT_DEBUG) + E("F:temperature_setup()"));

  displayServiceLine(cE("Init: Temp.."));
  if(DS18B20.isParasitePowerMode())
    DEBUG_OUTPUT.println(E("PARASITIC mode: Temperature sensors"));

  loadTempSensorAddressesFromEeprom();
  
  //Store Initial values
  DS18B20.requestTemperatures(); //initial read
  for(uint8_t sensorIndex = 0;sensorIndex < MAX_TEMP_SENSORS_COUNT;sensorIndex++)
  {
    if(GLOBAL.TEMP.sensors[sensorIndex].sensorConnected)
    {
      float currentTempSampleValue = DS18B20.getTempC(GLOBAL.TEMP.sensors[sensorIndex].address);
      if(isTemperatureCorrectMeasurment(currentTempSampleValue))
      {
        if(TEMPERATURE_DEBUG) DEBUG_OUTPUT.println(sE("OK: ")+GLOBAL.TEMP.sensors[sensorIndex].sensorName + E(": initial. temp: ") + currentTempSampleValue);
        GLOBAL.TEMP.sensors[sensorIndex].temp = currentTempSampleValue;
        GLOBAL.TEMP.sensors[sensorIndex].lastTemp = currentTempSampleValue;
      }
      else
      {  
        GLOBAL.TEMP.sensors[sensorIndex].temp = 0;
        GLOBAL.TEMP.sensors[sensorIndex].lastTemp = 0;
      }
    }
  }
  DS18B20.setWaitForConversion(false);  // makes it async // sync dealy: delay(750/ (1 << (12-resolution)));
  DS18B20.requestTemperatures(); //For next times
  yield();
}


bool readTemperaturesAsync()
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + E("F:readTemperaturesAsync(): ") + (MAX_TEMPERATURE_SAMPLES_COUNT-GLOBAL.TEMP.asyncMeasurmentCounter+1));
  DS18B20.requestTemperatures();

  GLOBAL.TEMP.asyncMeasurmentCounter--;
  uint8_t sensorMeasurmentDoneCount = 0; //how many sensors are ready (all samples measured)
  for(uint8_t sensorIndex = 0;sensorIndex < MAX_TEMP_SENSORS_COUNT;sensorIndex++)
  {
    //---  Checking if any or all sensors have saved enough temp samples  ---
    if(GLOBAL.TEMP.sensors[sensorIndex].correctTempSampleIndex == CORRECT_TEMP_SAMPLES_COUNT || GLOBAL.TEMP.sensors[sensorIndex].sensorConnected == false)
    {
      sensorMeasurmentDoneCount++;
      if(sensorMeasurmentDoneCount == MAX_TEMP_SENSORS_COUNT)
      {
          GLOBAL.TEMP.asyncMeasurmentCounter = 0;//enough good temp samples for ALL sensors
          break;
        }
        else
          continue;//enough good temp samples for this sensor
    }

    
    //---  Measuring and saving temp samples  ---
    if(GLOBAL.TEMP.sensors[sensorIndex].sensorConnected)
    {
      float currentTempSampleValue = DS18B20.getTempC(GLOBAL.TEMP.sensors[sensorIndex].address);
      if(TEMPERATURE_DEBUG) DEBUG_OUTPUT.print((String)(MAX_TEMPERATURE_SAMPLES_COUNT - GLOBAL.TEMP.asyncMeasurmentCounter) + E(". ")+getSensorAddressHexString(GLOBAL.TEMP.sensors[sensorIndex].address)+E(" ")+GLOBAL.TEMP.sensors[sensorIndex].sensorName+E(" temp trial: ") + currentTempSampleValue);

      if(isTemperatureCorrectMeasurment(currentTempSampleValue))
      {
        if(GLOBAL.TEMP.sensors[sensorIndex].correctTempSampleIndex == 0) //Get initial temp Sample
        {
          if(TEMPERATURE_DEBUG) DEBUG_OUTPUT.println();

          GLOBAL.TEMP.sensors[sensorIndex].newTempSamples[GLOBAL.TEMP.sensors[sensorIndex].correctTempSampleIndex] = currentTempSampleValue;
          GLOBAL.TEMP.sensors[sensorIndex].correctTempSampleIndex++;
        }
        else if(abs(currentTempSampleValue - GLOBAL.TEMP.sensors[sensorIndex].newTempSamples[GLOBAL.TEMP.sensors[sensorIndex].correctTempSampleIndex - 1]) < MAX_ACCEPTED_TEMP_SAMPLES_DIFFERENCE)
        {
          if(TEMPERATURE_DEBUG) DEBUG_OUTPUT.print(E(" Temp difference: "));
          if(TEMPERATURE_DEBUG) DEBUG_OUTPUT.println(abs(currentTempSampleValue - GLOBAL.TEMP.sensors[sensorIndex].newTempSamples[GLOBAL.TEMP.sensors[sensorIndex].correctTempSampleIndex - 1]));

          GLOBAL.TEMP.sensors[sensorIndex].newTempSamples[GLOBAL.TEMP.sensors[sensorIndex].correctTempSampleIndex] = currentTempSampleValue;
          GLOBAL.TEMP.sensors[sensorIndex].correctTempSampleIndex++;
        }
        else
          if(TEMPERATURE_DEBUG) DEBUG_OUTPUT.printf(cE("Bad TEMP sample: current:(%d) last good:(%d)\n"), (int)(currentTempSampleValue * 1000) / 1, (int)(GLOBAL.TEMP.sensors[sensorIndex].newTempSamples[GLOBAL.TEMP.sensors[sensorIndex].correctTempSampleIndex - 1] * 1000) / 1);
      }
      else
        if(TEMPERATURE_DEBUG) DEBUG_OUTPUT.println();
        // if(SHOW_WARNING_DEBUG_MESSAGES) DEBUG_OUTPUT.printf(String(E("!!Warning: %d measurment unsuccessful!!\n")).c_str(), (MAX_TEMPERATURE_SAMPLES_COUNT - GLOBAL.TEMP.asyncMeasurmentCounter - CORRECT_TEMP_SAMPLES_COUNT));
    }
  }

  //Is all measurment ready to count average?
  if(GLOBAL.TEMP.asyncMeasurmentCounter != 0)
    return false;


  //---  All measurment done, avg value counting  ---
  for(uint8_t sensorIndex = 0;sensorIndex < MAX_TEMP_SENSORS_COUNT;sensorIndex++)
  {
    //Sensor connected
    if(GLOBAL.TEMP.sensors[sensorIndex].sensorConnected)
    {
      //enough correct samples
      if(GLOBAL.TEMP.sensors[sensorIndex].correctTempSampleIndex == CORRECT_TEMP_SAMPLES_COUNT)
      {
        float sensorTempAverage = 0;
        for(uint8_t i = 0; i < CORRECT_TEMP_SAMPLES_COUNT; i++)
          sensorTempAverage += GLOBAL.TEMP.sensors[sensorIndex].newTempSamples[i];

        sensorTempAverage = sensorTempAverage / CORRECT_TEMP_SAMPLES_COUNT;
        // if(sensorTempAverage < 1)                //   if(SHOW_ERROR_DEBUG_MESSAGES) DEBUG_OUTPUT.println(E("!!!Error: Temp measurment unsuccessful!! Temp avarege is smaller than zero"));
        if(TEMPERATURE_DEBUG) DEBUG_OUTPUT.println(sE("OK: ")+GLOBAL.TEMP.sensors[sensorIndex].sensorName + E(": avg.temp: ") + sensorTempAverage);
          GLOBAL.TEMP.sensors[sensorIndex].temp = sensorTempAverage;
      }
      else
      {
        if(SHOW_ERROR_DEBUG_MESSAGES) DEBUG_OUTPUT.println((String)GLOBAL.TEMP.sensors[sensorIndex].sensorName + E(": !!!Error: Temp measurment unsuccessful!!"));
        GLOBAL.TEMP.sensors[sensorIndex].temp = ERROR_TEMP_VALUE_MINUS_127;
      }
    }
    //resets sensor good sample counter
    GLOBAL.TEMP.sensors[sensorIndex].correctTempSampleIndex = 0;
  }
  //resets counter of samples for next measurment
  GLOBAL.TEMP.asyncMeasurmentCounter = MAX_TEMPERATURE_SAMPLES_COUNT;

  return true;
}


bool isTemperatureCorrectMeasurment(float tempToCheck)
{
  return (tempToCheck >= 1 && tempToCheck != ERROR_TEMP_VALUE_PLUS_85);
}


