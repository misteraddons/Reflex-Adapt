/*******************************************************************************
 * 3DO input module for RetroZord / Reflex-Adapt.
 * By Matheus Fraguas (sonik-br)
 * 
 * Handles up to 2 input ports.
 * 
 * Works with digital pad.
 * Supports multiple daisy chained devices.
 *
 * Uses PceLib
 * https://github.com/sonik-br/ThreedoLib
 *
 * Uses a modified version of MPG
 * https://github.com/sonik-br/MPG
 *
*/

#include "RZInputModule.h"
#include "src/ThreedoLib/ThreedoLib.h"

//3DO pins
#define THREEDOPIN_CLOCK  4 // (db9 7)
#define THREEDOPIN_DOUT   5 // (db9 6)
//#define THREEDO_PIN_DIN   // (db9 9)
#if REFLEX_PIN_VERSION == 1
  #define THREEDOPIN_DIN 3
#else
  #define THREEDOPIN_DIN 13
#endif

class ReflexInput3do : public RZInputModule {
  private:
    ThreedoPort<THREEDOPIN_CLOCK, THREEDOPIN_DOUT, THREEDOPIN_DIN> tdo1;
    
    #ifdef ENABLE_REFLEX_PAD
      const Pad pad3do[11] = {
        { THREEDO_UP,    1, 1*6, UP_ON, UP_OFF },
        { THREEDO_DOWN,  3, 1*6, DOWN_ON, DOWN_OFF },
        { THREEDO_LEFT,  2, 0,   LEFT_ON, LEFT_OFF },
        { THREEDO_RIGHT, 2, 2*6, RIGHT_ON, RIGHT_OFF },
        { THREEDO_B,     2, 7*6, FACEBTN_ON, FACEBTN_OFF },
        { THREEDO_C,     2, 8*6, FACEBTN_ON, FACEBTN_OFF },
        { THREEDO_A,     2, 6*6, FACEBTN_ON, FACEBTN_OFF },
        { THREEDO_P,     2, (5*6)-3, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
        { THREEDO_X,     2, (4*6)-3, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
        { THREEDO_R,     0, 7*6, SHOULDERBTN_ON, SHOULDERBTN_OFF },
        { THREEDO_L,     0, 1*6, SHOULDERBTN_ON, SHOULDERBTN_OFF }
      };
    
      void loopPadDisplayChars3do(const uint8_t index, const ThreedoDeviceType_Enum padType, const void* sc, const bool force) {
        for(uint8_t i = 0; i < (sizeof(pad3do) / sizeof(Pad)); ++i){
          const Pad pad = pad3do[i];
          PrintPadChar(index, padDivision[index].firstCol, pad.col, pad.row, pad.padvalue, !sc || static_cast<const ThreedoController*>(sc)->digitalPressed(static_cast<ThreedoDigital_Enum>(pad.padvalue)), pad.on, pad.off, force);
        }
      }
    
      void ShowDefaultPad3do(const uint8_t index, const ThreedoDeviceType_Enum padType) {
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
          loopPadDisplayChars3do(index, padType, NULL, true);
        }
      }
    #endif
    
  public:
    ReflexInput3do() : RZInputModule() { }

    const char* getUsbId() override {
      static const char* usbId { "RZM3do" };
      return usbId;
    }

    const uint16_t getUsbVersion() override {
      return MODE_ID_3DO;
    }

//    #ifdef ENABLE_REFLEX_PAD
//      void displayInputName() override {
//        display.setCol(9*6);
//        display.print(F("3DO"));
//      }
//    #endif

    void setup() override {
      //Init the 3do class
      tdo1.begin();

      delayMicroseconds(10);

      //todo detect number of connected devices?
      totalUsb = min(THREEDO_MAX_CTRL, MAX_USB_STICKS);

      sleepTime = 500;

      delay(50);
    }

    void setup2() override { }
    
    bool read() override {
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
              resetState(i);
              #ifdef ENABLE_REFLEX_PAD
                if(inputPort < 2)
                  ShowDefaultPad3do(inputPort, sc.deviceType());
              #endif
            }

            state[i].dpad = 0
              | (sc.digitalPressed(THREEDO_UP)    ? GAMEPAD_MASK_UP    : 0)
              | (sc.digitalPressed(THREEDO_DOWN)  ? GAMEPAD_MASK_DOWN  : 0)
              | (sc.digitalPressed(THREEDO_LEFT)  ? GAMEPAD_MASK_LEFT  : 0)
              | (sc.digitalPressed(THREEDO_RIGHT) ? GAMEPAD_MASK_RIGHT : 0)
            ;
            
            state[i].buttons = 0
              | (sc.digitalPressed(THREEDO_B) ? GAMEPAD_MASK_B1 : 0) // Generic: K1, Switch: B, Xbox: A
              | (sc.digitalPressed(THREEDO_C) ? GAMEPAD_MASK_B2 : 0) // Generic: K2, Switch: A, Xbox: B
              | (sc.digitalPressed(THREEDO_A) ? GAMEPAD_MASK_B3 : 0) // Generic: P1, Switch: Y, Xbox: X
              //| (sc.digitalPressed(Y) ? GAMEPAD_MASK_B4 : 0) // Generic: P2, Switch: X, Xbox: Y
              | (sc.digitalPressed(THREEDO_L) ? GAMEPAD_MASK_L1 : 0) // Generic: P4, Switch: L, Xbox: LB
              | (sc.digitalPressed(THREEDO_R) ? GAMEPAD_MASK_R1 : 0) // Generic: P3, Switch: R, Xbox: RB
              //| (sc.digitalPressed(LT) ? GAMEPAD_MASK_L2 : 0) // Generic: K4, Switch: ZL, Xbox: LT (Digital)
              //| (sc.digitalPressed(RT) ? GAMEPAD_MASK_R2 : 0) // Generic: K3, Switch: ZR, Xbox: RT (Digital)
              | (sc.digitalPressed(THREEDO_X) ? GAMEPAD_MASK_S1 : 0) // Generic: Select, Switch: -, Xbox: View
              | (sc.digitalPressed(THREEDO_P) ? GAMEPAD_MASK_S2 : 0) // Generic: Start, Switch: +, Xbox: Menu
              //| (sc.digitalPressed(LCLICK) ? GAMEPAD_MASK_L3 : 0) // All: Left Stick Click
              //| (sc.digitalPressed(RCLICK) ? GAMEPAD_MASK_R3 : 0) // All: Right Stick Click
            ;

            #ifdef ENABLE_REFLEX_PAD
              if (inputPort < 2) {
                const ThreedoDeviceType_Enum padType = currentPadType[inputPort];//sc.deviceType();
                loopPadDisplayChars3do(inputPort, padType, &sc, false);
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
          resetState(i);
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
        sleepTime = 100 * joyCount;
      }
    
      return stateChanged; //joyCount != 0;
    }//end read

};
