/*******************************************************************************
 * Sega Saturn controllers to USB using an Arduino Leonardo.
 *
 * Works with digital pad and analog pad.
 *
 * By using the multitap it's possible to connect up to 7 controllers.
 *
 * Also works with MegaDrive controllers and mulltitaps.
 *
 * For details on Joystick Library, see
 * https://github.com/MHeironimus/ArduinoJoystickLibrary
 *
*/

#include "src/SaturnLib/SaturnLib.h"
#include "src/ArduinoJoystickLibrary/Joy1.h"

//Saturn pins - Port 1
#define SAT1_TH 4 //S0
#if REFLEX_PIN_VERSION == 1
  #define SAT1_TR 3 //S1
#else
  #define SAT1_TR 13 //S1
#endif
#define SAT1_TL 5 //S2
#define SAT1_D0 9
#define SAT1_D1 8
#define SAT1_D2 7
#define SAT1_D3 6

//Saturn pins - Port 2
#define SAT2_TH 16 //S0
#define SAT2_TR 10 //S1
#define SAT2_TL 14 //S2
#define SAT2_D0 20
#define SAT2_D1 19
#define SAT2_D2 18
#define SAT2_D3 15

SaturnPort<SAT1_D0, SAT1_D1, SAT1_D2, SAT1_D3, SAT1_TH, SAT1_TR, SAT1_TL> saturn1;
SaturnPort<SAT2_D0, SAT2_D1, SAT2_D2, SAT2_D3, SAT2_TH, SAT2_TR, SAT2_TL> saturn2;

