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

CGamecubeController* cube;//variable to hold current reading port

CGamecubeController* gclist[] = {
  new CGamecubeController(GC_1_DATA),
  new CGamecubeController(GC_2_DATA)
};

#define GC_ANALOG_DEFAULT_CENTER 127U

#ifdef GC_ANALOG_MAX
  #if GC_ANALOG_MAX == 0 //automatic adjustment. value starts at 50
    #define GC_INITIAL_ANALOG_MAX 50
    static int8_t gc_x_min[] = { -GC_INITIAL_ANALOG_MAX, -GC_INITIAL_ANALOG_MAX };
    static int8_t gc_x_max[] = { GC_INITIAL_ANALOG_MAX, GC_INITIAL_ANALOG_MAX };
    static int8_t gc_y_min[] = { -GC_INITIAL_ANALOG_MAX, -GC_INITIAL_ANALOG_MAX };
    static int8_t gc_y_max[] = { GC_INITIAL_ANALOG_MAX, GC_INITIAL_ANALOG_MAX };
  #else //fixed range
    static const int8_t gc_x_min = -GC_ANALOG_MAX;
    static const int8_t gc_x_max = GC_ANALOG_MAX;
    static const int8_t gc_y_min = -GC_ANALOG_MAX;
    static const int8_t gc_y_max = GC_ANALOG_MAX;
  #endif
#endif

#ifdef ENABLE_REFLEX_PAD
enum PadButton {
    BTN_START   = 1 << 12,
    BTN_X       = 1 << 11,
    BTN_Y       = 1 << 10,    
    BTN_B       = 1 << 9,
    BTN_A       = 1 << 8,
    /* Unused   = 1 << 7, */
    BTN_L       = 1 << 6,
    BTN_R       = 1 << 5,
    BTN_Z       = 1 << 4,
    BTN_UP      = 1 << 3,
    BTN_DOWN    = 1 << 2,
    BTN_RIGHT   = 1 << 1,
    BTN_LEFT    = 1 << 0
  };
  const Pad padN64[] = {
    { (uint32_t)BTN_UP,      1, 1*6, UP_ON, UP_OFF },
    { (uint32_t)BTN_DOWN,    3, 1*6, DOWN_ON, DOWN_OFF },
    { (uint32_t)BTN_LEFT,    2, 0,   LEFT_ON, LEFT_OFF },
    { (uint32_t)BTN_RIGHT,   2, 2*6, RIGHT_ON, RIGHT_OFF },
    { (uint32_t)BTN_START,   2, 4*6, FACEBTN_ON, FACEBTN_OFF },
    { (uint32_t)BTN_Z,       3, 4*6, SHOULDERBTN_ON, SHOULDERBTN_OFF },
    //{ (uint32_t)BTN_B,       2, 6*6, FACEBTN_ON, FACEBTN_OFF }, //3, 9*6
    //{ (uint32_t)BTN_A,       3, 7*6, FACEBTN_ON, FACEBTN_OFF }, //3, 8*6
    { (uint32_t)BTN_X,       1, 8*6, UP_ON, UP_OFF },
    { (uint32_t)BTN_B,       3, 8*6, DOWN_ON, DOWN_OFF },
    { (uint32_t)BTN_Y,       2, 7*6,   LEFT_ON, LEFT_OFF },
    { (uint32_t)BTN_A,       2, 9*6, RIGHT_ON, RIGHT_OFF },
    { (uint32_t)BTN_R,         0, 8*6, SHOULDERBTN_ON, SHOULDERBTN_OFF },
    { (uint32_t)BTN_L,         0, 1*6, SHOULDERBTN_ON, SHOULDERBTN_OFF }
  };

  void showDefaultPadGameCube(const uint8_t index, const bool haveController) {
    //print default joystick state to oled screen
  
    //const uint8_t firstCol = index == 0 ? 0 : 11*6;
    //const uint8_t lastCol = index == 0 ? 11*6 : 127;
  
    display.clear(padDivision[index].firstCol, padDivision[index].lastCol, oledDisplayFirstRow + 1, 7);
    display.setCursor(padDivision[index].firstCol, 7);
    
    switch(haveController) {
      case true:
        display.print(F("PAD"));
        break;
      default:
        display.print(F("NONE"));
        return;
    }
  
    if (index < 2) {
      //const uint8_t startCol = index == 0 ? 0 : 11*6;
      for(uint8_t x = 0; x < 14; x++){
        const Pad pad = padN64[x];
        PrintPadChar(index, padDivision[index].firstCol, pad.col, pad.row, pad.padvalue, true, pad.on, pad.off, true);
      }
    }
  }
#endif

