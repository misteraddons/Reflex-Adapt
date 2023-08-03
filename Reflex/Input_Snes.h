/*******************************************************************************
 * Reflex Adapt USB
 * NES/SNES/VB input module
 * 
 * Uses SnesLib
 * https://github.com/sonik-br/SnesLib
 * 
 * Uses a modified version of Joystick Library
 * https://github.com/MHeironimus/ArduinoJoystickLibrary
*/

#include "src/SnesLib/SnesLib.h"
#include "src/ArduinoJoystickLibrary/Joy1.h"

//Snes pins - Port 1
#define SNES1_CLOCK  9
#define SNES1_LATCH  8
#define SNES1_DATA1  7
#define SNES1_DATA2  5
#define SNES1_SELECT 4

//Snes pins - Port 2
#define SNES2_CLOCK  20
#define SNES2_LATCH  19
#define SNES2_DATA1  18
#define SNES2_DATA2  14
#define SNES2_SELECT 16

#ifdef SNES_ENABLE_MULTITAP
  SnesPort<SNES1_CLOCK, SNES1_LATCH, SNES1_DATA1, SNES1_DATA2, SNES1_SELECT> snes1;
  SnesPort<SNES2_CLOCK, SNES2_LATCH, SNES2_DATA1, SNES2_DATA2, SNES2_SELECT> snes2;
#else
  SnesPort<SNES1_CLOCK, SNES1_LATCH, SNES1_DATA1> snes1;
  SnesPort<SNES2_CLOCK, SNES2_LATCH, SNES2_DATA1> snes2;
#endif

bool isVirtualBoy = false;

