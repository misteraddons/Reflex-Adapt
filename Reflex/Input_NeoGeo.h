/*******************************************************************************
 * Reflex Adapt USB
 * Neogeo input module - without debounce
 * 
 * Uses a modified version of Joystick Library
 * https://github.com/MHeironimus/ArduinoJoystickLibrary
 * 
*/

#include "src/ArduinoJoystickLibrary/Joy1.h"
#include "src/DigitalIO/DigitalIO.h"

//Neogeo pins
//https://old.pinouts.ru/Game/NeoGeoJoystick_pinout.shtml
#define NEOGEOPIN_UP     15
#define NEOGEOPIN_DOWN   5
#define NEOGEOPIN_LEFT   13
#define NEOGEOPIN_RIGHT  6
#define NEOGEOPIN_A      16
#define NEOGEOPIN_B      7
#define NEOGEOPIN_C      4
#define NEOGEOPIN_D      8
#define NEOGEOPIN_SELECT 9
#define NEOGEOPIN_START  14

#define NEOGEO_TOTAL_PINS 10

DigitalPin<NEOGEOPIN_UP>     ngeo_U;
DigitalPin<NEOGEOPIN_DOWN>   ngeo_D;
DigitalPin<NEOGEOPIN_LEFT>   ngeo_L;
DigitalPin<NEOGEOPIN_RIGHT>  ngeo_R;
DigitalPin<NEOGEOPIN_A>      ngeo_BA;
DigitalPin<NEOGEOPIN_B>      ngeo_BB;
DigitalPin<NEOGEOPIN_C>      ngeo_BC;
DigitalPin<NEOGEOPIN_D>      ngeo_BD;
DigitalPin<NEOGEOPIN_SELECT> ngeo_SELECT;
DigitalPin<NEOGEOPIN_START>  ngeo_START;
  
#ifdef ENABLE_REFLEX_PAD
  const Pad padNeoGeo[] = {
    { 0x0001, 1, 1*6, UP_ON, UP_OFF },//UP
    { 0x0002, 3, 1*6, DOWN_ON, DOWN_OFF },//DOWN
    { 0x0004, 2, 0,   LEFT_ON, LEFT_OFF },//LEFT
    { 0x0008, 2, 2*6, RIGHT_ON, RIGHT_OFF },//RGHT
    { 0x0010, 3, 7*6, FACEBTN_ON, FACEBTN_OFF },//A
    { 0x0020, 2, 8*6, FACEBTN_ON, FACEBTN_OFF },//B
    { 0x0040, 2, 6*6, FACEBTN_ON, FACEBTN_OFF },//C
    { 0x0080, 1, 7*6, FACEBTN_ON, FACEBTN_OFF },//D
    { 0x0100, 2, 4*6, DASHBTN_ON, DASHBTN_OFF },//SELECT
    { 0x0200, 3, 4*6, DASHBTN_ON, DASHBTN_OFF }//START
  };

  void ShowDefaultPadNeoGeo() {
    //print default joystick state to oled screen
    const uint8_t index = 0;
    const uint8_t firstCol = 35;//padDivision[index].firstCol;
    const uint8_t lastCol = 127;//padDivision[index].lastCol;

    display.clear(firstCol, lastCol, oledDisplayFirstRow + 1, 7);
    //display.setCursor(firstCol, 7);
    //display.print(F("NEOGEO"));
  
    //const uint8_t startCol = 0;
    for(uint8_t x = 0; x < NEOGEO_TOTAL_PINS; x++){
      const Pad pad = padNeoGeo[x];
      PrintPadChar(index, firstCol, pad.col, pad.row, pad.padvalue, false, pad.on, pad.off, true);
    }
  }

#endif

void neogeoSetup() {

  //init input pins with pull-up
  ngeo_U.config(INPUT, HIGH);
  ngeo_D.config(INPUT, HIGH);
  ngeo_L.config(INPUT, HIGH);
  ngeo_R.config(INPUT, HIGH);
  ngeo_BA.config(INPUT, HIGH);
  ngeo_BB.config(INPUT, HIGH);
  ngeo_BC.config(INPUT, HIGH);
  ngeo_BD.config(INPUT, HIGH);
  ngeo_SELECT.config(INPUT, HIGH);
  ngeo_START.config(INPUT, HIGH);
  
  //Create usb controllers
  usbStick[0] = new Joy1_("ReflexNeoGeo", JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_GAMEPAD, totalUsb);
  
  //Set usb parameters and reset to default values
  usbStick[0]->resetState();
  usbStick[0]->sendState();

  dstart (115200);
  
  sleepTime = 0;
}

inline bool __attribute__((always_inline))
neogeoLoop() {
  static uint16_t lastState = 0x03FF;
  bool stateChanged = false;
  
  #ifdef ENABLE_REFLEX_PAD
  static bool firstTime = true;
  if(firstTime) {
    firstTime = false;
    ShowDefaultPadNeoGeo();
  }
  #endif

  //Read the neogeo port
  const uint16_t portState = (
      (ngeo_START << 9)
    | (ngeo_SELECT << 8)
    | (ngeo_BD << 7)
    | (ngeo_BC << 6)
    | (ngeo_BB << 5)
    | (ngeo_BA << 4)
    | (ngeo_R << 3)
    | (ngeo_L << 2)
    | (ngeo_D << 1)
    | ngeo_U);
  
  //Only process data if state changed from previous read
  if (lastState != portState) {
    stateChanged = true;
    lastState = portState;

    uint16_t buttonData = 0;
    bitWrite(buttonData, 1, ~portState & 0x010); //A
    bitWrite(buttonData, 2, ~portState & 0x020); //B
    bitWrite(buttonData, 0, ~portState & 0x040); //C
    bitWrite(buttonData, 3, ~portState & 0x080); //D
    bitWrite(buttonData, 8, ~portState & 0x100); //SEL
    bitWrite(buttonData, 9, ~portState & 0x200); //STA

    ((Joy1_*)usbStick[0])->setButtons(buttonData);

    //Get angle from hatTable and pass to joystick class
    ((Joy1_*)usbStick[0])->setHatSwitch(hatTable[portState & 0xF]);

    usbStick[0]->sendState();

    #ifdef ENABLE_REFLEX_PAD
      for(uint8_t i = 0; i < NEOGEO_TOTAL_PINS; ++i) {
        const bool isPressed = !(portState & (1 << i));
        const Pad pad = padNeoGeo[i];
        PrintPadChar(0, 35, pad.col, pad.row, pad.padvalue, isPressed, pad.on, pad.off);
      }
    #endif
  }

  return stateChanged;
}
