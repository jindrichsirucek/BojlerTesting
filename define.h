  #pragma once
  // definitions limits
  // uint8_t   byte, unsigned char, unsigned byte max 255
  // uint16_t  unsigned int, unsigned short, max 65 535
  // uint32_t  size_t, unsigned long/word, max 4 294 967 295


// int a = 1;
// int *b = &a; //b i pointer to int || &a is pointer address of variable a
// *b += 1;
//now a == 2 (and b's value is a pointer to the location of a in memory)



// int i;          // integer variable 'i'
// int *p;         // pointer 'p' to an integer
// int a[];        // array 'a' of integers
// int f();        // function 'f' with return value of type integer
// int **pp;       // pointer 'pp' to a pointer to an integer
// int (*pa)[];    // pointer 'pa' to an array of integer
// int (*pf)();    // pointer 'pf' to a function with return value integer
// int *ap[];      // array 'ap' of pointers to an integer
// int *fp();      // function 'fp' which returns a pointer to an integer
// int ***ppp;     // pointer 'ppp' to a pointer to a pointer to an integer
// int (**ppa)[];  // pointer 'ppa' to a pointer to an array of integers
// int (**ppf)();  // pointer 'ppf' to a pointer to a function with return value of type integer
// int *(*pap)[];  // pointer 'pap' to an array of pointers to an integer
// int *(*pfp)();  // pointer 'pfp' to function with return value of type pointer to an integer
// int **app[];    // array of pointers 'app' that point to pointers to integer values
// int (*apa[])[]; // array of pointers 'apa' to arrays of integers
// int (*apf[])(); // array of pointers 'apf' to functions with return values of type integer
// int ***fpp();   // function 'fpp' which returns a pointer to a pointer to a pointer to an int
// int (*fpa())[]; // function 'fpa' with return value of a pointer to array of integers
// int (*fpf())(); // function 'fpf' with return value of a pointer to function which returns an integer


  //!!!Source: http://www.esp8266.com/viewtopic.php?f=9&t=6376#
  //GITHUB: https://github.com/wa0uwh/ERB-EspWebServer/tree/master/Main
  // Macros to force format stings and string constants into FLASH Memory

  char globalCharBuf[256+1] = {0}; // Buffer, Used with cF() to store constants in program space (FLASH)
  
  //F Used insteda of F macro, Macro can be adjusted to debugging
  #define E(x) F(x) 
  //Usage:   // Serial.println(  E("<!-- Verbose -->") );

  // Used as an F() when being used as the first Element of a Multi-Element Expression
  #define sE(x) String( E(x) )
  //Usage: // Serial.println( sE("Sketch Rev: <b>") + 13.0 + E("</b>"));

  // Used with printf() and other fucntions where c_str parameter format is needed
  #define cE(x) strncpy_P(globalCharBuf, (PGM_P)PSTR(x), sizeof(globalCharBuf))
  //Usage: Serial.printf(cE("Parameter value: %d"), 23);

  #define ALIGN __attribute__ (( aligned ( sizeof(char*) ) ))
  //#define ALIGN     __attribute__ (( aligned (__BIGGEST_ALIGNMENT__) ))
  #define INFLASH PROGMEM ALIGN

  //Used to snprintf to char variable[], result is in buf variable (globalCharBuf) ready to use until buf is overwritten
  #define snprintfTo_globalCharBuf(f_, ...) snprintf(globalCharBuf,sizeof(globalCharBuf),strncpy_P(globalCharBuf,(PGM_P)(f_),sizeof(globalCharBuf)), __VA_ARGS__); //My own macro :-) yeah, Im awsome :-)
  //Usage:char globalCharBuf[32+1] = {0}; 
  //uint8_t stringLength = snprintfTo_globalCharBuf(E("Score: %d:%d"), 3,2);
  //Serial.println(globalCharBuf);
  


  #define xE(x) (x)


  #define SHORT_BLINK 20
  #define NORMAL_BLINK 50
  #define LONG_BLINK 200

  #define SET_HEATING_RELAY_CONNECTED false
  #define SET_HEATING_RELAY_DISCONECTED true


  // Define Multipliers for Ms Counters
  #define MSEC (1)
  #define SEC  (MSEC * 1000)
  #define MIN  (SEC * 60)
  #define HOUR (MIN * 60)
  #define DAY  (HOUR * 24)
  #define WEEK (DAY * 7)

  enum {
    WITHOUT_SECONDS = 0,
    WITH_SECONDS = 1,
  };

  #define SIZE_OF_LOCAL_ARRAY(variable) ((uint8_t)(sizeof variable / sizeof *variable))




  #define Q(x) #x
  #define QUOTE(x) E(Q(x))
  #define _GET_MACRO(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,NAME,...) NAME

  #define  _PRINT_STRUCT_1_MEMBER(struct, member) DEBUG_OUTPUT.print(sE("  - ") + QUOTE(member) + E(" -> ") + struct.member + E("\n"))
  #define _PRINT_STRUCT_2_MEMBERS(struct, member, ...) _PRINT_STRUCT_1_MEMBER(struct, member); _PRINT_STRUCT_1_MEMBER(struct, __VA_ARGS__);
  #define _PRINT_STRUCT_3_MEMBERS(struct, member, ...) _PRINT_STRUCT_1_MEMBER(struct,member); _PRINT_STRUCT_2_MEMBERS(struct, __VA_ARGS__);
  #define _PRINT_STRUCT_4_MEMBERS(struct, member, ...) _PRINT_STRUCT_1_MEMBER(struct,member); _PRINT_STRUCT_3_MEMBERS(struct, __VA_ARGS__);
  #define _PRINT_STRUCT_5_MEMBERS(struct, member, ...) _PRINT_STRUCT_1_MEMBER(struct,member); _PRINT_STRUCT_4_MEMBERS(struct, __VA_ARGS__);
  #define _PRINT_STRUCT_6_MEMBERS(struct, member, ...) _PRINT_STRUCT_1_MEMBER(struct,member); _PRINT_STRUCT_5_MEMBERS(struct, __VA_ARGS__);
  #define _PRINT_STRUCT_7_MEMBERS(struct, member, ...) _PRINT_STRUCT_1_MEMBER(struct,member); _PRINT_STRUCT_6_MEMBERS(struct, __VA_ARGS__);
  #define _PRINT_STRUCT_8_MEMBERS(struct, member, ...) _PRINT_STRUCT_1_MEMBER(struct,member); _PRINT_STRUCT_7_MEMBERS(struct, __VA_ARGS__);
  #define _PRINT_STRUCT_9_MEMBERS(struct, member, ...) _PRINT_STRUCT_1_MEMBER(struct,member); _PRINT_STRUCT_8_MEMBERS(struct, __VA_ARGS__);
  #define _PRINT_STRUCT_10_MEMBERS(struct, member, ...) _PRINT_STRUCT_1_MEMBER(struct,member); _PRINT_STRUCT_9_MEMBERS(struct, __VA_ARGS__);


  #define PRINT_STRUCT_MEMBERS(struct,...) DEBUG_OUTPUT.print(sE("- ") + QUOTE(struct) + E("\n")) ; _GET_MACRO(__VA_ARGS__, _PRINT_STRUCT_10_MEMBERS,_PRINT_STRUCT_9_MEMBERS,_PRINT_STRUCT_8_MEMBERS,_PRINT_STRUCT_7_MEMBERS,_PRINT_STRUCT_6_MEMBERS,_PRINT_STRUCT_5_MEMBERS,_PRINT_STRUCT_4_MEMBERS, _PRINT_STRUCT_3_MEMBERS, _PRINT_STRUCT_2_MEMBERS, _PRINT_STRUCT_1_MEMBER, VOID_MACRO)(struct,__VA_ARGS__)
