/*******************************************************************************
 * GameCube controllers to USB using an Arduino Leonardo.
 *
 * Works with analog pad.
 * 
 * Uses Nintendo Lib
 * https://github.com/NicoHood/Nintendo
 *
 * For details on Joystick Library, see
 * https://github.com/MHeironimus/ArduinoJoystickLibrary
 *
*/

#include "src/NicoHoodNintendo/Nintendo.h"
#include "src/ArduinoJoystickLibrary/Joy1.h"


//GameCube pins - Port 1
#define GC_1_DATA 21 // 9 old, 21 new

//GameCube pins - Port 2
#define GC_2_DATA 5 // 20 old, 5 new

CGamecubeController* gclist[] = {
  new CGamecubeController(GC_1_DATA),
  new CGamecubeController(GC_2_DATA)
};

#define GC_ANALOG_DEFAULT_CENTER 127U

#ifdef GC_ANALOG_MAX //fixed range
  static const uint8_t gc_min = 255-GC_ANALOG_MAX;
  static const uint8_t gc_max = GC_ANALOG_MAX;
#endif

#ifdef ENABLE_REFLEX_PAD
enum GCPadButton {
    GCBTN_START   = 1 << 12,
    GCBTN_X       = 1 << 11,
    GCBTN_Y       = 1 << 10,    
    GCBTN_B       = 1 << 9,
    GCBTN_A       = 1 << 8,
    /* Unused   = 1 << 7, */
    GCBTN_L       = 1 << 6,
    GCBTN_R       = 1 << 5,
    GCBTN_Z       = 1 << 4,
    GCBTN_UP      = 1 << 3,
    GCBTN_DOWN    = 1 << 2,
    GCBTN_RIGHT   = 1 << 1,
    GCBTN_LEFT    = 1 << 0
  };
  const Pad padGC[] = {
    { (uint32_t)GCBTN_UP,    1, 1*6, UP_ON, UP_OFF },
    { (uint32_t)GCBTN_DOWN,  3, 1*6, DOWN_ON, DOWN_OFF },
    { (uint32_t)GCBTN_LEFT,  2, 0,   LEFT_ON, LEFT_OFF },
    { (uint32_t)GCBTN_RIGHT, 2, 2*6, RIGHT_ON, RIGHT_OFF },
    { (uint32_t)GCBTN_START, 2, 4*6, FACEBTN_ON, FACEBTN_OFF },
    { (uint32_t)GCBTN_Z,     0, 7*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
    { (uint32_t)GCBTN_X,     2, 8*6, FACEBTN_ON, FACEBTN_OFF },
    { (uint32_t)GCBTN_A,     3, 8*6, FACEBTN_ON, FACEBTN_OFF },
    { (uint32_t)GCBTN_B,     3, 7*6, FACEBTN_ON, FACEBTN_OFF },
    { (uint32_t)GCBTN_Y,     3, 9*6, FACEBTN_ON, FACEBTN_OFF },
    { (uint32_t)GCBTN_R,     0, 8*6, SHOULDERBTN_ON, SHOULDERBTN_OFF },
    { (uint32_t)GCBTN_L,     0, 1*6, SHOULDERBTN_ON, SHOULDERBTN_OFF }
  };

  void showDefaultPadGameCube(const uint8_t index, const bool haveController) {
    //print default joystick state to oled screen
  
    //const uint8_t firstCol = index == 0 ? 0 : 11*6;
    //const uint8_t lastCol = index == 0 ? 11*6 : 127;
  
    display.clear(padDivision[index].firstCol, padDivision[index].lastCol, oledDisplayFirstRow + 1, 7);
    display.setCursor(padDivision[index].firstCol, 7);
    
    switch(haveController) {
      case true:
        display.print(PSTR_TO_F(PSTR_PAD));
        break;
      default:
        display.print(PSTR_TO_F(PSTR_NONE));
        return;
    }
  
    if (index < 2) {
      //const uint8_t startCol = index == 0 ? 0 : 11*6;
      for(uint8_t x = 0; x < 12; x++){
        const Pad pad = padGC[x];
        PrintPadChar(index, padDivision[index].firstCol, pad.col, pad.row, pad.padvalue, true, pad.on, pad.off, true);
      }
    }
  }
#endif


void gameCubeResetJoyValues(const uint8_t i) {
  if (i >= totalUsb)
    return;
  usbStick[i]->resetState();
  ((Joy1_*)usbStick[i])->setAnalog0(GC_ANALOG_DEFAULT_CENTER); //x
  ((Joy1_*)usbStick[i])->setAnalog1(GC_ANALOG_DEFAULT_CENTER); //y
  ((Joy1_*)usbStick[i])->setAnalog2(GC_ANALOG_DEFAULT_CENTER); //rx
  ((Joy1_*)usbStick[i])->setAnalog3(GC_ANALOG_DEFAULT_CENTER); //ry
}

void gameCubeSetup() {
  //Init onboard led pin
  //pinMode(LED_BUILTIN, OUTPUT);
  
  delay(5);

  totalUsb = 2;
  sleepTime = 100;
  
  //Create usb controllers
  for (uint8_t i = 0; i < totalUsb; i++) {
    usbStick[i] = new Joy1_("RZordGC", JOYSTICK_DEFAULT_REPORT_ID + i, JOYSTICK_TYPE_GAMEPAD, totalUsb,
          true,//includeXAxis,
          true,//includeYAxis,
          true,//includeZAxis,
          true,//includeRzAxis,
          false,//includeThrottle,
          false,//includeBrake,
          false);//includeSteering
  }
  
  //Set usb parameters and reset to default values
  for (uint8_t i = 0; i < totalUsb; i++) {
    gameCubeResetJoyValues(i);
    usbStick[i]->sendState();
  }

  for (uint8_t i = 0; i < totalUsb; i++) {
    gclist[i]->begin();
  }
  
  delay(50);
  
  dstart(115200);
  debugln (F("Powered on!"));
}

inline bool __attribute__((always_inline))
gameCubeLoop() {
  static uint8_t lastControllerCount = 0;
  static bool haveController[] = { false, false };
  static bool isEnabled[] = { false, false };
  static uint16_t oldButtons[] = { 0, 0 };
  static int8_t oldX[] = { 0, 0 };
  static int8_t oldY[] = { 0, 0 };
  static int8_t oldCX[] = { 0, 0 };
  static int8_t oldCY[] = { 0, 0 };

  #ifdef ENABLE_REFLEX_PAD
    static bool firstTime = true;
    if(firstTime) {
      firstTime = false;
      showDefaultPadGameCube(0, false);
    }
  #endif

  //uint8_t outputIndex = 0;

    //nothing detected yet
    if (!isEnabled[0] && !isEnabled[1]) {
      for (uint8_t i = 0; i < 2; i++) {
        isEnabled[i] = haveController[i] || (haveController[i] = gclist[i]->read());
      }
      
      #ifdef ENABLE_REFLEX_PAD

      setOledDisplay(true);

      display.clear(0, 127, 7, 7);
      display.setRow(7);

      for (uint8_t i = 0; i < 2; i++) {
        display.setCol(padDivision[i].firstCol);
        if (!isEnabled[0] && !isEnabled[1])
          display.print(PSTR_TO_F(PSTR_NONE));  
        else if (!isEnabled[i])
          display.print(PSTR_TO_F(PSTR_NA));
      }
      for (uint8_t i = 0; i < 2; i++) {
        if(isEnabled[i])
          showDefaultPadGameCube(i, true);
      }

      #endif
    }


  bool stateChanged[] = { false, false };
  Gamecube_Data_t gcdata[2];

  //read all ports
  
  for (uint8_t i = 0; i < totalUsb; i++) {
    //const CGamecubeController* cube = gclist[i];
    //isReadSuccess[i] = false;

    if (!isEnabled[i])
      continue;

    if (gclist[i]->read ()) {
      gcdata[i] = gclist[i]->getData ();
      const bool haveControllerNow = gcdata[i].status.device != NINTENDO_DEVICE_GC_NONE;

      //controller just connected?
      if(!haveController[i] && haveControllerNow) {
        gameCubeResetJoyValues(i);
        #ifdef ENABLE_REFLEX_PAD
          showDefaultPadGameCube(i, true);
        #endif
      }
      haveController[i] = haveControllerNow;
      sleepTime = 100;
    } else {
      //controller just removed?
      if(haveController) {
        gameCubeResetJoyValues(i);
        usbStick[i]->sendState();
        #ifdef ENABLE_REFLEX_PAD
          showDefaultPadGameCube(i, false);
        #endif
      }
      haveController[i] = false;
      sleepTime = 50000;
    }
  }

  for (uint8_t i = 0; i < totalUsb; i++) {
    if (haveController[i]) {
      //controller read sucess
  
      const uint16_t digitalData = (gcdata[i].report.buttons0 << 8) | (gcdata[i].report.buttons1);
      const bool buttonsChanged = digitalData != oldButtons[i];
      const bool analogChanged = gcdata[i].report.xAxis != oldX[i] || gcdata[i].report.yAxis != oldY[i] || gcdata[i].report.cxAxis != oldCX[i] || gcdata[i].report.cyAxis != oldCY[i];

      if (buttonsChanged || analogChanged) { //state changed?
        stateChanged[i] = true;
  
        uint16_t buttonData = 0;
        bitWrite(buttonData, 2, gcdata[i].report.a);
        bitWrite(buttonData, 1, gcdata[i].report.b);
        bitWrite(buttonData, 3, gcdata[i].report.x);
        bitWrite(buttonData, 0, gcdata[i].report.y);
        bitWrite(buttonData, 8, gcdata[i].report.z);
        bitWrite(buttonData, 9, gcdata[i].report.start);
        bitWrite(buttonData, 4, gcdata[i].report.l);
        bitWrite(buttonData, 5, gcdata[i].report.r);
        //bitWrite(buttonData, 6, gcdata[i].report.cup);
        //bitWrite(buttonData, 2, gcdata[i].report.cdown);
        //bitWrite(buttonData, 3, gcdata[i].report.cleft);
        //bitWrite(buttonData, 7, gcdata[i].report.cright);

        //bitWrite(buttonData, 6, gcdata[i].report.cup); //LT?
        //bitWrite(buttonData, 7, gcdata[i].report.cright); //RT?
  
        ((Joy1_*)usbStick[i])->setButtons(buttonData);
  
        uint8_t hatData = 0;
        bitWrite(hatData, 0, gcdata[i].report.dup == 0);
        bitWrite(hatData, 1, gcdata[i].report.ddown == 0);
        bitWrite(hatData, 2, gcdata[i].report.dleft == 0);
        bitWrite(hatData, 3, gcdata[i].report.dright == 0);
  
        //Get angle from hatTable and pass to joystick class
        ((Joy1_*)usbStick[i])->setHatSwitch(hatTable[hatData]);
  
        //how to handle analog range?
        #ifdef GC_ANALOG_MAX
          //limit it's range
          if(gcdata[i].report.xAxis < gc_min)
            gcdata[i].report.xAxis = gc_min;
          else if(gcdata[i].report.xAxis > gc_max)
            gcdata[i].report.xAxis = gc_max;
          if(gcdata[i].report.yAxis < gc_min)
            gcdata[i].report.yAxis = gc_min;
          else if(gcdata[i].report.yAxis > gc_max)
            gcdata[i].report.yAxis = gc_max;

          if(gcdata[i].report.cxAxis < gc_min)
            gcdata[i].report.cxAxis = gc_min;
          else if(gcdata[i].report.cxAxis > gc_max)
            gcdata[i].report.cxAxis = gc_max;
          if(gcdata[i].report.cyAxis < gc_min)
            gcdata[i].report.cyAxis = gc_min;
          else if(gcdata[i].report.cyAxis > gc_max)
            gcdata[i].report.cyAxis = gc_max;
            
          ((Joy1_*)usbStick[i])->setAnalog0(map(gcdata[i].report.xAxis, gc_min, gc_max, 0, 255)); //x
          ((Joy1_*)usbStick[i])->setAnalog1(map(gcdata[i].report.yAxis, gc_max, gc_min, 0, 255)); //y
          ((Joy1_*)usbStick[i])->setAnalog2(map(gcdata[i].report.cxAxis, gc_min, gc_max, 0, 255)); //cx
          ((Joy1_*)usbStick[i])->setAnalog3(map(gcdata[i].report.cyAxis, gc_max, gc_min, 0, 255)); //cy
          
        #else //use raw value
          ((Joy1_*)usbStick[i])->setAnalog0( (uint8_t)(gcdata[i].report.xAxis) ); //x
          ((Joy1_*)usbStick[i])->setAnalog1( (uint8_t)~(gcdata[i].report.yAxis) ); //y
          ((Joy1_*)usbStick[i])->setAnalog2( (uint8_t)(gcdata[i].report.cxAxis) ); //x
          ((Joy1_*)usbStick[i])->setAnalog3( (uint8_t)~(gcdata[i].report.cyAxis) ); //y
        #endif
  
        usbStick[i]->sendState();
  
        #ifdef ENABLE_REFLEX_PAD
          //const uint8_t startCol = inputPort == 0 ? 0 : 11*6;
          for(uint8_t x = 0; x < 12; x++){
            const Pad pad = padGC[x];
            PrintPadChar(i, padDivision[i].firstCol, pad.col, pad.row, pad.padvalue, digitalData & pad.padvalue, pad.on, pad.off);
          }
        #endif
  
        //keep values
        oldButtons[i] = digitalData;
        oldX[i] = gcdata[i].report.xAxis;
        oldY[i] = gcdata[i].report.yAxis;
        oldCX[i] = gcdata[i].report.cxAxis;
        oldCY[i] = gcdata[i].report.cyAxis;
      }//end if statechanged
    }//end havecontroller
  }//end for


  return stateChanged[0] || stateChanged[1]; //joyCount != 0;
}
