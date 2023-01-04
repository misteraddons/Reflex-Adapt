/*******************************************************************************
 * N64 controllers to USB using an Arduino Leonardo.
 *
 * Works with analog pad.
 * 
 * Uses N64PadForArduino Lib
 * https://github.com/SukkoPera/N64PadForArduino
 *
 * For details on Joystick Library, see
 * https://github.com/MHeironimus/ArduinoJoystickLibrary
 *
*/

#include "src/N64PadForArduino/N64Pad.h"
#include "src/ArduinoJoystickLibrary/Joy1.h"

//N64 pins - Port 1
//Pin defined in N64PadForArduino/protocol/pinconfig.h
//Currenty using D9/A9/PB5

N64Pad* n64;

#define N64_ANALOG_DEFAULT_CENTER 127U

#ifdef N64_ANALOG_MAX
  #if N64_ANALOG_MAX == 0 //automatic adjustment. value starts at 50
    #define N64_INITIAL_ANALOG_MAX 50
    static int8_t n64_x_min = -N64_INITIAL_ANALOG_MAX;
    static int8_t n64_x_max = N64_INITIAL_ANALOG_MAX;
    static int8_t n64_y_min = -N64_INITIAL_ANALOG_MAX;
    static int8_t n64_y_max = N64_INITIAL_ANALOG_MAX;
  #else //fixed range
    static const int8_t n64_x_min = -N64_ANALOG_MAX;
    static const int8_t n64_x_max = N64_ANALOG_MAX;
    static const int8_t n64_y_min = -N64_ANALOG_MAX;
    static const int8_t n64_y_max = N64_ANALOG_MAX;
  #endif
#endif