#ifdef ENABLE_REFLEX_PAD
  const Pad padSnes[] = {
    { SNES_B,      3, 8*6, FACEBTN_ON, FACEBTN_OFF },
    { SNES_Y,      2, 7*6, FACEBTN_ON, FACEBTN_OFF },
    { SNES_SELECT, 2, 4*6, DASHBTN_ON, DASHBTN_OFF },
    { SNES_START,  2, 5*6, DASHBTN_ON, DASHBTN_OFF },
    { SNES_UP,     1, 1*6, UP_ON, UP_OFF },
    { SNES_DOWN,   3, 1*6, DOWN_ON, DOWN_OFF },
    { SNES_LEFT,   2, 0,   LEFT_ON, LEFT_OFF },
    { SNES_RIGHT,  2, 2*6, RIGHT_ON, RIGHT_OFF },
    { SNES_A,      2, 9*6, FACEBTN_ON, FACEBTN_OFF },
    { SNES_X,      1, 8*6, FACEBTN_ON, FACEBTN_OFF },
    { SNES_L,      0, 1*6, SHOULDERBTN_ON, SHOULDERBTN_OFF },
    { SNES_R,      0, 8*6, SHOULDERBTN_ON, SHOULDERBTN_OFF },
    
    //NES specific - Horizontal aligned
    { SNES_Y,      3, 7*6, FACEBTN_ON, FACEBTN_OFF },
    { SNES_B,      3, 8*6, FACEBTN_ON, FACEBTN_OFF },
    { SNES_SELECT, 2, 4*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
    { SNES_START,  2, 5*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF }
  };

  #ifdef SNES_ENABLE_VBOY
    const Pad padVB[] = {
      { SNES_B,      3, 8*6, DOWN_ON, DOWN_OFF }, //VB Right D-pad Down, NES A
      { SNES_Y,      2, 7*6, LEFT_ON, LEFT_OFF }, //VB Right D-pad Left, NES B
      { SNES_SELECT, 3, (3*6)-2, FACEBTN_ON, FACEBTN_OFF },
      { SNES_START,  3, (4*6)-2, FACEBTN_ON, FACEBTN_OFF },
      { SNES_UP,     1, 1*6, UP_ON, UP_OFF },
      { SNES_DOWN,   3, 1*6, DOWN_ON, DOWN_OFF },
      { SNES_LEFT,   2, 0,   LEFT_ON, LEFT_OFF },
      { SNES_RIGHT,  2, 2*6, RIGHT_ON, RIGHT_OFF },
      { SNES_A,      2, 9*6, RIGHT_ON, RIGHT_OFF }, //VB Right D-pad Right
      { SNES_X,      1, 8*6, UP_ON, UP_OFF }, //VB Right D-pad Up
      { SNES_L,      0, 1*6, SHOULDERBTN_ON, SHOULDERBTN_OFF },
      { SNES_R,      0, 8*6, SHOULDERBTN_ON, SHOULDERBTN_OFF },
      { SNES_NTT_0 << 12, 3, (5*6)+2, FACEBTN_ON, FACEBTN_OFF }, //VB B
      { SNES_NTT_1 << 12, 3, (6*6)+2, FACEBTN_ON, FACEBTN_OFF }  //VB A
    };
  #endif

  bool ShowSnesPadName(const SnesDeviceType_Enum padType) {
    switch(padType) {
      case SNES_DEVICE_NES:
        display.print(F("NES"));
        return true;
      case SNES_DEVICE_PAD:
        display.print(F("SNES"));
        return true;
      case SNES_DEVICE_NTT:
        display.print(F("NTT"));
        return true;
      case SNES_DEVICE_VB:
        display.print(F("VBOY"));
        return true;
      default:
        display.print(PSTR_TO_F(PSTR_NONE));
        return false;
    }
  }
  
  void ShowDefaultPadSnes(const uint8_t index, const SnesDeviceType_Enum padType) {
    //print default joystick state to oled screen
  
    //const uint8_t firstCol = index == 0 ? 0 : 11*6;
    //const uint8_t lastCol = index == 0 ? 11*6 : 127;

    display.clear(padDivision[index].firstCol, padDivision[index].lastCol, oledDisplayFirstRow + 1, 7);
    display.setCursor(padDivision[index].firstCol, 7);

    if(!ShowSnesPadName(padType))
      return;
  
    if (index < 2) {
      //const uint8_t startCol = index == 0 ? 0 : 11*6;
      #ifdef SNES_ENABLE_VBOY
      if (isVirtualBoy) {
        for(uint8_t x = 0; x < 14; x++){
          const Pad pad = padVB[x];
          PrintPadChar(index, padDivision[index].firstCol, pad.col, pad.row, pad.padvalue, true, pad.on, pad.off, true);
        }
      } else 
      #endif
      {
        for(uint8_t x = 0; x < 12; x++){
          if(padType == SNES_DEVICE_NES && x > 7)
            continue;
          const Pad pad = (padType == SNES_DEVICE_NES && x < 4) ? padSnes[x+12] : padSnes[x]; //NES uses horizontal align
          PrintPadChar(index, padDivision[index].firstCol, pad.col, pad.row, pad.padvalue, true, pad.on, pad.off, true);
        }        
      }

    }
  }

#endif


void snesResetJoyValues(const uint8_t i) {
  if (i >= totalUsb)
    return;

  usbStick[i]->resetState();
}

void snesSetup() {
  //Init the class
  snes1.begin();
  snes2.begin();

  delayMicroseconds(10);

  //Multitap is connected?
   const uint8_t tap = snes1.getMultitapPorts();
  if (tap == 0){ //No multitap connected during boot
    totalUsb = 2;
    sleepTime = 50;

    //check if theres a virtualboy controller on first port only
    #ifdef SNES_ENABLE_VBOY
      snes1.update();
      for(uint8_t i = 0; i < snes1.getControllerCount(); i++) {
        if (i == 0)
          isVirtualBoy = snes1.getSnesController(i).deviceType() == SNES_DEVICE_VB;
        snes1.getSnesController(i).reset(true, true);
      }
    #endif
    
  } else { //Multitap connected
    totalUsb = MAX_USB_STICKS; //min(tap, MAX_USB_STICKS);
    sleepTime = 1000; //use longer interval between reads for multitap
  }
  //sleepTime = 50;

  //totalUsb = 4;

  //Create usb controllers
  for (uint8_t i = 0; i < totalUsb; i++) {
    usbStick[i] = new Joy1_(isVirtualBoy ? "RZordVboy" : "RZordSnesNtt", JOYSTICK_DEFAULT_REPORT_ID + i, JOYSTICK_TYPE_GAMEPAD, totalUsb);
  }

  //Set usb parameters and reset to default values
  for (uint8_t i = 0; i < totalUsb; i++) {
      snesResetJoyValues(i);
      usbStick[i]->sendState();
  }

  delayMicroseconds(sleepTime);
  
  dstart (115200);
}

inline bool __attribute__((always_inline))
snesLoop() {
  static uint8_t lastControllerCount = 0;
  #ifdef ENABLE_REFLEX_PAD
    static SnesDeviceType_Enum lastPadType[] = { SNES_DEVICE_NOTSUPPORTED, SNES_DEVICE_NOTSUPPORTED, SNES_DEVICE_NOTSUPPORTED, SNES_DEVICE_NOTSUPPORTED, SNES_DEVICE_NOTSUPPORTED, SNES_DEVICE_NOTSUPPORTED };
    SnesDeviceType_Enum currentPadType[] = { SNES_DEVICE_NONE, SNES_DEVICE_NONE, SNES_DEVICE_NONE, SNES_DEVICE_NONE, SNES_DEVICE_NONE, SNES_DEVICE_NONE };

    static bool firstTime = true;
    if(firstTime) {
      firstTime = false;
      //Multitap mode!
      if(totalUsb > 2) {
        display.setCursor(6*6, oledDisplayFirstRow + 3);
        display.print(F("MULTI-TAP"));
      }
    }

  #endif
  bool stateChanged = false;

  //Read snes port
  snes1.update();
  snes2.update();
  
  //Get the number of connected controllers
  const uint8_t joyCount1 = snes1.getControllerCount();
  const uint8_t joyCount2 = snes2.getControllerCount();
  const uint8_t joyCount = joyCount1 + joyCount2;

  for (uint8_t i = 0; i < joyCount; i++) {
    if (i == totalUsb)
      break;
      
    //Get the data for the specific controller
    const uint8_t inputPort = (i < joyCount1) ? 0 : 1;
    const SnesController& sc = inputPort == 0 ? snes1.getSnesController(i) : snes2.getSnesController(i - joyCount1);

    #ifdef ENABLE_REFLEX_PAD
      //Only used if not in multitap mode
      if(totalUsb == 2) {
        if(inputPort == 0 && joyCount1 != 0)
          currentPadType[inputPort] = sc.deviceType();
        else if(inputPort == 1 && joyCount2 != 0)
          currentPadType[inputPort] = sc.deviceType();
      }
    #endif
    
    //Only process data if state changed from previous read
    if(sc.stateChanged()) {
      stateChanged = true;
      
      //Controller just connected.
      if (sc.deviceJustChanged()) {
        snesResetJoyValues(i);
        #ifdef ENABLE_REFLEX_PAD
          //Only used if not in multitap mode
          if (totalUsb == 2)
            ShowDefaultPadSnes(inputPort, sc.deviceType());
        #endif
      }

      const SnesDeviceType_Enum padType = sc.deviceType();

      uint8_t hatData = sc.hat();
      uint32_t buttonData = 0;

      if (padType == SNES_DEVICE_NES) {
        //Remaps as SNES B and A
        //bitWrite(buttonData, 1, sc.digitalPressed(SNES_Y));
        //bitWrite(buttonData, 2, sc.digitalPressed(SNES_B));

        //Same map as on a real snes console
        bitWrite(buttonData, 0, sc.digitalPressed(SNES_Y));
        bitWrite(buttonData, 1, sc.digitalPressed(SNES_B));
      } else {
        bitWrite(buttonData, 0, sc.digitalPressed(SNES_Y));
        bitWrite(buttonData, 1, sc.digitalPressed(SNES_B));
        bitWrite(buttonData, 2, sc.digitalPressed(SNES_A));
        bitWrite(buttonData, 3, sc.digitalPressed(SNES_X));
        bitWrite(buttonData, 4, sc.digitalPressed(SNES_L));
        bitWrite(buttonData, 5, sc.digitalPressed(SNES_R));

        if(padType == SNES_DEVICE_NTT) {
          bitWrite(buttonData, 6, sc.nttPressed(SNES_NTT_DOT));
          bitWrite(buttonData, 7, sc.nttPressed(SNES_NTT_C));
          bitWrite(buttonData, 10, sc.nttPressed(SNES_NTT_0));
          bitWrite(buttonData, 11, sc.nttPressed(SNES_NTT_1));
          bitWrite(buttonData, 12, sc.nttPressed(SNES_NTT_2));
          bitWrite(buttonData, 13, sc.nttPressed(SNES_NTT_3));
          bitWrite(buttonData, 14, sc.nttPressed(SNES_NTT_4));
          bitWrite(buttonData, 15, sc.nttPressed(SNES_NTT_5));
          bitWrite(buttonData, 16, sc.nttPressed(SNES_NTT_6));
          bitWrite(buttonData, 17, sc.nttPressed(SNES_NTT_7));
          bitWrite(buttonData, 18, sc.nttPressed(SNES_NTT_8));
          bitWrite(buttonData, 19, sc.nttPressed(SNES_NTT_9));
          bitWrite(buttonData, 20, sc.nttPressed(SNES_NTT_STAR));
          bitWrite(buttonData, 21, sc.nttPressed(SNES_NTT_HASH));
          bitWrite(buttonData, 22, sc.nttPressed(SNES_NTT_EQUAL));
        } else if(padType == SNES_DEVICE_VB) {
          bitWrite(buttonData, 6, sc.nttPressed(SNES_NTT_0));
          bitWrite(buttonData, 7, sc.nttPressed(SNES_NTT_1));
        }

      }
      bitWrite(buttonData, 8, sc.digitalPressed(SNES_SELECT));
      bitWrite(buttonData, 9, sc.digitalPressed(SNES_START));

      ((Joy1_*)usbStick[i])->setButtons(buttonData);

      //Get angle from hatTable and pass to joystick class
      ((Joy1_*)usbStick[i])->setHatSwitch(hatTable[hatData]);
      

      usbStick[i]->sendState();
      #ifdef ENABLE_REFLEX_PAD
        //Only used if not in multitap mode
        if (totalUsb == 2 && inputPort < 2) {
          //const uint8_t startCol = inputPort == 0 ? 0 : 11*6;

          #ifdef SNES_ENABLE_VBOY
            if (isVirtualBoy) {
              for(uint8_t x = 0; x < 14; x++){
//                if(padType == SNES_DEVICE_NES && x > 7)
//                  continue;
                const Pad pad = padVB[x];
                if (x < 12) {
                  if (padType == SNES_DEVICE_NES && x > 7)
                    PrintPadChar(inputPort, padDivision[inputPort].firstCol, pad.col, pad.row, pad.padvalue, false, pad.on, pad.off);
                  else
                    PrintPadChar(inputPort, padDivision[inputPort].firstCol, pad.col, pad.row, pad.padvalue, sc.digitalPressed((SnesDigital_Enum)pad.padvalue), pad.on, pad.off);
                } else {
                  PrintPadChar(inputPort, padDivision[inputPort].firstCol, pad.col, pad.row, pad.padvalue, padType == SNES_DEVICE_VB && sc.nttPressed((SnesDigitalNTT_Enum)(pad.padvalue >> 12)), pad.on, pad.off);
                }
              }
            } else
          #endif
          {
            for(uint8_t x = 0; x < 12; x++){
              if(padType == SNES_DEVICE_NES && x > 7)
                continue;
              const Pad pad = (padType == SNES_DEVICE_NES && x < 4) ? padSnes[x+12] : padSnes[x]; //NES uses horizontal align
              PrintPadChar(inputPort, padDivision[inputPort].firstCol, pad.col, pad.row, pad.padvalue, sc.digitalPressed((SnesDigital_Enum)pad.padvalue), pad.on, pad.off);
            }
          }
        }
      #endif
        
    }
  }
  #ifdef ENABLE_REFLEX_PAD
    for (uint8_t i = 0; i < 2; i++) {
      //Only used if not in multitap mode
      if(totalUsb == 2 && lastPadType[i] != currentPadType[i] && currentPadType[i] == SNES_DEVICE_NONE)
        ShowDefaultPadSnes(i, SNES_DEVICE_NONE);
      lastPadType[i] = currentPadType[i];
    }
  #endif

  //Controller has been disconnected? Reset it's values!
  if (lastControllerCount > joyCount) {
    for (uint8_t i = joyCount; i < lastControllerCount; i++) {
      if (i == totalUsb)
        break;
      snesResetJoyValues(i);
      usbStick[i]->sendState();
    }
  } 

  //Keep count for next read
  lastControllerCount = joyCount;
  
  return stateChanged;
}