#ifdef ENABLE_REFLEX_PAD
  const Pad padSat[] = {
    { SAT_PAD_UP,    1, 1*6, UP_ON, UP_OFF },
    { SAT_PAD_DOWN,  3, 1*6, DOWN_ON, DOWN_OFF },
    { SAT_PAD_LEFT,  2, 0,   LEFT_ON, LEFT_OFF },
    { SAT_PAD_RIGHT, 2, 2*6, RIGHT_ON, RIGHT_OFF },
    { SAT_B,         3, 7*6, FACEBTN_ON, FACEBTN_OFF },
    { SAT_C,         3, 8*6, FACEBTN_ON, FACEBTN_OFF },
    { SAT_A,         3, 6*6, FACEBTN_ON, FACEBTN_OFF },
    { SAT_START,     2, 4*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
    { SAT_Z,         2, 8*6, FACEBTN_ON, FACEBTN_OFF },
    { SAT_Y,         2, 7*6, FACEBTN_ON, FACEBTN_OFF },
    { SAT_X,         2, 6*6, FACEBTN_ON, FACEBTN_OFF },
    { SAT_R,         0, 7*6, SHOULDERBTN_ON, SHOULDERBTN_OFF },
    { SAT_L,         0, 1*6, SHOULDERBTN_ON, SHOULDERBTN_OFF }
  };
  
  void ShowDefaultPadSat(const uint8_t index, const SatDeviceType_Enum padType) {
    //print default joystick state to oled screen

    //const uint8_t firstCol = index == 0 ? 0 : 11*6;
    //const uint8_t lastCol = index == 0 ? 11*6 : 127;
  
    display.clear(padDivision[index].firstCol, padDivision[index].lastCol, oledDisplayFirstRow + 1, 7);
    display.setCursor(padDivision[index].firstCol, 7);
    
    switch(padType) {
      case SAT_DEVICE_MEGA3:
        display.print(F("GENESIS-3"));
        break;
      case SAT_DEVICE_MEGA6:
        display.print(F("GENESIS-6"));
        break;
      case SAT_DEVICE_PAD:
        display.print(F("SATURN"));
        break;
      case SAT_DEVICE_3DPAD:
        display.print(F("SAT-3D"));
        break;
      case SAT_DEVICE_WHEEL:
        display.print(F("SAT-WHEEL"));
        break;
      default:
        display.print(F("NONE"));
        return;
    }
  
    if (index < 2) {
      //const uint8_t startCol = index == 0 ? 0 : 11*6;
      for(uint8_t x = 0; x < 13; x++){
        if(padType == SAT_DEVICE_MEGA3 && x == 8)
          break;
        if(padType == SAT_DEVICE_MEGA6 && x == 12) //skip L button
          continue;
        const Pad pad = padSat[x];
        PrintPadChar(index, padDivision[index].firstCol, pad.col, pad.row, pad.padvalue, true, pad.on, pad.off, true);
      }
    }
  }
#endif



void satResetJoyValues(const uint8_t i) {
    if (i >= totalUsb)
        return;

    usbStick[i]->resetState();
}

void saturnSetup() {
    //Init onboard led pin
    //pinMode(LED_BUILTIN, OUTPUT);

    //Init the saturn class
    saturn1.begin();
    saturn2.begin();

    delayMicroseconds(10);

    //Detect multitap on first port
    saturn1.detectMultitap();
    uint8_t tap = saturn1.getMultitapPorts();
    //if (tap == 0) { //Detect on second port
    //    saturn2.detectMultitap();
    //    tap = saturn2.getMultitapPorts();
    //}

    if (tap == 0) //No multitap connected during boot
        totalUsb = 2;
    else //Multitap connected with 4 or 6 ports.
        totalUsb = MAX_USB_STICKS;//min(tap + 1, MAX_USB_STICKS);

    //Create usb controllers
    for (uint8_t i = 0; i < totalUsb; i++) {
        usbStick[i] = new Joy1_("RZordSat", JOYSTICK_DEFAULT_REPORT_ID + i, JOYSTICK_TYPE_GAMEPAD, totalUsb,
          true,//includeXAxis,
          true,//includeYAxis,
          false,//includeZAxis,
          false,//includeRxAxis,
          false,//includeRyAxis,
          true,//includeThrottle,
          true,//includeBrake,
          false);//includeSteering
    }

    //Set usb parameters and reset to default values
    for (uint8_t i = 0; i < totalUsb; i++) {
        //usbStick[i]->setXAxisRange(0, 255);
        //usbStick[i]->setYAxisRange(0, 255);
        //usbStick[i]->setThrottleRange(0, 255);
        //usbStick[i]->setBrakeRange(0, 255);
        //usbStick[i]->begin(false); //disable automatic sendState
        satResetJoyValues(i);
        usbStick[i]->sendState();
    }

    delay(50);

    dstart(115200);
    //debugln (F("Powered on!"));
}

inline bool __attribute__((always_inline))
saturnLoop() {
    static uint8_t lastControllerCount = 0;
    #ifdef ENABLE_REFLEX_PAD
      //static SatDeviceType_Enum lastPadType[] = { SAT_DEVICE_NONE, SAT_DEVICE_NONE };
      //SatDeviceType_Enum currentPadType[] = { SAT_DEVICE_NONE, SAT_DEVICE_NONE };
      static SatDeviceType_Enum lastPadType[] = { SAT_DEVICE_NOTSUPPORTED, SAT_DEVICE_NOTSUPPORTED, SAT_DEVICE_NOTSUPPORTED, SAT_DEVICE_NOTSUPPORTED, SAT_DEVICE_NOTSUPPORTED, SAT_DEVICE_NOTSUPPORTED };
      SatDeviceType_Enum currentPadType[] = { SAT_DEVICE_NONE, SAT_DEVICE_NONE, SAT_DEVICE_NONE, SAT_DEVICE_NONE, SAT_DEVICE_NONE, SAT_DEVICE_NONE };

      static bool firstTime = true;
      if(firstTime) {
        firstTime = false;
        //Multitap mode!
        if(totalUsb > 2) {
          display.setCursor(5*6, oledDisplayFirstRow + 3);
          display.print(F("MULTI-TAP"));
        }
      }
    #endif
    bool stateChanged = false;

    //Read each saturn port
    saturn1.update();
    saturn2.update();

    //Get the number of connected controllers on each port
    const uint8_t joyCount1 = saturn1.getControllerCount();
    const uint8_t joyCount2 = saturn2.getControllerCount();
    const uint8_t joyCount = joyCount1 + joyCount2;

    for (uint8_t i = 0; i < joyCount; i++) {
        if (i == totalUsb)
            break;

        //Get the data for the specific controller from a port
        const uint8_t inputPort = (i < joyCount1) ? 0 : 1;
        const SaturnController& sc = inputPort == 0 ? saturn1.getSaturnController(i) : saturn2.getSaturnController(i - joyCount1);

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
        if (sc.stateChanged()) {
          stateChanged = true;

            //Controller just connected. Also can happen when changing mode on 3d pad
            if (sc.deviceJustChanged()) {
                satResetJoyValues(i);
                #ifdef ENABLE_REFLEX_PAD
                  //Only used if not in multitap mode
                  if (totalUsb == 2)
                    ShowDefaultPadSat(inputPort, sc.deviceType());
                #endif
            }

            //const bool isAnalog = sc.getIsAnalog();
            uint8_t hatData = sc.hat();
            uint16_t buttonData = 0;

            bitWrite(buttonData, 1, sc.digitalPressed(SAT_A));
            bitWrite(buttonData, 2, sc.digitalPressed(SAT_B));
            bitWrite(buttonData, 5, sc.digitalPressed(SAT_C));
            bitWrite(buttonData, 0, sc.digitalPressed(SAT_X));
            bitWrite(buttonData, 3, sc.digitalPressed(SAT_Y));
            bitWrite(buttonData, 4, sc.digitalPressed(SAT_Z));
            bitWrite(buttonData, 9, sc.digitalPressed(SAT_START));

            if (sc.isAnalog()) {
                ((Joy1_*)usbStick[i])->setAnalog0(sc.analog(SAT_ANALOG_X)); //x
                ((Joy1_*)usbStick[i])->setAnalog1(sc.analog(SAT_ANALOG_Y)); //y
                ((Joy1_*)usbStick[i])->setAnalog2(sc.analog(SAT_ANALOG_R)); //throttle
                ((Joy1_*)usbStick[i])->setAnalog3(sc.analog(SAT_ANALOG_L)); //brake

                //For racing wheel, don't report digital left and right of the dpad
                if (sc.deviceType() == SAT_DEVICE_WHEEL) {
                    hatData |= B1100;
                }

            }
            else {
                //Only report digital L and R on digital controllers.
                //The 3d pad will report both the analog and digital state for those when in analog mode.
                bitWrite(buttonData, 7, sc.digitalPressed(SAT_R)); //R
                bitWrite(buttonData, 6, sc.digitalPressed(SAT_L)); //L
            }

            ((Joy1_*)usbStick[i])->setButtons(buttonData);

            //Get angle from hatTable and pass to joystick class
            ((Joy1_*)usbStick[i])->setHatSwitch(hatTable[hatData]);

            usbStick[i]->sendState();

            #ifdef ENABLE_REFLEX_PAD
              //Only used if not in multitap mode
              if (totalUsb == 2 && inputPort < 2) {
                //const uint8_t startCol = inputPort == 0 ? 0 : 11*6;
                const SatDeviceType_Enum padType = currentPadType[inputPort];//sc.deviceType();
                for(uint8_t x = 0; x < 13; x++){
                  if(padType == SAT_DEVICE_MEGA3 && x == 8)
                    break;
                  if(padType == SAT_DEVICE_MEGA6 && x == 12) //skip L button
                    continue;
                  const Pad pad = padSat[x];
                  PrintPadChar(inputPort, padDivision[inputPort].firstCol, pad.col, pad.row, pad.padvalue, sc.digitalPressed((SatDigital_Enum)pad.padvalue), pad.on, pad.off);
                }
              }
            #endif
        }

    }
    
    #ifdef ENABLE_REFLEX_PAD
      for (uint8_t i = 0; i < 2; i++) {
        //Only used if not in multitap mode
        if(totalUsb == 2 && lastPadType[i] != currentPadType[i] && currentPadType[i] == SAT_DEVICE_NONE)
          ShowDefaultPadSat(i, SAT_DEVICE_NONE);
        lastPadType[i] = currentPadType[i];
      }
    #endif

    //Controller has been disconnected? Reset it's values!
    if (lastControllerCount > joyCount) {
        for (uint8_t i = joyCount; i < lastControllerCount; i++) {
            if (i == totalUsb)
                break;
            satResetJoyValues(i);
            usbStick[i]->sendState();
        }
    }

    //Keep count for next read
    lastControllerCount = joyCount;

    const uint8_t tap1 = saturn1.getMultitapPorts();
    const uint8_t tap2 = saturn2.getMultitapPorts();
    const uint8_t multitapPorts = tap1 + tap2;
    const bool isMegadriveTap = (tap1 == TAP_MEGA_PORTS) || (tap2 == TAP_MEGA_PORTS);

//megadrive pad and multitap timing are correct
//need to check saturn, saturn mtap

    if (isMegadriveTap) { //megadrive multitap
        sleepTime = 2000;//2500;//4000;
    } else {
        //if (joyCount == 0)
        //    sleepTime = 1000;
        if (multitapPorts != 0) //saturn multitap
            sleepTime = (joyCount + 1) * 500;
        else
            sleepTime = 50; //sleepTime = joyCount * 500;
    }

    //sleepTime = 2000;

    //Sleep if total loop time was less than sleepTime
    /*unsigned long delta = micros() - start;
    if (delta < sleepTime) {
        //debugln(delta);
        delta = sleepTime - delta;
        //debug(F("\t"));
        //debugln(delta);
        //delayMicroseconds(delta);
    }
    delayMicroseconds(2500);*/

    return stateChanged; //joyCount != 0;
}
