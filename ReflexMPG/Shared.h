#ifndef RZORD_SHARED_H_
#define RZORD_SHARED_H_

//#define RZORD_ENABLE_AUTORESET

//Send debug messages to serial port
//#define ENABLE_SERIAL_DEBUG

//maximum 6 controllers per arduino
#define MAX_USB_STICKS 2

//uint16_t sleepTime = 5000;//In micro seconds.

//Joystick_* usbStick[MAX_USB_STICKS];

//uint8_t totalUsb = 1;//how many controller outputs via usb.

const uint16_t version = 0x0100;
const uint16_t MODE_ID_SNES     = version |  1; // NES/SNES
const uint16_t MODE_ID_VB       = version |  2; // VirtualBoy
const uint16_t MODE_ID_PCE      = version |  3; // PCEngine
const uint16_t MODE_ID_NEOGEO   = version |  4; // NeoGeo
const uint16_t MODE_ID_3DO      = version |  5; // 3DO
const uint16_t MODE_ID_JAGUAR   = version |  6; // Jaguar
const uint16_t MODE_ID_N64      = version |  7; // N64
const uint16_t MODE_ID_GC       = version |  8; // GameCube
const uint16_t MODE_ID_WII      = version |  9; // Wii
const uint16_t MODE_ID_SMS      = version | 10; // MasterSystem
const uint16_t MODE_ID_JPC      = version | 11; // JapanesePC

const uint16_t MODE_ID_SATURN   = version | 12;  // MegaDrive/Saturn  todo: add multiple modes

const uint16_t MODE_ID_PSX      = version | 13;  // RZMPsDS1

const uint16_t MODE_ID_PSX2     = version | 14;  // ReflexPSGun       Guncon
const uint16_t MODE_ID_PSX3     = version | 15;  // RZMPsNeGcon       NeGCon HID
const uint16_t MODE_ID_PSX4     = version | 16;  // ReflexPSWheel     NeGCon MiSTer (wheel+paddle)
const uint16_t MODE_ID_PSX5     = version | 17;  // MiSTer-A1 JogCon  JogCon MiSTer (spinner+paddle)
const uint16_t MODE_ID_PSX6     = version | 18;  // RZMPSJogCon


//strings in progmem
#define PSTR_TO_F(s) reinterpret_cast<const __FlashStringHelper *> (s)
const char PSTR_NA[] PROGMEM = "N/A";
const char PSTR_NONE[] PROGMEM = "NONE";
const char PSTR_DIGITAL[] PROGMEM = "DIGITAL";
const char PSTR_PAD[] PROGMEM = "PAD";
const char PSTR_MULTITAP[] PROGMEM = "MULTI-TAP";


enum DeviceEnum : uint8_t {
  RZORD_NONE = 0,
#ifdef ENABLE_REFLEX_SATURN
  RZORD_SATURN, //1
#endif
#ifdef ENABLE_REFLEX_SNES
  RZORD_SNES,   //2
#endif
#ifdef ENABLE_REFLEX_PSX
  RZORD_PSX,    //3
#endif
#ifdef ENABLE_REFLEX_PSX_JOG
  RZORD_PSX_JOG,//4
#endif
#ifdef ENABLE_REFLEX_PCE
  RZORD_PCE,    //5
#endif
#ifdef ENABLE_REFLEX_NEOGEO
  RZORD_NEOGEO,  //6
#endif
#ifdef ENABLE_REFLEX_3DO
  RZORD_3DO,  //7
#endif
#ifdef ENABLE_REFLEX_JAGUAR
  RZORD_JAGUAR,  //8
#endif
#ifdef ENABLE_REFLEX_N64
  RZORD_N64,  //9
#endif
#ifdef ENABLE_REFLEX_GAMECUBE
  RZORD_GAMECUBE,  //10
#endif
#ifdef ENABLE_REFLEX_WII
  RZORD_WII,  //11
#endif
#ifdef ENABLE_REFLEX_SMS
  RZORD_SMS,  //12
#endif
#ifdef ENABLE_REFLEX_JPC
  RZORD_JPC,  //13
#endif
  RZORD_LAST //this must be the last enum value
};

DeviceEnum deviceMode = RZORD_NONE;

#ifdef ENABLE_SERIAL_DEBUG
  #define dstart(spd) do {Serial.begin (spd); while (!Serial) {digitalWrite (LED_BUILTIN, (millis () / 500) % 2);}} while (0);
  #define debug(...) Serial.print (__VA_ARGS__)
  #define debugln(...) Serial.println (__VA_ARGS__)
#else
  #define dstart(...)
  #define debug(...)
  #define debugln(...)
#endif

