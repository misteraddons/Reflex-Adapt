/*******************************************************************************
 * Sega Saturn input module for RetroZord / Reflex-Adapt.
 * By Matheus Fraguas (sonik-br)
 * 
 * Handles up to 2 input ports.
 * 
 * Works with digital pad and analog pad.
 * By using the multitap it's possible to connect multiple controllers.
 * Also works with MegaDrive controllers and mulltitaps.
 *
 * Uses SaturnLib
 * https://github.com/sonik-br/SaturnLib
 *
 * Uses a modified version of MPG
 * https://github.com/sonik-br/MPG
 *
*/

#include "RZInputModule.h"
#include "src/SaturnLib/SaturnLib.h"

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


class ReflexInputSaturn : public RZInputModule {
  private:
    SaturnPort<SAT1_D0, SAT1_D1, SAT1_D2, SAT1_D3, SAT1_TH, SAT1_TR, SAT1_TL> saturn1;
    SaturnPort<SAT2_D0, SAT2_D1, SAT2_D2, SAT2_D3, SAT2_TH, SAT2_TR, SAT2_TL> saturn2;

    #ifdef ENABLE_REFLEX_PAD
      const Pad padSat[13] {
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

      void loopPadDisplayChars(const uint8_t index, const SatDeviceType_Enum padType, const void* sc, const bool force) {
        for(uint8_t i = 0; i < (sizeof(padSat) / sizeof(Pad)); ++i){
          if(padType == SAT_DEVICE_MEGA3 && i == 8) //mega3 have less buttons
            break;
          if(padType == SAT_DEVICE_MEGA6 && i == 12) //skip L button
            continue;
          const Pad pad = padSat[i];
          PrintPadChar(index, padDivision[index].firstCol, pad.col, pad.row, pad.padvalue, !sc || static_cast<const SaturnController*>(sc)->digitalPressed(static_cast<SatDigital_Enum>(pad.padvalue)), pad.on, pad.off, force);
        }
      }
  
      void ShowDefaultPadSat(const uint8_t index, const SatDeviceType_Enum padType) {
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
            display.print(PSTR_TO_F(PSTR_NONE));
            return;
        }

        if (index < 2)
          loopPadDisplayChars(index, padType, NULL, true);
      }
    #endif
      
  public:
    ReflexInputSaturn() : RZInputModule() { }

    const char* getUsbId() override {
      static const char* usbId { "RZMSat" };
      return usbId;
    }

    const uint16_t getUsbVersion() override {
      return MODE_ID_SATURN;
    }

//    #ifdef ENABLE_REFLEX_PAD
//      void displayInputName() override {
//        display.setCol(2*6);
//        display.print(F("GENESIS + SATURN"));
//      }
//    #endif

    void setup() override {
      //Init the saturn class
      saturn1.begin();
      saturn2.begin();
  
      delay(50);
  
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

      delay(50);
    }

    void setup2() override { }
    
