/*******************************************************************************
 * GameCube controllers to USB using an Arduino Leonardo.
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

#include "src/N64PadForArduino/GCPad.h"
#include "src/ArduinoJoystickLibrary/Joy1.h"

//GameCube pins - Port 1
//Pin defined in N64PadForArduino/protocol/pinconfig.h
//Currenty using D9/A9/PB5

GCPad* cube;

#define GAMECUBE_ANALOG_DEFAULT_CENTER 127U

static int8_t gc_x_min = 0;
static int8_t gc_x_max = 0;
static int8_t gc_y_min = 0;
static int8_t gc_y_max = 0;

#ifdef ENABLE_REFLEX_PAD
  const Pad padGameCube[] = {
    { GCPad::BTN_D_UP,    1, 1*6, UP_ON, UP_OFF },
    { GCPad::BTN_D_DOWN,  3, 1*6, DOWN_ON, DOWN_OFF },
    { GCPad::BTN_D_LEFT,  2, 0,   LEFT_ON, LEFT_OFF },
    { GCPad::BTN_D_RIGHT, 2, 2*6, RIGHT_ON, RIGHT_OFF },
    { GCPad::BTN_START,   2, 4*6, FACEBTN_ON, FACEBTN_OFF },
    { GCPad::BTN_Z,       3, 4*6, SHOULDERBTN_ON, SHOULDERBTN_OFF },
    { GCPad::BTN_B,       2, 6*6, FACEBTN_ON, FACEBTN_OFF }, //3, 9*6
    { GCPad::BTN_A,       3, 7*6, FACEBTN_ON, FACEBTN_OFF }, //3, 8*6
    { GCPad::BTN_X,       1, 9*6, UP_ON, UP_OFF },
    { GCPad::BTN_Y,       3, 9*6, DOWN_ON, DOWN_OFF },
    //{ GCPad::BTN_C_LEFT,  2, 8*6,   LEFT_ON, LEFT_OFF },
    //{ GCPad::BTN_C_RIGHT, 2, 10*6, RIGHT_ON, RIGHT_OFF },
    /*{ GCPad::BTN_C_UP,         2, 9*6, FACEBTN_ON, FACEBTN_OFF },
    { GCPad::BTN_C_DOWN,         2, 8*6, FACEBTN_ON, FACEBTN_OFF },
    { GCPad::BTN_C_LEFT,         2, 7*6, FACEBTN_ON, FACEBTN_OFF },
    { GCPad::BTN_C_RIGHT,         2, 7*6, FACEBTN_ON, FACEBTN_OFF },*/
    { GCPad::BTN_R,       0, 9*6, SHOULDERBTN_ON, SHOULDERBTN_OFF },
    { GCPad::BTN_L,       0, 1*6, SHOULDERBTN_ON, SHOULDERBTN_OFF }
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
      for(uint8_t x = 0; x < 12; x++){
        const Pad pad = padGameCube[x];
        PrintPadChar(index, padDivision[index].firstCol, pad.col, pad.row, pad.padvalue, true, pad.on, pad.off, true);
      }
    }
  }
#endif

void gameCubeResetAnalogMinMax() {
  gc_x_min = -50;
  gc_x_max = 50;
  gc_y_min = -50;
  gc_y_max = 50;
}

void gameCubeResetJoyValues(const uint8_t i) {
  if (i >= totalUsb)
    return;
  usbStick[i]->resetState();
  ((Joy1_*)usbStick[0])->setAnalog0(GAMECUBE_ANALOG_DEFAULT_CENTER); //x
  ((Joy1_*)usbStick[0])->setAnalog1(GAMECUBE_ANALOG_DEFAULT_CENTER); //y
  ((Joy1_*)usbStick[0])->setAnalog2(GAMECUBE_ANALOG_DEFAULT_CENTER); //rx
  ((Joy1_*)usbStick[0])->setAnalog3(GAMECUBE_ANALOG_DEFAULT_CENTER); //ry
  gameCubeResetAnalogMinMax();
}

