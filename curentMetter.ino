


void current_setup()
{
  pinMode(CURRENT_SENSOR_PIN, INPUT);
  lastElectricCurrentState_global = isThereElectricCurrent();
}


bool isThereElectricCurrent()
{
  if(MAIN_DEBUG) DEBUG_OUTPUT.print(getUpTimeDebug() + F("F:isThereElectricCurrent(): "));
  bool isThereCurrent = false;
  float actualCurrent = 0;
  
  actualCurrent = getActualCurrentValue();
  char bufferCharConversion[10];               //temporarily holds data from vals
  dtostrf(actualCurrent, 1, 3, bufferCharConversion);  //first num is mininum width, second is precision
  lastCurrentMeasurmentText_global = bufferCharConversion;

  if(CURRENT_DEBUG) Serial.print("Current saved in lastCurrentMeasurmentText_global: ");
  if(CURRENT_DEBUG) Serial.println(lastCurrentMeasurmentText_global);

  isThereCurrent = actualCurrent > 6; //1A treshold?

  if(MAIN_DEBUG) DEBUG_OUTPUT.print((isThereCurrent? sE("YES") : sE("NO")));
  if(MAIN_DEBUG) DEBUG_OUTPUT.println(sE(" (") + lastCurrentMeasurmentText_global + cE("A)"));

  return isThereCurrent;
}


float getActualCurrentValue()
 {
   float resistorValue = 77.5;
   float vRef = 1;
   float linearizationCoef = 1.0986;//1.122;
   float nVPP;   // Voltage measured across resistor
   float nCurrThruResistorPP; // Peak Current Measured Through Resistor
   float nCurrThruResistorRMS; // RMS current through Resistor
   float nCurrentThruWire;     // Actual RMS current in Wire

   int maxAnalogValue = getMaxAnalogValue();
   if(CURRENT_DEBUG) Serial.print("\nMeasured analog input: ");
   if(CURRENT_DEBUG) Serial.print(maxAnalogValue);
   if(CURRENT_DEBUG) Serial.println("/1024");
   

  // Convert the digital data to a voltage
   nVPP = (maxAnalogValue * vRef)/1024.0;
   // Use Ohms law to calculate current across resistor  and express in mA
   nCurrThruResistorPP = (nVPP/resistorValue); //* 1000.0; //*1000=mA
   // Use Formula for SINE wave to convert to RMS 
   nCurrThruResistorRMS = nCurrThruResistorPP * 0.707;
   
   // Current Transformer Ratio is 1000:1...   
   // Therefore current through 200 ohm resistor
   // is multiplied by 1000 to get input current
   nCurrentThruWire = nCurrThruResistorRMS * 1000;

   
   if(CURRENT_DEBUG) Serial.print("Volts Peak : ");
   if(CURRENT_DEBUG) Serial.print(nVPP,3);
   if(CURRENT_DEBUG) Serial.println("V");
   
   if(CURRENT_DEBUG) Serial.print("Current Through Resistor (Peak) : ");
   if(CURRENT_DEBUG) Serial.print(nCurrThruResistorPP,3);
   if(CURRENT_DEBUG) Serial.println(" A Peak to Peak");
   
   if(CURRENT_DEBUG) Serial.print("Current Through Resistor (RMS) : ");
   if(CURRENT_DEBUG) Serial.print(nCurrThruResistorRMS,3);
   if(CURRENT_DEBUG) Serial.println(" A RMS");
   
   if(CURRENT_DEBUG) Serial.print("Current Through Wire : ");
   if(CURRENT_DEBUG) Serial.print(nCurrentThruWire,3);
   if(CURRENT_DEBUG) Serial.println(" A RMS");
   
   float linearizedCurrent = nCurrentThruWire*linearizationCoef;
   if(CURRENT_DEBUG) Serial.print("LINEARIZED : ");
   if(CURRENT_DEBUG) Serial.print(linearizedCurrent,3);
   if(CURRENT_DEBUG) Serial.println(" A RMS");

   if(CURRENT_DEBUG) Serial.println();

   return linearizedCurrent;
 }