    bool read() override {
      //static uint32_t lastRead = 0;
      //static uint16_t sleepTime = 200;
      static uint8_t lastControllerCount = 0;

      #ifdef ENABLE_REFLEX_PAD
        static SatDeviceType_Enum lastPadType[] = { SAT_DEVICE_NOTSUPPORTED, SAT_DEVICE_NOTSUPPORTED, SAT_DEVICE_NOTSUPPORTED, SAT_DEVICE_NOTSUPPORTED, SAT_DEVICE_NOTSUPPORTED, SAT_DEVICE_NOTSUPPORTED };
        SatDeviceType_Enum currentPadType[] = { SAT_DEVICE_NONE, SAT_DEVICE_NONE, SAT_DEVICE_NONE, SAT_DEVICE_NONE, SAT_DEVICE_NONE, SAT_DEVICE_NONE };
        static bool firstTime = true;
        if(firstTime) {
          firstTime = false;
          if(totalUsb > 2) { //Multitap mode!
            display.setCursor(5*6, oledDisplayFirstRow + 3);
            display.print(PSTR_TO_F(PSTR_MULTITAP));
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
  
      for (uint8_t i = 0; i < joyCount; ++i) {
        if (i == totalUsb)
          break;
  
        //Get the data for the specific controller from a port
        const uint8_t inputPort = (i < joyCount1) ? 0 : 1;
        const SaturnController& sc = inputPort == 0 ? saturn1.getSaturnController(i) : saturn2.getSaturnController(i - joyCount1);

        #ifdef ENABLE_REFLEX_PAD
          //Only used if not in multitap mode
          if(totalUsb < 3) {
            if(inputPort == 0 && joyCount1 != 0)
              currentPadType[inputPort] = sc.deviceType();
            else if(inputPort == 1 && joyCount2 != 0)
              currentPadType[inputPort] = sc.deviceType();
          }
        #endif

        //Only process data if state changed from previous read
        if (sc.stateChanged()) {
          stateChanged = true;
          hasAnalogTriggers[i] = false;
          hasLeftAnalogStick[i] = false;

          //Controller just connected. Also can happen when changing mode on 3d pad
          if (sc.deviceJustChanged()) {
            resetState(i);
            #ifdef ENABLE_REFLEX_PAD
              //Only used if not in multitap mode
              if (totalUsb < 3)
                ShowDefaultPadSat(inputPort, sc.deviceType());
            #endif
          }

          state[i].dpad = 0
            | (sc.digitalPressed(SAT_PAD_UP)    ? GAMEPAD_MASK_UP    : 0)
            | (sc.digitalPressed(SAT_PAD_DOWN)  ? GAMEPAD_MASK_DOWN  : 0)
            | (sc.digitalPressed(SAT_PAD_LEFT)  ? GAMEPAD_MASK_LEFT  : 0)
            | (sc.digitalPressed(SAT_PAD_RIGHT) ? GAMEPAD_MASK_RIGHT : 0)
          ;

          state[i].buttons = 0
            | (sc.digitalPressed(SAT_A) ? GAMEPAD_MASK_B1 : 0) // Generic: K1, Switch: B, Xbox: A
            | (sc.digitalPressed(SAT_B) ? GAMEPAD_MASK_B2 : 0) // Generic: K2, Switch: A, Xbox: B
            | (sc.digitalPressed(SAT_X) ? GAMEPAD_MASK_B3 : 0) // Generic: P1, Switch: Y, Xbox: X
            | (sc.digitalPressed(SAT_Y) ? GAMEPAD_MASK_B4 : 0) // Generic: P2, Switch: X, Xbox: Y
            | (sc.digitalPressed(SAT_Z) ? GAMEPAD_MASK_L1 : 0) // Generic: P4, Switch: L, Xbox: LB
            | (sc.digitalPressed(SAT_C) ? GAMEPAD_MASK_R1 : 0) // Generic: P3, Switch: R, Xbox: RB
            | (sc.digitalPressed(SAT_L) ? GAMEPAD_MASK_L2 : 0) // Generic: K4, Switch: ZL, Xbox: LT (Digital)
            | (sc.digitalPressed(SAT_R) ? GAMEPAD_MASK_R2 : 0) // Generic: K3, Switch: ZR, Xbox: RT (Digital)
            //| (sc.digitalPressed(SELECT) ? GAMEPAD_MASK_S1 : 0) // Generic: Select, Switch: -, Xbox: View
            | (sc.digitalPressed(SAT_START) ? GAMEPAD_MASK_S2 : 0) // Generic: Start, Switch: +, Xbox: Menu
            //| (sc.digitalPressed(LCLICK) ? GAMEPAD_MASK_L3 : 0) // All: Left Stick Click
            //| (sc.digitalPressed(RCLICK) ? GAMEPAD_MASK_R3 : 0) // All: Right Stick Click
          ;

          if (sc.isAnalog()) {
            //Enable analog stick
            hasLeftAnalogStick[i] = true;
            
            state[i].lx = convertAnalog(sc.analog(SAT_ANALOG_X));
            state[i].ly = convertAnalog(sc.analog(SAT_ANALOG_Y));
            
            //Enable analog trigger
            if (canUseAnalogTrigger()) {
              hasAnalogTriggers[i] = true;
              state[i].lt = sc.analog(SAT_ANALOG_L);
              state[i].rt = sc.analog(SAT_ANALOG_R);
            }
            
            //For racing wheel, don't report digital left and right of the dpad
            if (sc.deviceType() == SAT_DEVICE_WHEEL) {
              state[i].dpad |= B0011;
            }
          }

          #ifdef ENABLE_REFLEX_PAD
            //Only used if not in multitap mode
            if (totalUsb < 3 && inputPort < 2)
              loopPadDisplayChars(inputPort, currentPadType[inputPort], &sc, false);
          #endif
        } //end if statechanged
      } //end for joycount

      #ifdef ENABLE_REFLEX_PAD
        for (uint8_t i = 0; i < 2; ++i) {
          //Only used if not in multitap mode
          if(totalUsb < 3 && lastPadType[i] != currentPadType[i] && currentPadType[i] == SAT_DEVICE_NONE)
            ShowDefaultPadSat(i, SAT_DEVICE_NONE);
          lastPadType[i] = currentPadType[i];
        }
      #endif

      //Controller has been disconnected? Reset it's values!
      if (lastControllerCount > joyCount) {
        for (uint8_t i = joyCount; i < lastControllerCount; ++i) {
          if (i == totalUsb)
            break;
          resetState(i);
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
      if (isMegadriveTap) //megadrive multitap
        sleepTime = 2000;
      else if (multitapPorts != 0) //saturn multitap
        sleepTime = (joyCount + 1) * 500;
      else
        sleepTime = 100;

      //lastRead = micros();
      return stateChanged;
    } //end read
  
};
