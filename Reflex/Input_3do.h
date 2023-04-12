/*******************************************************************************
 * 3DO controllers to USB using an Arduino Leonardo.
 *
 * Works with digital pad.
 *
 * Supports multiple daisy chained devices.
 *
 * For details on Joystick Library, see
 * https://github.com/MHeironimus/ArduinoJoystickLibrary
 *
*/


#include "src/ThreedoLib/ThreedoLib.h"
#include "src/ArduinoJoystickLibrary/Joy1.h"

//3DO pins

#define THREEDOPIN_CLOCK  4 // (db9 7)
#define THREEDOPIN_DOUT   5 // (db9 6)
//#define THREEDO_PIN_DIN   // (db9 9)
#if REFLEX_PIN_VERSION == 1
  #define THREEDOPIN_DIN 3
#else
  #define THREEDOPIN_DIN 13
#endif

ThreedoPort<THREEDOPIN_CLOCK, THREEDOPIN_DOUT, THREEDOPIN_DIN> tdo1;


#ifdef ENABLE_REFLEX_PAD
  const Pad pad3do[] = {
    { THREEDO_UP,    1, 1*6, UP_ON, UP_OFF },
    { THREEDO_DOWN,  3, 1*6, DOWN_ON, DOWN_OFF },
    { THREEDO_LEFT,  2, 0,   LEFT_ON, LEFT_OFF },
    { THREEDO_RIGHT, 2, 2*6, RIGHT_ON, RIGHT_OFF },
    { THREEDO_B,     2, 7*6, FACEBTN_ON, FACEBTN_OFF },
    { THREEDO_C,     2, 8*6, FACEBTN_ON, FACEBTN_OFF },
    { THREEDO_A,     2, 6*6, FACEBTN_ON, FACEBTN_OFF },
    { THREEDO_P,     2, (4*6)-3, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
    { THREEDO_X,     2, (5*6)-3, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
    { THREEDO_R,     0, 7*6, SHOULDERBTN_ON, SHOULDERBTN_OFF },
    { THREEDO_L,     0, 1*6, SHOULDERBTN_ON, SHOULDERBTN_OFF }
  };
  
  void ShowDefaultPad3do(const uint8_t index, const ThreedoDeviceType_Enum padType) {
    //print default joystick state to oled screen
  
    //const uint8_t firstCol = index == 0 ? 0 : 11*6;
    //const uint8_t lastCol = index == 0 ? 11*6 : 127;
  
    display.clear(padDivision[index].firstCol, padDivision[index].lastCol, oledDisplayFirstRow + 1, 7);
    display.setCursor(padDivision[index].firstCol, 7);
    
    switch(padType) { //only show pad type on the first input port
      case THREEDO_DEVICE_PAD:
        if (index == 0)
          display.print(PSTR_TO_F(PSTR_DIGITAL));
        break;
      default:
        if (index == 0)
          display.print(PSTR_TO_F(PSTR_NONE));
        return;
    }
  
    if (index < 2) {
      //const uint8_t startCol = index == 0 ? 0 : 11*6;
      for(uint8_t x = 0; x < 11; x++){
        const Pad pad = pad3do[x];
        PrintPadChar(index, padDivision[index].firstCol, pad.col, pad.row, pad.padvalue, true, pad.on, pad.off, true);
      }
    }
  }
#endif


void threedoResetJoyValues(const uint8_t i) {
    if (i >= totalUsb)
        return;

    usbStick[i]->resetState();
}

void threedoSetup() {
    //Init onboard led pin
    //pinMode(LED_BUILTIN, OUTPUT);

    //Init the 3do class
    tdo1.begin();

    delayMicroseconds(10);

    //todo detect number of connected devices?

    totalUsb = min(THREEDO_MAX_CTRL, MAX_USB_STICKS);

    //Create usb controllers
    for (uint8_t i = 0; i < totalUsb; i++) {
        usbStick[i] = new Joy1_("RZord3do", JOYSTICK_DEFAULT_REPORT_ID + i, JOYSTICK_TYPE_GAMEPAD, totalUsb);
    }

    //Set usb parameters and reset to default values
    for (uint8_t i = 0; i < totalUsb; i++) {
        threedoResetJoyValues(i);
        usbStick[i]->sendState();
    }

    delay(50);

    dstart(115200);
    //debugln (F("Powered on!"));
}

inline bool __attribute__((always_inline))
threedoLoop() {
    static uint8_t lastControllerCount = 0;
    #ifdef ENABLE_REFLEX_PAD
      static ThreedoDeviceType_Enum lastPadType[] = { THREEDO_DEVICE_NOTSUPPORTED, THREEDO_DEVICE_NOTSUPPORTED, THREEDO_DEVICE_NOTSUPPORTED, THREEDO_DEVICE_NOTSUPPORTED, THREEDO_DEVICE_NOTSUPPORTED, THREEDO_DEVICE_NOTSUPPORTED };
      ThreedoDeviceType_Enum currentPadType[] = { THREEDO_DEVICE_NONE, THREEDO_DEVICE_NONE, THREEDO_DEVICE_NONE, THREEDO_DEVICE_NONE, THREEDO_DEVICE_NONE, THREEDO_DEVICE_NONE };
    #endif
    bool stateChanged = false;

    //Read each 3do port
    tdo1.update();
    

    //Get the number of connected controllers on each port
    const uint8_t joyCount = tdo1.getControllerCount();

    for (uint8_t i = 0; i < joyCount; i++) {
        if (i == totalUsb)
            break;

        //Get the data for the specific controller from a port
        const uint8_t inputPort = i;//(i < joyCount1) ? 0 : 1;
        const ThreedoController& sc = tdo1.get3doController(i);

        #ifdef ENABLE_REFLEX_PAD
          if(inputPort == 0 && joyCount > 0)
            currentPadType[inputPort] = sc.deviceType();
          else if(inputPort == 1 && joyCount > 0)
            currentPadType[inputPort] = sc.deviceType();
        #endif

        //Only process data if state changed from previous read
        if (sc.stateChanged()) {
          stateChanged = true;

            //Controller just connected. Also can happen when changing mode on 3d pad
            if (sc.deviceJustChanged()) {
                threedoResetJoyValues(i);
                #ifdef ENABLE_REFLEX_PAD
                  if(inputPort < 2)
                    ShowDefaultPad3do(inputPort, sc.deviceType());
                #endif
            }

            uint16_t buttonData = 0;

            bitWrite(buttonData, 0, sc.digitalPressed(THREEDO_A));
            bitWrite(buttonData, 1, sc.digitalPressed(THREEDO_B));
            bitWrite(buttonData, 2, sc.digitalPressed(THREEDO_C));
            bitWrite(buttonData, 8, sc.digitalPressed(THREEDO_X));//select
            bitWrite(buttonData, 9, sc.digitalPressed(THREEDO_P));//start
            bitWrite(buttonData, 7, sc.digitalPressed(THREEDO_R)); //R
            bitWrite(buttonData, 6, sc.digitalPressed(THREEDO_L)); //L

            ((Joy1_*)usbStick[i])->setButtons(buttonData);

            uint8_t hatData = B0;
            bitWrite(hatData, 0, !sc.digitalPressed(THREEDO_UP));
            bitWrite(hatData, 1, !sc.digitalPressed(THREEDO_DOWN));
            bitWrite(hatData, 2, !sc.digitalPressed(THREEDO_LEFT));
            bitWrite(hatData, 3, !sc.digitalPressed(THREEDO_RIGHT));

            //Get angle from hatTable and pass to joystick class
            ((Joy1_*)usbStick[i])->setHatSwitch(hatTable[hatData]);

            usbStick[i]->sendState();

            #ifdef ENABLE_REFLEX_PAD
              if (inputPort < 2) {
                //const uint8_t startCol = inputPort == 0 ? 0 : 11*6;
                //const ThreedoDeviceType_Enum padType = currentPadType[inputPort];//sc.deviceType();
                for(uint8_t x = 0; x < 11; x++){
                  const Pad pad = pad3do[x];
                  PrintPadChar(inputPort, padDivision[inputPort].firstCol, pad.col, pad.row, pad.padvalue, sc.digitalPressed((ThreedoDigital_Enum)pad.padvalue), pad.on, pad.off);
                }
              }
            #endif
        }

    }
    
    #ifdef ENABLE_REFLEX_PAD
      for (uint8_t i = 0; i < 2; i++) {
        if(lastPadType[i] != currentPadType[i] && currentPadType[i] == THREEDO_DEVICE_NONE)
          ShowDefaultPad3do(i, THREEDO_DEVICE_NONE);
        lastPadType[i] = currentPadType[i];
      }
    #endif

    //Controller has been disconnected? Reset it's values!
    if (lastControllerCount > joyCount) {
        for (uint8_t i = joyCount; i < lastControllerCount; i++) {
            if (i == totalUsb)
                break;
            threedoResetJoyValues(i);
            usbStick[i]->sendState();
        }
    }

    //Keep count for next read
    lastControllerCount = joyCount;

    //Keep clock high for some time while pad stabilize it's data
    if(joyCount == 0 || lastControllerCount != joyCount) {
      //Nothing connected, or number of devices changed. Do a longer rest
      sleepTime = 500;
    } else {
      //100us per device seems enough
      sleepTime = 100*joyCount;
    }

    return stateChanged; //joyCount != 0;
}
