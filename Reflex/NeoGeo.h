/*******************************************************************************
 * Neogeo pad reading with debounce
*/

#include "src/ArduinoJoystickLibrary/Joy1.h"
#include "src/ZordButton/ZordButton.h"

//Neogeo pins
//https://old.pinouts.ru/Game/NeoGeoJoystick_pinout.shtml
#define NEOGEOPIN_UP     15 //15  F6
#define NEOGEOPIN_DOWN   5  //7   C6
#if REFLEX_PIN_VERSION == 1
  #define NEOGEOPIN_LEFT   3 //14  C7
#else
  #define NEOGEOPIN_LEFT   13 //14  C7
#endif
#define NEOGEOPIN_RIGHT  6  //6   D7
#define NEOGEOPIN_A      16 //13  F5
#define NEOGEOPIN_B      7  //5   E6
#define NEOGEOPIN_C      4  //12  D4
#define NEOGEOPIN_D      8  //4   B4
#define NEOGEOPIN_SELECT 9  //3   B5
#define NEOGEOPIN_START  14 //11  F7

#define NEOGEO_TOTAL_PINS 10

static const uint8_t buttonPins[NEOGEO_TOTAL_PINS] = {
  NEOGEOPIN_UP, NEOGEOPIN_DOWN, NEOGEOPIN_LEFT, NEOGEOPIN_RIGHT,
  NEOGEOPIN_A, NEOGEOPIN_B, NEOGEOPIN_C, NEOGEOPIN_D, 
  NEOGEOPIN_SELECT, NEOGEOPIN_START };

static const uint16_t outputMask[NEOGEO_TOTAL_PINS] = {
  (0x1), (0x1 << 1), (0x1 << 2), (0x1 << 3), 
  (0x1 << 1), (0x1 << 2), (0x1 << 0), (0x1 << 3), 
  (0x1 << 8), (0x1 << 9) };
  
//static Bugtton* buttons;
static ArcadePad* buttons;


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

  buttons = new ArcadePad(NEOGEO_TOTAL_PINS, buttonPins, NEOGEO_DEBOUNCE);

  buttons->begin();
  
  //Create usb controllers
  usbStick[0] = new Joy1_("RZordNeoGeo", JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_GAMEPAD, totalUsb);
  
  //Set usb parameters and reset to default values
  usbStick[0]->resetState();
  usbStick[0]->sendState();

  dstart (115200);
  
  sleepTime = 0;
}

inline bool __attribute__((always_inline))
neogeoLoop() {
  #ifdef ENABLE_REFLEX_PAD
  static bool firstTime = true;
  if(firstTime) {
    firstTime = false;
    ShowDefaultPadNeoGeo();
  }
  #endif

  //update debounce lib
  const bool stateChanged = buttons->update();

  if(stateChanged) {
    uint8_t dpadState = 0xF0;
    uint16_t buttonData = 0x0;
    
    for(uint8_t i = 0; i < NEOGEO_TOTAL_PINS; i++) {
      const bool isPressed = buttons->state(i) == LOW;
      if(isPressed) {
        if(i < 4) { //Dpad
          dpadState |= outputMask[i];
        } else { //Buttons
          buttonData |= outputMask[i];
        }
      }
      
      #ifdef ENABLE_REFLEX_PAD
        const Pad pad = padNeoGeo[i];
        PrintPadChar(0, 35, pad.col, pad.row, pad.padvalue, isPressed, pad.on, pad.off);
      #endif
    }

    dpadState = ~dpadState; //Dpad hatTable uses active low logic
    
    ((Joy1_*)usbStick[0])->setButtons(buttonData);
    ((Joy1_*)usbStick[0])->setHatSwitch(hatTable[dpadState]);
    usbStick[0]->sendState();
  }

  return stateChanged; //false
}