void gameCubeSetup() {
  //Init onboard led pin
  //pinMode(LED_BUILTIN, OUTPUT);
  
  delay(5);

  totalUsb = 1;
  sleepTime = 50;
  
  //Create usb controllers
  for (uint8_t i = 0; i < totalUsb; i++) {
    usbStick[i] = new Joy1_("RZordGC", JOYSTICK_DEFAULT_REPORT_ID + i, JOYSTICK_TYPE_GAMEPAD, totalUsb,
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
    gameCubeResetJoyValues(i);
    usbStick[i]->sendState();
  }

  cube = new GCPad();
  
  delay(50);
  
  dstart(115200);
  debugln (F("Powered on!"));
}

inline bool __attribute__((always_inline))
gameCubeLoop() {
  static uint8_t lastControllerCount = 0;

  #ifdef ENABLE_REFLEX_PAD
    static bool firstTime = true;
    if(firstTime) {
      firstTime = false;
      showDefaultPadGameCube(0, false);
    }
  #endif

  static boolean haveController = false;
  static uint16_t oldButtons = 0;
  static int8_t oldLX = GAMECUBE_ANALOG_DEFAULT_CENTER, oldLY = GAMECUBE_ANALOG_DEFAULT_CENTER;
  static int8_t oldRX = GAMECUBE_ANALOG_DEFAULT_CENTER, oldRY = GAMECUBE_ANALOG_DEFAULT_CENTER;

  bool stateChanged = false;


  if (!haveController) {
    if (cube->begin ()) {
      //controller connected
      haveController = true;
      gameCubeResetJoyValues(0);
      #ifdef ENABLE_REFLEX_PAD
        showDefaultPadGameCube(0, haveController);
      #endif
      sleepTime = 50;
    } else {
      sleepTime = 50000;
    }
  } else {
    if (!cube->read ()) {
      //controller removed
      haveController = false;
      sleepTime = 50000;
      gameCubeResetJoyValues(0);
      #ifdef ENABLE_REFLEX_PAD
        showDefaultPadGameCube(0, haveController);
      #endif
    } else {
      //controller read sucess

      const bool buttonsChanged = cube->buttons != oldButtons;
      const bool analogChanged = cube->x != oldLX || cube->y != oldLY || cube->c_x != oldRX || cube->c_y != oldRY;

      if (buttonsChanged || analogChanged) { //state changed?
        stateChanged = true;

        uint16_t buttonData = 0;
        bitWrite(buttonData, 0, cube->buttons & GCPad::BTN_Y);
        bitWrite(buttonData, 1, cube->buttons & GCPad::BTN_B);
        bitWrite(buttonData, 2, cube->buttons & GCPad::BTN_A);
        bitWrite(buttonData, 3, cube->buttons & GCPad::BTN_X);
        bitWrite(buttonData, 8, cube->buttons & GCPad::BTN_Z);
        bitWrite(buttonData, 9, cube->buttons & GCPad::BTN_START);
        //bitWrite(buttonData, 4, cube->buttons & GCPad::BTN_L);
        //bitWrite(buttonData, 5, cube->buttons & GCPad::BTN_R);
        //Analog trigger as buttons
        bitWrite(buttonData, 4, cube->left_trigger > 100);
        bitWrite(buttonData, 5, cube->right_trigger > 100);

        ((Joy1_*)usbStick[0])->setButtons(buttonData);

        uint8_t hatData = 0;
        bitWrite(hatData, 0, (cube->buttons & GCPad::BTN_D_UP) == 0);
        bitWrite(hatData, 1, (cube->buttons & GCPad::BTN_D_DOWN) == 0);
        bitWrite(hatData, 2, (cube->buttons & GCPad::BTN_D_LEFT) == 0);
        bitWrite(hatData, 3, (cube->buttons & GCPad::BTN_D_RIGHT) == 0);

        //Get angle from hatTable and pass to joystick class
        ((Joy1_*)usbStick[0])->setHatSwitch(hatTable[hatData]);
        
        //Update min and max values
        /*if(cube->x < gc_x_min) {
          gc_x_min = n64.x;
          gc_x_max = n64.x * -1;
        }
        else if(n64.x > gc_x_max) {
          gc_x_min = n64.x * -1;
          gc_x_max = n64.x;
        }

        if(n64.y < gc_y_min) {
          gc_y_min = n64.y;
          gc_y_max = n64.y * -1;
        }
        else if(n64.y > gc_y_max) {
          gc_y_min = n64.y * -1;
          gc_y_max = n64.y;
        }*/

        //((Joy1_*)usbStick[0])->setAnalog0(map(cube->x, gc_x_min, gc_x_max, 0, 255)); //x
        //((Joy1_*)usbStick[0])->setAnalog1(map(cube->y, gc_y_max, gc_y_min, 0, 255)); //y

        ((Joy1_*)usbStick[0])->setAnalog0(cube->x); //x
        ((Joy1_*)usbStick[0])->setAnalog1(cube->y); //y

        ((Joy1_*)usbStick[0])->setAnalog2(cube->c_x); //x
        ((Joy1_*)usbStick[0])->setAnalog3(cube->c_y); //y

        usbStick[0]->sendState();

        #ifdef ENABLE_REFLEX_PAD
          //const uint8_t startCol = inputPort == 0 ? 0 : 11*6;
          for(uint8_t x = 0; x < 12; x++){
            const Pad pad = padGameCube[x];
            PrintPadChar(0, padDivision[0].firstCol, pad.col, pad.row, pad.padvalue, cube->buttons & pad.padvalue, pad.on, pad.off);
          }
        #endif

        //keep values
        oldButtons = cube->buttons;
        oldLX = cube->x;
        oldLY = cube->y;
        oldRX = cube->c_x;
        oldRY = cube->c_y;
        
      }//end if statechanged

    }//end else pad read
  }//end havecontroller


  return stateChanged; //joyCount != 0;
}
