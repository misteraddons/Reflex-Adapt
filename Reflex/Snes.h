/*******************************************************************************
 * Snes controller input library.
 * https://github.com/sonik-br/SnesLib
*/

//Uncomment to enable multitap support. Requires wiring two additional pins.
//#define SNES_ENABLE_MULTITAP

//#define SNES_MULTI_CONNECTION 4

#include "src/SnesLib/SnesLib.h"

#ifdef SNES_ENABLE_NTTPAD
  #include "src/ArduinoJoystickLibrary/Joy1.h"
#else
  #include "src/ArduinoJoystickLibrary/Joy1.h"
#endif


//Snes joy 1 pins
#define SNES1_CLOCK  9
#define SNES1_LATCH  8
#define SNES1_DATA1  7
#define SNES1_DATA2  5
#define SNES1_SELECT 4

//Snes joy 2 pins. joy 2 to 6 shares same CLOCK and LATCH pins
#define SNES2_CLOCK  20
#define SNES2_LATCH  19
#define SNES2_DATA1  18
#define SNES2_DATA2  14
#define SNES2_SELECT  16

//Snes joy 3 pins
#define SNES3_DATA1  6

//Snes joy 4 pins
#if REFLEX_PIN_VERSION == 1
  #define SNES4_DATA1  2
#else
  #define SNES4_DATA1  21
#endif

//Snes joy 5 pins
#define SNES5_DATA1  10

//Snes joy 6 pins
#if REFLEX_PIN_VERSION == 1
  #define SNES6_DATA1  3
#else
  #define SNES6_DATA1  13
#endif

#ifdef SNES_ENABLE_MULTITAP
  //SnesPort<SNES1_CLOCK, SNES1_LATCH, SNES1_DATA1, SNES1_DATA2, SNES1_SELECT> snes;
  //SnesPort<SNES2_CLOCK, SNES2_LATCH, SNES2_DATA1> snes2;
  //SnesPort<SNES2_CLOCK, SNES2_LATCH, SNES3_DATA1> snes3;
  //SnesPort<SNES2_CLOCK, SNES2_LATCH, SNES4_DATA1> snes4;
  //SnesPort<SNES2_CLOCK, SNES2_LATCH, SNES5_DATA1> snes5;
  //SnesPort<SNES2_CLOCK, SNES2_LATCH, SNES6_DATA1> snes6;

  SnesPort<SNES1_CLOCK, SNES1_LATCH, SNES1_DATA1, SNES1_DATA2, SNES1_SELECT> snes1;
  //SnesPort<SNES2_CLOCK, SNES2_LATCH, SNES2_DATA1, 14, 16> snes2;
  SnesPort<SNES2_CLOCK, SNES2_LATCH, SNES2_DATA1, SNES2_DATA2, SNES2_SELECT> snes2;
#else
  /*SnesPort<SNES1_CLOCK, SNES1_LATCH, SNES1_DATA1> snes1;
  SnesPort<SNES2_CLOCK, SNES2_LATCH, SNES2_DATA1> snes2;
  SnesPort<SNES2_CLOCK, SNES2_LATCH, SNES3_DATA1> snes3;
  SnesPort<SNES2_CLOCK, SNES2_LATCH, SNES4_DATA1> snes4;
  SnesPort<SNES2_CLOCK, SNES2_LATCH, SNES5_DATA1> snes5;
  SnesPort<SNES2_CLOCK, SNES2_LATCH, SNES6_DATA1> snes6;*/


  SnesPort<SNES1_CLOCK, SNES1_LATCH, SNES1_DATA1> snes1;
  SnesPort<SNES2_CLOCK, SNES2_LATCH, SNES2_DATA1> snes2;

  //SnesPort<SNES2_CLOCK, SNES2_LATCH, SNES2_DATA1, SNES3_DATA1, SNES4_DATA1, SNES5_DATA1> snes1;

  //SnesPort<SNES1_CLOCK, SNES1_LATCH, SNES1_DATA1> snes1;
  //SnesPort<SNES1_CLOCK, SNES1_LATCH, SNES1_DATA1, SNES2_DATA1> snes1;
  //SnesPort<SNES2_CLOCK, SNES2_LATCH, SNES3_DATA1, SNES4_DATA1> snes2;
  //SnesPort<SNES2_CLOCK, SNES2_LATCH, SNES2_DATA1, SNES3_DATA1, SNES4_DATA1> snes2;
  //SnesPort<SNES2_CLOCK, SNES2_LATCH, SNES3_DATA1> snes3;
  //SnesPort<SNES2_CLOCK, SNES2_LATCH, SNES4_DATA1> snes4;
  //SnesPort<SNES2_CLOCK, SNES2_LATCH, SNES5_DATA1> snes5;
  //SnesPort<SNES2_CLOCK, SNES2_LATCH, SNES6_DATA1> snes6;