void gameCubeResetAnalogMinMax(const uint8_t index) {
  #ifdef GC_ANALOG_MAX
    #if GC_ANALOG_MAX == 0
      if(index < 2) {
        gc_x_min[index] = -GC_INITIAL_ANALOG_MAX;
        gc_x_max[index] = GC_INITIAL_ANALOG_MAX;
        gc_y_min[index] = -GC_INITIAL_ANALOG_MAX;
        gc_y_max[index] = GC_INITIAL_ANALOG_MAX;
      }
    #endif
  #endif
}

void gameCubeResetJoyValues(const uint8_t i) {
  if (i >= totalUsb)
    return;
  usbStick[i]->resetState();
  ((Joy1_*)usbStick[i])->setAnalog0(GC_ANALOG_DEFAULT_CENTER); //x
  ((Joy1_*)usbStick[i])->setAnalog1(GC_ANALOG_DEFAULT_CENTER); //y
  ((Joy1_*)usbStick[i])->setAnalog2(GC_ANALOG_DEFAULT_CENTER); //rx
  ((Joy1_*)usbStick[i])->setAnalog3(GC_ANALOG_DEFAULT_CENTER); //ry
  gameCubeResetAnalogMinMax(i);
}

void gameCubeSetup() {
  //Init onboard led pin
  //pinMode(LED_BUILTIN, OUTPUT);
  
  delay(5);

  totalUsb = 2;
  sleepTime = 50;
  
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
    //static bool firstTime = true;
    //if(firstTime) {
    //  firstTime = false;
    //  showDefaultPadGameCube(0, false);
    //}
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
          display.print(F("NONE"));  
        else if (!isEnabled[i])
          display.print(F("N/A"));
      }
      #endif
    }


  bool stateChanged[] = { false, false };
  Gamecube_Data_t n64data[2];

  //read all ports
  
  for (uint8_t i = 0; i < totalUsb; i++) {
    cube = gclist[i];
    //isReadSuccess[i] = false;

    if (!isEnabled[i])
      continue;
      
    if (cube->read ()) {
      n64data[i] = cube->getData ();
      const bool haveControllerNow = n64data[i].status.device != NINTENDO_DEVICE_GC_NONE;
      //controller just connected?
      if(!haveController[i] && haveControllerNow) {
        gameCubeResetJoyValues(i);
        #ifdef ENABLE_REFLEX_PAD
          showDefaultPadGameCube(i, true);
        #endif
      }
      haveController[i] = haveControllerNow;
      sleepTime = 50;
    } else {
      //controller just removed?
      if(haveController) {
        gameCubeResetJoyValues(i);
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
  
      const uint16_t digitalData = (n64data[i].report.buttons0 << 12) + (n64data[i].report.dpad << 8) + (n64data[i].report.buttons1 << 4);
      const bool buttonsChanged = digitalData != oldButtons;
      const bool analogChanged = n64data[i].report.xAxis != oldX[i] || n64data[i].report.yAxis != oldY[i] || n64data[i].report.cxAxis != oldCX[i] || n64data[i].report.cyAxis != oldCY[i];
  
      if (buttonsChanged || analogChanged) { //state changed?
        stateChanged[i] = true;
        
        //L+R+START internally resets the analog stick. We also need to reset it's min/max value;
        //if (n64data[i].report.low0)
        //  gameCubeResetAnalogMinMax(i);
  
        uint16_t buttonData = 0;
        bitWrite(buttonData, 2, n64data[i].report.a);
        bitWrite(buttonData, 1, n64data[i].report.b);
        bitWrite(buttonData, 3, n64data[i].report.x);
        bitWrite(buttonData, 0, n64data[i].report.y);
        bitWrite(buttonData, 8, n64data[i].report.z);
        bitWrite(buttonData, 9, n64data[i].report.start);
        bitWrite(buttonData, 4, n64data[i].report.l);
        bitWrite(buttonData, 5, n64data[i].report.r);
        //bitWrite(buttonData, 6, n64data[i].report.cup);
        //bitWrite(buttonData, 2, n64data[i].report.cdown);
        //bitWrite(buttonData, 3, n64data[i].report.cleft);
        //bitWrite(buttonData, 7, n64data[i].report.cright);

        //bitWrite(buttonData, 6, n64data[i].report.cup); //LT?
        //bitWrite(buttonData, 7, n64data[i].report.cright); //RT?
  
        ((Joy1_*)usbStick[i])->setButtons(buttonData);
  
        uint8_t hatData = 0;
        bitWrite(hatData, 0, n64data[i].report.dup == 0);
        bitWrite(hatData, 1, n64data[i].report.ddown == 0);
        bitWrite(hatData, 2, n64data[i].report.dleft == 0);
        bitWrite(hatData, 3, n64data[i].report.dright == 0);
  
        //Get angle from hatTable and pass to joystick class
        ((Joy1_*)usbStick[i])->setHatSwitch(hatTable[hatData]);
  
        //how to handle analog range?
        #ifdef GC_ANALOG_MAX
          
          #if GC_ANALOG_MAX == 0 //automatic range adjust
            //Update min and max values
            if(n64data[i].report.xAxis < gc_x_min[i]) {
              gc_x_min[i] = n64data[i].report.xAxis;
              gc_x_max[i] = -n64data[i].report.xAxis;//mirror
            } else if(n64data[i].report.xAxis > gc_x_max[i]) {
              gc_x_min[i] = -n64data[i].report.xAxis;//mirror
              gc_x_max[i] = n64data[i].report.xAxis;
            }
            if(n64data[i].report.yAxis < gc_y_min[i]) {
              gc_y_min[i] = n64data[i].report.yAxis;
              gc_y_max[i] = -n64data[i].report.yAxis;//mirror
            } else if(n64data[i].report.yAxis > gc_y_max[i]) {
              gc_y_min[i] = -n64data[i].report.yAxis; //mirror
              gc_y_max[i] = n64data[i].report.yAxis;
            }
            
            ((Joy1_*)usbStick[i])->setAnalog0(map(n64data[i].report.xAxis, gc_x_min[i], gc_x_max[i], 0, 255)); //x
            ((Joy1_*)usbStick[i])->setAnalog1(map(n64data[i].report.yAxis, gc_y_max[i], gc_y_min[i], 0, 255)); //y
          #else //map to fixed range
            //limit it's range
            if(n64data[i].report.xAxis < -GC_ANALOG_MAX)
              n64data[i].report.xAxis = -GC_ANALOG_MAX;
            else if(n64data[i].report.xAxis > GC_ANALOG_MAX)
              n64data[i].report.xAxis = GC_ANALOG_MAX;
            if(n64data[i].report.yAxis < -GC_ANALOG_MAX)
              n64data[i].report.yAxis = -GC_ANALOG_MAX;
            else if(n64data[i].report.yAxis > GC_ANALOG_MAX)
              n64data[i].report.yAxis = GC_ANALOG_MAX;
              
            ((Joy1_*)usbStick[i])->setAnalog0(map(n64data[i].report.xAxis, gc_x_min, gc_x_max, 0, 255)); //x
            ((Joy1_*)usbStick[i])->setAnalog1(map(n64data[i].report.yAxis, gc_y_max, gc_y_min, 0, 255)); //y
          #endif
  
          //use autmatic values from above, or fixed value
          //((Joy1_*)usbStick[i])->setAnalog0(map(n64data[i].report.xAxis, gc_x_min, gc_x_max, 0, 255)); //x
          //((Joy1_*)usbStick[i])->setAnalog1(map(n64data[i].report.yAxis, gc_y_max, gc_y_min, 0, 255)); //y
          
          //#else //map to fixed range
          //  ((Joy1_*)usbStick[0])->setAnalog0(map(n64->x, -GC_ANALOG_MAX, GC_ANALOG_MAX, 0, 255)); //x
          //  ((Joy1_*)usbStick[0])->setAnalog1(map(n64->y, GC_ANALOG_MAX, -GC_ANALOG_MAX, 0, 255)); //y
          //#endif
          
        #else //use raw value
          ((Joy1_*)usbStick[i])->setAnalog0( (uint8_t)(n64data[i].report.xAxis + 128U) ); //x
          ((Joy1_*)usbStick[i])->setAnalog1( (uint8_t)~(n64data[i].report.yAxis + 128U) ); //y
        #endif
  
        usbStick[i]->sendState();
  
        #ifdef ENABLE_REFLEX_PAD
          //const uint8_t startCol = inputPort == 0 ? 0 : 11*6;
          for(uint8_t x = 0; x < 14; x++){
            const Pad pad = padN64[x];
            //PrintPadChar(0, padDivision[0].firstCol, pad.col, pad.row, pad.padvalue, n64data.report & pad.padvalue, pad.on, pad.off);
            PrintPadChar(i, padDivision[i].firstCol, pad.col, pad.row, pad.padvalue, digitalData & pad.padvalue, pad.on, pad.off);
          }
        #endif
  
        //keep values
        oldButtons[i] = digitalData;
        oldX[i] = n64data[i].report.xAxis;
        oldY[i] = n64data[i].report.yAxis;
        oldCX[i] = n64data[i].report.cxAxis;
        oldCY[i] = n64data[i].report.cyAxis;
      }//end if statechanged
    }//end havecontroller
  }//end for


  return stateChanged[0] || stateChanged[1]; //joyCount != 0;
}
