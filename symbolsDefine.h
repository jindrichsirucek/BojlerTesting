//symbolsDefine.h

#ifndef symbolsDefine_h
#define symbolsDefine_h

#define PROGMEM_TT __attribute__((section(".irom.text.template2")))


PROGMEM_TT uint8_t degreeSymbol[8] = 
{
  B00110,
  B01001,
  B01001,
  B00110,
  B00000,
  B00000,
  B00000,
  B00000
};

 PROGMEM_TT uint8_t tempSymbol[8] = {
  B00100,
  B01010,
  B01010,
  B01110,
  B01110,
  B11111,
  B11111,
  B01110
};

 PROGMEM_TT uint8_t waterDropSymbol[8] = {
  B00100,
  B00100,
  B01010,
  B01010,
  B10001,
  B10001,
  B10001,
  B01110
};

 PROGMEM_TT uint8_t timeSymbol[8] = {
  B00000,
  B01110,
  B10101,
  B10111,
  B10001,
  B01110,
  B00000,
  B00000
};

 PROGMEM_TT uint8_t emptySymbol[8] = 
{
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000
};

 PROGMEM_TT uint8_t degreeCelsiusSymbol[8] =
{
  B11000,
  B11000,
  B00011,
  B00100,
  B00100,
  B00100,
  B00011,
  B00000,
};

 PROGMEM_TT uint8_t upTempSymbol[8] = 
{
  B11111,
  B00000,
  B00100,
  B01110,
  B11111,
  B00100,
  B00100,
  B00100,
};

 PROGMEM_TT uint8_t boilerSymbol[8] = 
{
  B01110,
  B10001,
  B11111,
  B10001,
  B10001,
  B10001,
  B10001,
  B01110
};

 PROGMEM_TT uint8_t wifiNOTConnectedSymbol[8] = {
  B00000,
  B10001,
  B01010,
  B00100,
  B01010,
  B10001,
  B00100,
  B00000
};

 PROGMEM_TT uint8_t wifiConnectedSymbol[8] =
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



//////////              //////////
//////////   ANIMATIONS //////////
//////////              //////////

PROGMEM_TT uint8_t showerSymbolAnimation[][8] = {
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


PROGMEM_TT uint8_t waterFlowSymbolAnimation[][8] = {
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

 PROGMEM_TT uint8_t wifiSendingSymbolAnimation[][8] = 
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

PROGMEM_TT uint8_t signalStrengthSymbolAnimation[][8] = {
{
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00011,
  B11111,
},
{
  B00000,
  B00000,
  B00000,
  B00000,
  B00001,
  B01111,
  B11111,
  B11111,
},
{
  B00000,
  B00000,
  B00000,
  B00111,
  B11111,
  B11111,
  B11111,
  B11111,
},
{
  B00000,
  B00011,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
},
{
  B01111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
},
{
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
}
};


//  uint8_t upTempSymbolAnimation[][8] = {
// {
//   B00000,
//   B00000,
//   B00000,
//   B00000,
//   B00100,
//   B01010,
//   B10001
// },
// {
//   B00000,
//   B00000,
//   B00000,
//   B00100,
//   B01010,
//   B10001,
//   B00000
// },
// {
//   B00000,
//   B00000,
//   B00100,
//   B01010,
//   B10001,
//   B00000,
//   B00000,
//   B00000
// },
// {
//   B00000,
//   B00100,
//   B01010,
//   B10001,
//   B00000,
//   B00000,
//   B00000,
//   B00000
// },
// {
//   B00100,
//   B01010,
//   B10001,
//   B00000,
//   B00000,
//   B00000,
//   B00000,
//   B00000
// },
// };

#endif
