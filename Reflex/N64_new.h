/*******************************************************************************
 * N64 controllers to USB using an Arduino Leonardo.
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

//N64 pins - Port 1
#define N64_1_DATA 21 // 9 old, 21 new

//N64 pins - Port 2
#define N64_2_DATA 5 // 20 old, 5 new

CN64Controller* n64;//variable to hold current reading port

CN64Controller* n64list[] = {
  new CN64Controller(N64_1_DATA),
  new CN64Controller(N64_2_DATA)
};


#define N64_ANALOG_DEFAULT_CENTER 127U

#ifdef N64_ANALOG_MAX
  #if N64_ANALOG_MAX == 0 //automatic adjustment. value starts at 50
    #define N64_INITIAL_ANALOG_MAX 50
    static int8_t n64_x_min[] = { -N64_INITIAL_ANALOG_MAX, -N64_INITIAL_ANALOG_MAX };
    static int8_t n64_x_max[] = { N64_INITIAL_ANALOG_MAX, N64_INITIAL_ANALOG_MAX };
    static int8_t n64_y_min[] = { -N64_INITIAL_ANALOG_MAX, -N64_INITIAL_ANALOG_MAX };
    static int8_t n64_y_max[] = { N64_INITIAL_ANALOG_MAX, N64_INITIAL_ANALOG_MAX };
  #else //fixed range
    static const int8_t n64_x_min = -N64_ANALOG_MAX;
    static const int8_t n64_x_max = N64_ANALOG_MAX;
    static const int8_t n64_y_min = -N64_ANALOG_MAX;
    static const int8_t n64_y_max = N64_ANALOG_MAX;
  #endif
#endif

#ifdef ENABLE_REFLEX_PAD
enum PadButton {
    BTN_A       = 1 << 15,
    BTN_B       = 1 << 14,
    BTN_Z       = 1 << 13,
    BTN_START   = 1 << 12,
    BTN_UP      = 1 << 11,
    BTN_DOWN    = 1 << 10,
    BTN_LEFT    = 1 << 9,
    BTN_RIGHT   = 1 << 8,
    BTN_LRSTART = 1 << 7, // This is set when L+R+Start are pressed (and BTN_START is not)
    /* Unused   = 1 << 6, */
    BTN_L       = 1 << 5,
    BTN_R       = 1 << 4,
    BTN_C_UP    = 1 << 3,
    BTN_C_DOWN  = 1 << 2,
    BTN_C_LEFT  = 1 << 1,
    BTN_C_RIGHT = 1 << 0
  };
  const Pad padN64[] = {
    { (uint32_t)BTN_UP,      1, 1*6, UP_ON, UP_OFF },
    { (uint32_t)BTN_DOWN,    3, 1*6, DOWN_ON, DOWN_OFF },
    { (uint32_t)BTN_LEFT,    2, 0,   LEFT_ON, LEFT_OFF },
    { (uint32_t)BTN_RIGHT,   2, 2*6, RIGHT_ON, RIGHT_OFF },
    { (uint32_t)BTN_START,   2, 4*6, FACEBTN_ON, FACEBTN_OFF },
    { (uint32_t)BTN_Z,       3, 4*6, SHOULDERBTN_ON, SHOULDERBTN_OFF },
    { (uint32_t)BTN_B,       2, 6*6, FACEBTN_ON, FACEBTN_OFF }, //3, 9*6
    { (uint32_t)BTN_A,       3, 7*6, FACEBTN_ON, FACEBTN_OFF }, //3, 8*6
    { (uint32_t)BTN_C_UP,    1, 8*6, UP_ON, UP_OFF },
    { (uint32_t)BTN_C_DOWN,  3, 8*6, DOWN_ON, DOWN_OFF },
    { (uint32_t)BTN_C_LEFT,  2, 7*6,   LEFT_ON, LEFT_OFF },
    { (uint32_t)BTN_C_RIGHT, 2, 9*6, RIGHT_ON, RIGHT_OFF },
    /*{ (uint32_t)N64Pad::BTN_C_UP,         2, 9*6, FACEBTN_ON, FACEBTN_OFF },
    { (uint32_t)N64Pad::BTN_C_DOWN,         2, 8*6, FACEBTN_ON, FACEBTN_OFF },
    { (uint32_t)N64Pad::BTN_C_LEFT,         2, 7*6, FACEBTN_ON, FACEBTN_OFF },
    { (uint32_t)N64Pad::BTN_C_RIGHT,         2, 7*6, FACEBTN_ON, FACEBTN_OFF },*/
    { (uint32_t)BTN_R,         0, 8*6, SHOULDERBTN_ON, SHOULDERBTN_OFF },
    { (uint32_t)BTN_L,         0, 1*6, SHOULDERBTN_ON, SHOULDERBTN_OFF }
  };

  void showDefaultPadN64(const uint8_t index, const bool haveController) {
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

void n64ResetAnalogMinMax(const uint8_t index) {
  #ifdef N64_ANALOG_MAX
    #if N64_ANALOG_MAX == 0
      if(index < 2) {
        n64_x_min[index] = -N64_INITIAL_ANALOG_MAX;
        n64_x_max[index] = N64_INITIAL_ANALOG_MAX;
        n64_y_min[index] = -N64_INITIAL_ANALOG_MAX;
        n64_y_max[index] = N64_INITIAL_ANALOG_MAX;
      }
    #endif
  #endif
}

void n64ResetJoyValues(const uint8_t i) {
  if (i >= totalUsb)
    return;
  usbStick[i]->resetState();
  ((Joy1_*)usbStick[i])->setAnalog0(N64_ANALOG_DEFAULT_CENTER); //x
  ((Joy1_*)usbStick[i])->setAnalog1(N64_ANALOG_DEFAULT_CENTER); //y
  ((Joy1_*)usbStick[i])->setAnalog2(N64_ANALOG_DEFAULT_CENTER); //rx
  ((Joy1_*)usbStick[i])->setAnalog3(N64_ANALOG_DEFAULT_CENTER); //ry
  n64ResetAnalogMinMax(i);
}

void n64Setup() {
  //Init onboard led pin
  //pinMode(LED_BUILTIN, OUTPUT);
  
  delay(5);

  totalUsb = 2;
  sleepTime = 50;
  
  //Create usb controllers
  for (uint8_t i = 0; i < totalUsb; i++) {
    usbStick[i] = new Joy1_("RZordN64", JOYSTICK_DEFAULT_REPORT_ID + i, JOYSTICK_TYPE_GAMEPAD, totalUsb,
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
    n64ResetJoyValues(i);
    usbStick[i]->sendState();
  }
  
  delay(50);
  
  dstart(115200);
  debugln (F("Powered on!"));
}

inline bool __attribute__((always_inline))
n64Loop() {
  static uint8_t lastControllerCount = 0;
  static bool haveController[] = { false, false };
  static bool isEnabled[] = { false, false };
  static uint16_t oldButtons[] = { 0, 0 };
  static int8_t oldX[] = { 0, 0 };
  static int8_t oldY[] = { 0, 0 };

  #ifdef ENABLE_REFLEX_PAD
    //static bool firstTime = true;
    //if(firstTime) {
    //  firstTime = false;
    //  showDefaultPadN64(0, false);
    //}
  #endif

  //uint8_t outputIndex = 0;

    //nothing detected yet
    if (!isEnabled[0] && !isEnabled[1]) {
      for (uint8_t i = 0; i < 2; i++) {
        isEnabled[i] = haveController[i] || (haveController[i] = n64list[i]->read());
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
  N64_Data_t n64data[2];

  //read all ports
  
  for (uint8_t i = 0; i < totalUsb; i++) {
    n64 = n64list[i];
    //isReadSuccess[i] = false;

    if (!isEnabled[i])
      continue;
      
    if (n64->read ()) {
      n64data[i] = n64->getData ();
      const bool haveControllerNow = n64data[i].status.device != NINTENDO_DEVICE_N64_NONE;
      //controller just connected?
      if(!haveController[i] && haveControllerNow) {
        n64ResetJoyValues(i);
        #ifdef ENABLE_REFLEX_PAD
          showDefaultPadN64(i, true);
        #endif
      }
      haveController[i] = haveControllerNow;
      sleepTime = 50;
    } else {
      //controller just removed?
      if(haveController) {
        n64ResetJoyValues(i);
        #ifdef ENABLE_REFLEX_PAD
          showDefaultPadN64(i, false);
        #endif
      }
      haveController[i] = false;
      sleepTime = 50000;
    }
  }

  for (uint8_t i = 0; i < totalUsb; i++) {
    if (haveController[i]) {
      //controller read sucess
  
      const uint16_t digitalData = (n64data[i].report.buttons0 << 12) + (n64data[i].report.dpad << 8) + (n64data[i].report.buttons1 << 4) + n64data[i].report.cpad;
      const bool buttonsChanged = digitalData != oldButtons;
      const bool analogChanged = n64data[i].report.xAxis != oldX[i] || n64data[i].report.yAxis != oldY[i];
  
      if (buttonsChanged || analogChanged) { //state changed?
        stateChanged[i] = true;
        
        //L+R+START internally resets the analog stick. We also need to reset it's min/max value;
        if (n64data[i].report.low0)
          n64ResetAnalogMinMax(i);
  
        uint16_t buttonData = 0;
        bitWrite(buttonData, 1, n64data[i].report.a);
        bitWrite(buttonData, 0, n64data[i].report.b);
        bitWrite(buttonData, 8, n64data[i].report.z);
        bitWrite(buttonData, 9, n64data[i].report.start);
        bitWrite(buttonData, 4, n64data[i].report.l);
        bitWrite(buttonData, 5, n64data[i].report.r);
        bitWrite(buttonData, 6, n64data[i].report.cup);
        bitWrite(buttonData, 2, n64data[i].report.cdown);
        bitWrite(buttonData, 3, n64data[i].report.cleft);
        bitWrite(buttonData, 7, n64data[i].report.cright);
  
        ((Joy1_*)usbStick[i])->setButtons(buttonData);
  
        uint8_t hatData = 0;
        bitWrite(hatData, 0, n64data[i].report.dup == 0);
        bitWrite(hatData, 1, n64data[i].report.ddown == 0);
        bitWrite(hatData, 2, n64data[i].report.dleft == 0);
        bitWrite(hatData, 3, n64data[i].report.dright == 0);
  
        //Get angle from hatTable and pass to joystick class
        ((Joy1_*)usbStick[i])->setHatSwitch(hatTable[hatData]);
  
        //how to handle analog range?
        #ifdef N64_ANALOG_MAX
          
          #if N64_ANALOG_MAX == 0 //automatic range adjust
            //Update min and max values
            if(n64data[i].report.xAxis < n64_x_min[i]) {
              n64_x_min[i] = n64data[i].report.xAxis;
              n64_x_max[i] = -n64data[i].report.xAxis;//mirror
            } else if(n64data[i].report.xAxis > n64_x_max[i]) {
              n64_x_min[i] = -n64data[i].report.xAxis;//mirror
              n64_x_max[i] = n64data[i].report.xAxis;
            }
            if(n64data[i].report.yAxis < n64_y_min[i]) {
              n64_y_min[i] = n64data[i].report.yAxis;
              n64_y_max[i] = -n64data[i].report.yAxis;//mirror
            } else if(n64data[i].report.yAxis > n64_y_max[i]) {
              n64_y_min[i] = -n64data[i].report.yAxis; //mirror
              n64_y_max[i] = n64data[i].report.yAxis;
            }
            
            ((Joy1_*)usbStick[i])->setAnalog0(map(n64data[i].report.xAxis, n64_x_min[i], n64_x_max[i], 0, 255)); //x
            ((Joy1_*)usbStick[i])->setAnalog1(map(n64data[i].report.yAxis, n64_y_max[i], n64_y_min[i], 0, 255)); //y
          #else //map to fixed range
            //limit it's range
            if(n64data[i].report.xAxis < -N64_ANALOG_MAX)
              n64data[i].report.xAxis = -N64_ANALOG_MAX;
            else if(n64data[i].report.xAxis > N64_ANALOG_MAX)
              n64data[i].report.xAxis = N64_ANALOG_MAX;
            if(n64data[i].report.yAxis < -N64_ANALOG_MAX)
              n64data[i].report.yAxis = -N64_ANALOG_MAX;
            else if(n64data[i].report.yAxis > N64_ANALOG_MAX)
              n64data[i].report.yAxis = N64_ANALOG_MAX;
              
            ((Joy1_*)usbStick[i])->setAnalog0(map(n64data[i].report.xAxis, n64_x_min, n64_x_max, 0, 255)); //x
            ((Joy1_*)usbStick[i])->setAnalog1(map(n64data[i].report.yAxis, n64_y_max, n64_y_min, 0, 255)); //y
          #endif
  
          //use autmatic values from above, or fixed value
          //((Joy1_*)usbStick[i])->setAnalog0(map(n64data[i].report.xAxis, n64_x_min, n64_x_max, 0, 255)); //x
          //((Joy1_*)usbStick[i])->setAnalog1(map(n64data[i].report.yAxis, n64_y_max, n64_y_min, 0, 255)); //y
          
          //#else //map to fixed range
          //  ((Joy1_*)usbStick[0])->setAnalog0(map(n64->x, -N64_ANALOG_MAX, N64_ANALOG_MAX, 0, 255)); //x
          //  ((Joy1_*)usbStick[0])->setAnalog1(map(n64->y, N64_ANALOG_MAX, -N64_ANALOG_MAX, 0, 255)); //y
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
      }//end if statechanged
    }//end havecontroller
  }//end for


  return stateChanged[0] || stateChanged[1]; //joyCount != 0;
}