#endif

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
    
    //NES specific - Horizontal align
    { SNES_Y,      3, 7*6, FACEBTN_ON, FACEBTN_OFF },
    { SNES_B,      3, 8*6, FACEBTN_ON, FACEBTN_OFF },
    { SNES_SELECT, 2, 4*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
    { SNES_START,  2, 5*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF }
  };
  
  void ShowDefaultPadSnes(const uint8_t index, const SnesDeviceType_Enum padType) {
    //print default joystick state to oled screen
  
    //const uint8_t firstCol = index == 0 ? 0 : 11*6;
    //const uint8_t lastCol = index == 0 ? 11*6 : 127;

    display.clear(padDivision[index].firstCol, padDivision[index].lastCol, oledDisplayFirstRow + 1, 7);
    display.setCursor(padDivision[index].firstCol, 7);

    switch(padType) {
      case SNES_DEVICE_NES:
        display.print(F("NES"));
        break;
      case SNES_DEVICE_PAD:
        display.print(F("SNES"));
        break;
      case SNES_DEVICE_NTT:
        display.print(F("NTT"));
        break;
      case SNES_DEVICE_VB:
        display.print(F("VBOY"));
        break;
      default:
        display.print(F("NONE"));
        return;
    }
  
    if (index < 2) {
      //const uint8_t startCol = index == 0 ? 0 : 11*6;
      for(uint8_t x = 0; x < 12; x++){
        if(padType == SNES_DEVICE_NES && x > 7)
          continue;
        const Pad pad = (padType == SNES_DEVICE_NES && x < 4) ? padSnes[x+12] : padSnes[x]; //NES uses horizontal align
        PrintPadChar(index, padDivision[index].firstCol, pad.col, pad.row, pad.padvalue, true, pad.on, pad.off, true);
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
  //snes3.begin();
  //snes4.begin();
  //snes5.begin();
  //snes6.begin();

  delayMicroseconds(10);

  //Multitap is connected?
   const uint8_t tap = snes1.getMultitapPorts();
  if (tap == 0){ //No multitap connected during boot
    totalUsb = 2;
    sleepTime = 50;
  } else { //Multitap connected
    totalUsb = MAX_USB_STICKS; //min(tap, MAX_USB_STICKS);
    sleepTime = 1000; //use longer interval between reads for multitap
  }
  //sleepTime = 50;

  //totalUsb = 4;

  //Create usb controllers
  for (uint8_t i = 0; i < totalUsb; i++) {
#ifdef SNES_ENABLE_NTTPAD
  usbStick[i] = new Joy1_("RZordSnesNtt", JOYSTICK_DEFAULT_REPORT_ID + i, JOYSTICK_TYPE_GAMEPAD, totalUsb);
#else
  usbStick[i] = new Joy1_("RZordSnes", JOYSTICK_DEFAULT_REPORT_ID + i, JOYSTICK_TYPE_GAMEPAD, totalUsb);
#endif
  }

  //Set usb parameters and reset to default values
  for (uint8_t i = 0; i < totalUsb; i++) {
      snesResetJoyValues(i);
      usbStick[i]->sendState();
  }
  
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
  //snes3.update();
  //snes4.update();
  //snes5.update();
  //snes6.update();

  
  //Get the number of connected controllers
  const uint8_t joyCount1 = snes1.getControllerCount();
  const uint8_t joyCount2 = snes2.getControllerCount();
  //const uint8_t joyCount3 = snes3.getControllerCount();
  //const uint8_t joyCount4 = snes4.getControllerCount();
  //const uint8_t joyCount5 = snes5.getControllerCount();
  //const uint8_t joyCount6 = snes6.getControllerCount();
  const uint8_t joyCount = joyCount1 + joyCount2;// + joyCount3 + joyCount4;// + joyCount5 + joyCount6;
  //Serial.println(joyCount);

  for (uint8_t i = 0; i < joyCount; i++) {
    if (i == totalUsb)
      break;
      
    //Get the data for the specific controller
    const uint8_t inputPort = (i < joyCount1) ? 0 : 1;
    const SnesController& sc = inputPort == 0 ? snes1.getSnesController(i) : snes2.getSnesController(i - joyCount1);
    /*const SnesController& sc = (i < joyCount1) ? snes1.getSnesController(i) :
                          snes2.getSnesController(i - joyCount1);
                         //(i - joyCount1 < joyCount2) ? snes2.getSnesController(i - joyCount1) :
                         //(i - joyCount1 - joyCount2 < joyCount3) ? snes3.getSnesController(i - joyCount1 - joyCount2) :
                         //snes4.getSnesController(i - joyCount1 - joyCount2 - joyCount3);
                         //(i - joyCount1 - joyCount2 - joyCount3 < joyCount4) ? snes4.getSnesController(i - joyCount1 - joyCount2 - joyCount3) :
                         //(i - joyCount1 - joyCount2 - joyCount3 - joyCount4 < joyCount5) ? snes5.getSnesController(i - joyCount1 - joyCount2 - joyCount3 - joyCount4) :
                         //snes6.getSnesController(i - joyCount1 - joyCount2 - joyCount3 - joyCount4 - joyCount5);*/

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
#ifdef SNES_ENABLE_NTTPAD
      uint32_t buttonData = 0;
#else
      uint16_t buttonData = 0;
#endif

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

#ifdef SNES_ENABLE_NTTPAD
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
#endif
      }
      bitWrite(buttonData, 8, sc.digitalPressed(SNES_SELECT));
      bitWrite(buttonData, 9, sc.digitalPressed(SNES_START));

#ifdef SNES_ENABLE_NTTPAD
  ((Joy1_*)usbStick[i])->setButtons(buttonData);
#else
  ((Joy1_*)usbStick[i])->setButtons(buttonData);
#endif
      

      //Get angle from hatTable and pass to joystick class
#ifdef SNES_ENABLE_NTTPAD
  ((Joy1_*)usbStick[i])->setHatSwitch(hatTable[hatData]);
#else
  ((Joy1_*)usbStick[i])->setHatSwitch(hatTable[hatData]);
#endif
      

      usbStick[i]->sendState();
      #ifdef ENABLE_REFLEX_PAD
        //Only used if not in multitap mode
        if (totalUsb == 2 && inputPort < 2) {
          //const uint8_t startCol = inputPort == 0 ? 0 : 11*6;
          for(uint8_t x = 0; x < 12; x++){
            if(padType == SNES_DEVICE_NES && x > 7)
              continue;
            const Pad pad = (padType == SNES_DEVICE_NES && x < 4) ? padSnes[x+12] : padSnes[x]; //NES uses horizontal align
            PrintPadChar(inputPort, padDivision[inputPort].firstCol, pad.col, pad.row, pad.padvalue, sc.digitalPressed((SnesDigital_Enum)pad.padvalue), pad.on, pad.off);
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
  
  return stateChanged; //joyCount != 0;
}
