//symbolsDefine.h

#ifndef symbolsDefine_h
#define symbolsDefine_h


byte newCharC[8] = {
  B00110,
  B01001,
  B01001,
  B00110,
  B00000,
  B00000,
  B00000,
  B00000
};

byte newCharTemp[8] = {
  B00100,
  B01010,
  B01010,
  B01110,
  B01110,
  B11111,
  B11111,
  B01110
};

byte newCharWaterDrop[8] = {
  B00100,
  B00100,
  B01010,
  B01010,
  B10001,
  B10001,
  B10001,
  B01110
};

byte newCharTime[8] = {
  B00000,
  B01110,
  B10101,
  B10111,
  B10001,
  B01110,
  B00000,
  B00000
};

byte newCharShower[][8] = {
 {
  B00100,
  B00100,
  B01110,
  B11111,
  B01010,
  B10101,
  B00000,
  B01010
},
{
  B00100,
  B00100,
  B01110,
  B11111,
  B10101,
  B01010,
  B00000,
  B10101,
  
}};

byte newCharWaterFlow[][8] = {
{
  B01110,
  B00100,
  B11111,
  B11111,
  B00011,
  B00001,
  B00010,
  B00001
},
{
  B01110,
  B00100,
  B11111,
  B11111,
  B00011,
  B00010,
  B00001,
  B00010
}
};



byte newCharWifiNOTConnected[8] = {
  B00000,
  B10001,
  B01010,
  B00100,
  B01010,
  B10001,
  B00100,
  B00000
};

byte newCharWifiConnected[8] = 
{
  B01110,
  B11111,
  B10001,
  B00100,
  B01010,
  B00000,
  B00100,
  B00000
};


byte newCharWifiSending[][8] = 
{
  {
    B00000,
    B00000,
    B00000,
    B00000,
    B00000,
    B00000,
    B00100,
    B00000
  },
  {  
    B00000,
    B00000,
    B00000,
    B00100,
    B01010,
    B00000,
    B00100,
    B00000
    
  },
  {
    B00000,
    B01110,
    B10001,
    B00100,
    B01010,
    B00000,
    B00100,
    B00000
  },
  {
    B01110,
    B11111,
    B10001,
    B00100,
    B01010,
    B00000,
    B00100,
    B00000
  }
};

#endif