#ifdef REFLEX_USE_OLED_DISPLAY
  #include "src/SSD1306Ascii/SSD1306Ascii.h"
  #include "src/SSD1306Ascii/SSD1306AsciiAvrI2c.h"
  #include "ReflexPad5x7.h"
  
  #define I2C_ADDRESS 0x3C // 0X3C+SA0 - 0x3C or 0x3D
  SSD1306AsciiAvrI2c display;

  #ifdef ENABLE_REFLEX_LOGO
    const uint8_t oledDisplayFirstRow = 2;//3
  #else
    const uint8_t oledDisplayFirstRow = 0;
  #endif

  unsigned long oledDisplayTimer = 0;
  bool oledOn = true;

  //clear(0, displayWidth() - 1, 0 , displayRows() - 1);
  void clearOledDisplay() {
    display.clear(0, 127, oledDisplayFirstRow, 7);
  }

  void clearOledLineAndPrint(const uint8_t col, const uint8_t row, const __FlashStringHelper* value) {
    display.setCursor(col, oledDisplayFirstRow + row);
    display.clearToEOL();
    display.print(value);
  }

  void setOledDisplay(const bool state) {
    oledOn = state;
    display.ssd1306WriteCmd(state ? SSD1306_DISPLAYON : SSD1306_DISPLAYOFF);
    oledDisplayTimer = millis();
  }

  #ifdef ENABLE_REFLEX_PAD
    struct PadDivision {
      uint8_t firstCol;
      uint8_t lastCol;
    };

    const PadDivision padDivision[] =  {
      { 0,    10*6 },
      { 11*6, 127  }
    };

    struct Pad {
      uint32_t padvalue;
      uint8_t row;
      uint8_t col;
      char on;
      char off;
    };
  
    #define UP_ON 34
    #define UP_OFF 35
    #define DOWN_ON 36
    #define DOWN_OFF 37
    #define LEFT_ON 38
    #define LEFT_OFF 39
    #define RIGHT_ON 40
    #define RIGHT_OFF 41
    #define FACEBTN_ON 59
    #define FACEBTN_OFF 60
    #define SHOULDERBTN_ON 62
    #define SHOULDERBTN_OFF 63
    #define DASHBTN_ON '^'
    #define DASHBTN_OFF '`'
    #define RECTANGLEBTN_ON '{'
    #define RECTANGLEBTN_OFF '}'
  
    void PrintPadChar(const uint8_t col, const uint8_t row, const char value) {
      display.setCursor(col, row);
      display.print(value);
    }
  
    void PrintPadChar(const uint8_t padIndex, const uint8_t startCol, const uint8_t col, const uint8_t row, const uint32_t bitMask, const bool isPressed, const char valueOn, const char valueOff, const bool force = false) {
      static uint32_t state[] = {0,0};
      const bool oledIsOn = state[padIndex] & bitMask;
    
      if(isPressed == oledIsOn && !force)
        return;
    
      PrintPadChar(startCol + col, oledDisplayFirstRow + 1 + row, isPressed ? valueOn : valueOff);
    
      if(isPressed)
        state[padIndex] |= bitMask;
      else
        state[padIndex] &= ~ bitMask;
    }
  #endif //ENABLE_REFLEX_PAD


#endif //REFLEX_USE_OLED_DISPLAY



#ifdef RZORD_ENABLE_AUTORESET
#include <avr/wdt.h>
void resetDevice(){
  //Set all used Leonardo pins as INPUT LOW.
  //Comment if need to free space, not using a leonardo or not based on ATmega32U4
  DDRB &= B00000001; //bits .1234567
  PORTB &= B00000001;
  DDRC &= B10111111; //bits ......6.
  PORTC &= B10000000;
  DDRD &= B00101100; //bits 01..4.67
  PORTD &= B00101100;
  DDRE &= B01000000; //bits ......6.
  PORTE &= B01000000;
  DDRF &= B00001100; //bits 01..4567
  PORTF &= B00001100;

  //use the watchdog to reset the arduino
  bool ledmode = false;
  wdt_enable(WDTO_4S);
  while(1) {
    //blink led fast to indicate that mode will change
    digitalWrite(LED_BUILTIN, ledmode = !ledmode);
    delay(90);
  }
}
#endif
//
//void blinkLed() {
//  #if REFLEX_VERSION == 2
//    TXLED1;//OFF
//    delay(500);
//    TXLED0;//ON
//    delay(500);
//  #else
//    digitalWrite(LED_BUILTIN, HIGH);
//    delay(500);
//    digitalWrite(LED_BUILTIN, LOW);
//    delay(500);
//  #endif
//}

#endif //RZORD_SHARED_H_