#ifdef ENABLE_REFLEX_PAD
  const Pad padN64[] = {
    { (uint32_t)N64Pad::BTN_UP,      1, 1*6, UP_ON, UP_OFF },
    { (uint32_t)N64Pad::BTN_DOWN,    3, 1*6, DOWN_ON, DOWN_OFF },
    { (uint32_t)N64Pad::BTN_LEFT,    2, 0,   LEFT_ON, LEFT_OFF },
    { (uint32_t)N64Pad::BTN_RIGHT,   2, 2*6, RIGHT_ON, RIGHT_OFF },
    { (uint32_t)N64Pad::BTN_START,   2, 4*6, FACEBTN_ON, FACEBTN_OFF },
    { (uint32_t)N64Pad::BTN_Z,       3, 4*6, SHOULDERBTN_ON, SHOULDERBTN_OFF },
    { (uint32_t)N64Pad::BTN_B,       2, 6*6, FACEBTN_ON, FACEBTN_OFF }, //3, 9*6
    { (uint32_t)N64Pad::BTN_A,       3, 7*6, FACEBTN_ON, FACEBTN_OFF }, //3, 8*6
    { (uint32_t)N64Pad::BTN_C_UP,    1, 9*6, UP_ON, UP_OFF },
    { (uint32_t)N64Pad::BTN_C_DOWN,  3, 9*6, DOWN_ON, DOWN_OFF },
    { (uint32_t)N64Pad::BTN_C_LEFT,  2, 8*6,   LEFT_ON, LEFT_OFF },
    { (uint32_t)N64Pad::BTN_C_RIGHT, 2, 10*6, RIGHT_ON, RIGHT_OFF },
    /*{ (uint32_t)N64Pad::BTN_C_UP,         2, 9*6, FACEBTN_ON, FACEBTN_OFF },
    { (uint32_t)N64Pad::BTN_C_DOWN,         2, 8*6, FACEBTN_ON, FACEBTN_OFF },
    { (uint32_t)N64Pad::BTN_C_LEFT,         2, 7*6, FACEBTN_ON, FACEBTN_OFF },
    { (uint32_t)N64Pad::BTN_C_RIGHT,         2, 7*6, FACEBTN_ON, FACEBTN_OFF },*/
    { (uint32_t)N64Pad::BTN_R,         0, 9*6, SHOULDERBTN_ON, SHOULDERBTN_OFF },
    { (uint32_t)N64Pad::BTN_L,         0, 1*6, SHOULDERBTN_ON, SHOULDERBTN_OFF }
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

void n64ResetAnalogMinMax() {
  #ifdef N64_ANALOG_MAX
    #if N64_ANALOG_MAX == 0
      n64_x_min = -N64_INITIAL_ANALOG_MAX;
      n64_x_max = N64_INITIAL_ANALOG_MAX;
      n64_y_min = -N64_INITIAL_ANALOG_MAX;
      n64_y_max = N64_INITIAL_ANALOG_MAX;
    #endif
  #endif
}

void n64ResetJoyValues(const uint8_t i) {
  if (i >= totalUsb)
    return;
  usbStick[i]->resetState();
  ((Joy1_*)usbStick[0])->setAnalog0(N64_ANALOG_DEFAULT_CENTER); //x
  ((Joy1_*)usbStick[0])->setAnalog1(N64_ANALOG_DEFAULT_CENTER); //y
  ((Joy1_*)usbStick[0])->setAnalog2(N64_ANALOG_DEFAULT_CENTER); //rx
  ((Joy1_*)usbStick[0])->setAnalog3(N64_ANALOG_DEFAULT_CENTER); //ry
  n64ResetAnalogMinMax();
}

void n64Setup() {
  //Init onboard led pin
  //pinMode(LED_BUILTIN, OUTPUT);
  
  delay(5);

  totalUsb = 1;
  sleepTime = 50;
  
  //Create usb controllers
  for (uint8_t i = 0; i < totalUsb; i++) {
    usbStick[i] = new Joy1_("RZordN64", JOYSTICK_DEFAULT_REPORT_ID + i, JOYSTICK_TYPE_GAMEPAD, totalUsb,
          true,//includeXAxis,
          true,//includeYAxis,
          false,//includeZAxis,
          true,//includeRxAxis,
          true,//includeRyAxis,
          false,//includeThrottle,
          false,//includeBrake,
          false);//includeSteering
  }
  
  //Set usb parameters and reset to default values
  for (uint8_t i = 0; i < totalUsb; i++) {
    n64ResetJoyValues(i);
    usbStick[i]->sendState();
  }

  n64 = new N64Pad();
  
  delay(50);
  
  dstart(115200);
  debugln (F("Powered on!"));
}

inline bool __attribute__((always_inline))
n64Loop() {
  static uint8_t lastControllerCount = 0;

  #ifdef ENABLE_REFLEX_PAD
    static bool firstTime = true;
    if(firstTime) {
      firstTime = false;
      showDefaultPadN64(0, false);
    }
  #endif

  static boolean haveController = false;
  static uint16_t oldButtons = 0;
  static int8_t oldX = 0, oldY = 0;

  bool stateChanged = false;


  if (!haveController) {
    if (n64->begin ()) {
      //controller connected
      haveController = true;
      n64ResetJoyValues(0);
      #ifdef ENABLE_REFLEX_PAD
        showDefaultPadN64(0, haveController);
      #endif
      sleepTime = 50;
    } else {
      sleepTime = 50000;
    }
  } else {
    if (!n64->read ()) {
      //controller removed
      haveController = false;
      sleepTime = 50000;
      n64ResetJoyValues(0);
      #ifdef ENABLE_REFLEX_PAD
        showDefaultPadN64(0, haveController);
      #endif
    } else {
      //controller read sucess

      const bool buttonsChanged = n64->buttons != oldButtons;
      const bool analogChanged = n64->x != oldX || n64->y != oldY;
      
      if (buttonsChanged || analogChanged) { //state changed?
        stateChanged = true;
        
        //L+R+START internally resets the analog stick. We also need to reset it's min/max value;
        if ((n64->buttons & N64Pad::BTN_LRSTART) != 0)
          n64ResetAnalogMinMax();

        uint16_t buttonData = 0;
        bitWrite(buttonData, 1, n64->buttons & N64Pad::BTN_A);
        bitWrite(buttonData, 0, n64->buttons & N64Pad::BTN_B);
        bitWrite(buttonData, 8, n64->buttons & N64Pad::BTN_Z);
        bitWrite(buttonData, 9, n64->buttons & N64Pad::BTN_START);
        bitWrite(buttonData, 4, n64->buttons & N64Pad::BTN_L);
        bitWrite(buttonData, 5, n64->buttons & N64Pad::BTN_R);
        bitWrite(buttonData, 6, n64->buttons & N64Pad::BTN_C_UP);
        bitWrite(buttonData, 2, n64->buttons & N64Pad::BTN_C_DOWN);
        bitWrite(buttonData, 3, n64->buttons & N64Pad::BTN_C_LEFT);
        bitWrite(buttonData, 7, n64->buttons & N64Pad::BTN_C_RIGHT);

        ((Joy1_*)usbStick[0])->setButtons(buttonData);

        uint8_t hatData = 0;
        bitWrite(hatData, 0, (n64->buttons & N64Pad::BTN_UP) == 0);
        bitWrite(hatData, 1, (n64->buttons & N64Pad::BTN_DOWN) == 0);
        bitWrite(hatData, 2, (n64->buttons & N64Pad::BTN_LEFT) == 0);
        bitWrite(hatData, 3, (n64->buttons & N64Pad::BTN_RIGHT) == 0);

        //Get angle from hatTable and pass to joystick class
        ((Joy1_*)usbStick[0])->setHatSwitch(hatTable[hatData]);

        //how to handle analog range?
        #ifdef N64_ANALOG_MAX
          
          #if N64_ANALOG_MAX == 0 //automatic range adjust
            //Update min and max values
            if(n64->x < n64_x_min) {
              n64_x_min = n64->x;
              n64_x_max = -n64->x;//mirror
            } else if(n64->x > n64_x_max) {
              n64_x_min = -n64->x;//mirror
              n64_x_max = n64->x;
            }
            if(n64->y < n64_y_min) {
              n64_y_min = n64->y;
              n64_y_max = -n64->y;//mirror
            } else if(n64->y > n64_y_max) {
              n64_y_min = -n64->y; //mirror
              n64_y_max = n64->y;
            }
          #else //map to fixed range
            //limit it's range
            if(n64->x < -N64_ANALOG_MAX)
              n64->x = -N64_ANALOG_MAX;
            else if(n64->x > N64_ANALOG_MAX)
              n64->x = N64_ANALOG_MAX;
            if(n64->y < -N64_ANALOG_MAX)
              n64->y = -N64_ANALOG_MAX;
            else if(n64->y > N64_ANALOG_MAX)
              n64->y = N64_ANALOG_MAX;
          #endif

          //use autmatic values from above, or fixed value
          ((Joy1_*)usbStick[0])->setAnalog0(map(n64->x, n64_x_min, n64_x_max, 0, 255)); //x
          ((Joy1_*)usbStick[0])->setAnalog1(map(n64->y, n64_y_max, n64_y_min, 0, 255)); //y
          
          //#else //map to fixed range
          //  ((Joy1_*)usbStick[0])->setAnalog0(map(n64->x, -N64_ANALOG_MAX, N64_ANALOG_MAX, 0, 255)); //x
          //  ((Joy1_*)usbStick[0])->setAnalog1(map(n64->y, N64_ANALOG_MAX, -N64_ANALOG_MAX, 0, 255)); //y
          //#endif
          
        #else //use raw value
          ((Joy1_*)usbStick[0])->setAnalog0( (uint8_t)(n64->x + 128U) ); //x
          ((Joy1_*)usbStick[0])->setAnalog1( (uint8_t)~(n64->y + 128U) ); //y          
        #endif

        usbStick[0]->sendState();

        #ifdef ENABLE_REFLEX_PAD
          //const uint8_t startCol = inputPort == 0 ? 0 : 11*6;
          for(uint8_t x = 0; x < 14; x++){
            const Pad pad = padN64[x];
            PrintPadChar(0, padDivision[0].firstCol, pad.col, pad.row, pad.padvalue, n64->buttons & pad.padvalue, pad.on, pad.off);
          }
        #endif

        //keep values
        oldButtons = n64->buttons;
        oldX = n64->x;
        oldY = n64->y;
      }//end if statechanged

    }//end else pad read
  }//end havecontroller


  return stateChanged; //joyCount != 0;
}