/************************************
In order to calculate RMS current, we need to know
the peak to peak voltage measured at the output across a Resistor

The following function takes one second worth of samples
and returns the peak value that is measured
*************************************/
float getMaxAnalogValue()
{
  int readValue;             //value read from the sensor
  int maxValue = 0;          // store max value here
   uint32_t start_time = millis();
   while((millis()-start_time) < 1000) //sample for 1 Sec
   {
       readValue = analogRead(CURRENT_SENSOR_PIN);
       // see if you have a new maxValue
       if (readValue > maxValue) 
       {
           /*record the maximum sensor value*/
           maxValue = readValue;
       }
   }
   
   return maxValue;
 }





///// OLD CODE //////

// DIGITAL READ
// bool isThereElectricCurrent()
// {
//   if(MAIN_DEBUG) DEBUG_OUTPUT.print(getUpTimeDebug() + F("F:isThereElectricCurrent(): "));
//   bool pinState = 1; //spíná v nule, je to input_pullup
//   uint8_t attempt = 10;
//   while(attempt--)
//   {
//     pinState = digitalRead(CURRENT_SENSOR_PIN);
//     // if(MAIN_DEBUG) DEBUG_OUTPUT.printf(("\nattempt %d: %d "),attempt,pinState);
//     if(pinState == 0)
//       break;
//     delay(1);
//   }
//   // if(MAIN_DEBUG) DEBUG_OUTPUT.print(("attempt: ") + attempt);
   
//   if(MAIN_DEBUG) DEBUG_OUTPUT.println((pinState)?F("NO") : F("YES"));
//   return !pinState;
// }


// float analogReadCurrent()
// {
//   int sensor_max;
//   sensor_max = getMaxCurrentValue();
//   DEBUG_OUTPUT.print("sensor_max = ");
//   DEBUG_OUTPUT.println(sensor_max);
//   //the VCC on the Grove interface of the sensor is 5v
//   float amplitude_current = (float)sensor_max / 124 * 5 / 800 * 2000000;
//   float effective_value = amplitude_current / 1.414; //minimum_current=1/124*5/800*2000000/1.414=8.6(mA)
//   //Only for sinusoidal alternating current
//   DEBUG_OUTPUT.println(F(""));
//   DEBUG_OUTPUT.println(amplitude_current, 1); //Only one number after the decimal point
//   DEBUG_OUTPUT.println(F(""));
//   DEBUG_OUTPUT.println(effective_value, 1);
//   return effective_value;
// }


// /*Function: Sample for 1000ms and get the maximum value from the SIG pin*/
// int getAnalogMaxCurrentValue()
// {
//   if(MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug() + F("F:getMaxCurrentValue()"));
//   int sensorValue;             //value read from the sensor
//   int sensorMax = 0;
//   int pocetMereni = 0;
//   while(pocetMereni++ < 100) //sample for 1000ms
//   {
//     delay(2);
//     sensorValue = analogRead(CURRENT_SENSOR_PIN);
//     if(sensorValue > sensorMax)
//     {
//       /*record the maximum sensor value*/
//       sensorMax = sensorValue;
//       if(CURRENT_DEBUG) DEBUG_OUTPUT.println("sensorMax value: " + (String)sensorMax);
//     }
//   }
//   return sensorMax;
// }




// void current_loop(int)
// {
//   if (MAIN_DEBUG) DEBUG_OUTPUT.println(getUpTimeDebug(RESET_SPACE_COUNT_DEBUG) + F("F:current_loop()"));
    


    
//   int currentNow = getMaxCurrentValue();
//   if (abs(lastCurrentMeasurment_global - currentNow) > 1000)
//   {
//     if (lastCurrentMeasurment_global > 1000 && currentNow < 200) //Pokud se právě vypnul ohřev vody v bojleru
//     {
//       waterFlowDisplay_global = 0; //Vynuluj měření spotřeby teplé vody - bojler je po vypnutí ohřevu celý nahřátý
//       //TODO : převést na funkci a funkci dát do display souboru kam patří
//       logNewNodeState(F("WaterFlowDisplay_reset"));
//     }
    
//     int lastCurrent = lastCurrentMeasurment_global;
    
//     lastCurrentMeasurment_global = currentNow;
//     logNewNodeState((lastCurrent > currentNow) ? "Current_dropped" : "Current_rised");
//   }
//   else
//     lastCurrentMeasurment_global = currentNow;
// }

