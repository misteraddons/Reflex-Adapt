/*******************************************************************************
 * NEC PC Engine input module for RetroZord / Reflex-Adapt.
 * By Matheus Fraguas (sonik-br)
 * 
 * Handles up to 2 input ports.
 * 
 * Works with 2btn and 6btn digital pad.
 * By using the multitap it's possible to connect multiple controllers.
 *
 * Uses PceLib
 * https://github.com/sonik-br/PceLib
 *
 * Uses a modified version of MPG
 * https://github.com/sonik-br/MPG
 *
*/

#include "RZInputModule.h"
#include "src/PceLib/PceLib.h"

//PCE pins - Port 1
#define PCE1_SEL 5
#define PCE1_CLR 4
#define PCE1_D0  9 //U 1 3
#define PCE1_D1  8 //R 2 4
#define PCE1_D2  7 //D S 5
#define PCE1_D3  6 //L R 6

//PCE pins - Port 2
#define PCE2_SEL 14
#define PCE2_CLR 16
#define PCE2_D0  20 //U 1 3
#define PCE2_D1  19 //R 2 4
#define PCE2_D2  18 //D S 5
#define PCE2_D3  15 //L R 6

class ReflexInputPce : public RZInputModule {
  private:
    PcePort<PCE1_SEL, PCE1_CLR, PCE1_D0, PCE1_D1, PCE1_D2, PCE1_D3> pce1;
    PcePort<PCE2_SEL, PCE2_CLR, PCE2_D0, PCE2_D1, PCE2_D2, PCE2_D3> pce2;

    #ifdef ENABLE_REFLEX_PAD
      const Pad padPce[12] = {
        { PCE_PAD_UP,    1, 1*6, UP_ON, UP_OFF },
        { PCE_PAD_RIGHT, 2, 2*6, RIGHT_ON, RIGHT_OFF },
        { PCE_PAD_DOWN,  3, 1*6, DOWN_ON, DOWN_OFF },
        { PCE_PAD_LEFT,  2, 0,   LEFT_ON, LEFT_OFF },
        { PCE_1,         3, 9*6, FACEBTN_ON, FACEBTN_OFF },
        { PCE_2,         3, 8*6, FACEBTN_ON, FACEBTN_OFF },
        { PCE_SELECT,    2, 4*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
        { PCE_RUN,       2, 5*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
        { PCE_3,         3, 7*6, FACEBTN_ON, FACEBTN_OFF },
        { PCE_4,         2, 7*6, FACEBTN_ON, FACEBTN_OFF },
        { PCE_5,         2, 8*6, FACEBTN_ON, FACEBTN_OFF },
        { PCE_6,         2, 9*6, FACEBTN_ON, FACEBTN_OFF }
      };
      
      void loopPadDisplayChars(const uint8_t index, const PceDeviceType_Enum padType, const void* sc, const bool force) {
        for(uint8_t i = 0; i < (sizeof(padPce) / sizeof(Pad)); ++i){
          if(padType == PCE_DEVICE_PAD2 && i > 7)
            continue;
          const Pad pad = padPce[i];
          PrintPadChar(index, padDivision[index].firstCol, pad.col, pad.row, pad.padvalue, !sc || static_cast<const PceController*>(sc)->digitalPressed(static_cast<PceDigital_Enum>(pad.padvalue)), pad.on, pad.off, force);
        }
      }
      
      void ShowDefaultPadPce(const uint8_t index, PceDeviceType_Enum padType) {
        //If multitap support is enabled, force display as 2btn pad. (can't detect if a 2btn pad is presend or not)
        #ifdef PCE_ENABLE_MULTITAP
          if(padType == PCE_DEVICE_NONE)
            padType = PCE_DEVICE_PAD2;
        #endif
      
        display.clear(padDivision[index].firstCol, padDivision[index].lastCol, oledDisplayFirstRow + 1, 7);
        display.setCursor(padDivision[index].firstCol, 7);
        
        switch(padType) {
          case PCE_DEVICE_PAD2:
            display.print(F("PCE-2"));
            break;
          case PCE_DEVICE_PAD6:
            display.print(F("PCE-6"));
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
    ReflexInputPce() : RZInputModule() { }

    const char* getUsbId() override {
      static const char* usbId { "RZMPce" };
      return usbId;
    }

    const uint16_t getUsbVersion() override {
      return MODE_ID_PCE;
    }

//    #ifdef ENABLE_REFLEX_PAD
//      void displayInputName() override {
//        display.setCol(6*6);
//        display.print(F("PC-ENGINE"));
//      }
//    #endif

    void setup() override {
      //Init the pce class
      pce1.begin();
      pce2.begin();
      
      delayMicroseconds(10);
      
      //Detect multitap on first port
      pce1.detectMultitap();
      
      const uint8_t tap = pce1.getMultitapPorts();//pce1.getMultitapPorts() + pce2.getMultitapPorts();
     
      if (tap == 0) //No multitap connected during boot
        totalUsb = 2;
      else //Multitap connected.
        totalUsb = min(tap + 1, MAX_USB_STICKS);
    
      delay(50);
    }

    void setup2() override { }

    bool read() override {
      static uint8_t lastControllerCount = 0;
      #ifdef ENABLE_REFLEX_PAD
        static PceDeviceType_Enum lastPadType[] = { PCE_DEVICE_NOTSUPPORTED, PCE_DEVICE_NOTSUPPORTED, PCE_DEVICE_NOTSUPPORTED, PCE_DEVICE_NOTSUPPORTED, PCE_DEVICE_NOTSUPPORTED, PCE_DEVICE_NOTSUPPORTED };
        PceDeviceType_Enum currentPadType[] = { PCE_DEVICE_NONE, PCE_DEVICE_NONE, PCE_DEVICE_NONE, PCE_DEVICE_NONE, PCE_DEVICE_NONE, PCE_DEVICE_NONE };
    
        static bool firstTime = true;
        if(firstTime) {
          firstTime = false;
          //Multitap mode!
          if(totalUsb > 2) {
            display.setCursor(5*6, oledDisplayFirstRow + 3);
            display.print(PSTR_TO_F(PSTR_MULTITAP));
          }
        }
      #endif
      
      bool stateChanged = false;
      
      //Read each pce port
      pce1.update();
      pce2.update();
    
      //Get the number of connected controllers on each port
      const uint8_t joyCount1 = pce1.getControllerCount();
      const uint8_t joyCount2 = pce2.getControllerCount();
      const uint8_t joyCount = joyCount1 + joyCount2;
      
      for (uint8_t i = 0; i < joyCount; ++i) {
        if (i == totalUsb)
          break;
        
        //Get the data for the specific controller from a port
        const uint8_t inputPort = (i < joyCount1) ? 0 : 1;
        const PceController& sc = inputPort == 0 ? pce1.getPceController(i) : pce2.getPceController(i - joyCount1);
    
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
          
          //Controller just connected. Also can happen when changing mode on 6btn pad
          if (sc.deviceJustChanged()) {
            resetState(i);
            #ifdef ENABLE_REFLEX_PAD
              //Only used if not in multitap mode
              if (totalUsb == 2)
                ShowDefaultPadPce(inputPort, sc.deviceType());
            #endif
          }
          
          state[i].dpad = 0
            | (sc.digitalPressed(PCE_PAD_UP)    ? GAMEPAD_MASK_UP    : 0)
            | (sc.digitalPressed(PCE_PAD_DOWN)  ? GAMEPAD_MASK_DOWN  : 0)
            | (sc.digitalPressed(PCE_PAD_LEFT)  ? GAMEPAD_MASK_LEFT  : 0)
            | (sc.digitalPressed(PCE_PAD_RIGHT) ? GAMEPAD_MASK_RIGHT : 0)
          ;

          state[i].buttons = 0
            | (sc.digitalPressed(PCE_2) ? GAMEPAD_MASK_B1 : 0) // Generic: K1, Switch: B, Xbox: A
            | (sc.digitalPressed(PCE_1) ? GAMEPAD_MASK_B2 : 0) // Generic: K2, Switch: A, Xbox: B
            //| (sc.digitalPressed(PCE_5) ? GAMEPAD_MASK_B3 : 0) // Generic: P1, Switch: Y, Xbox: X
            //| (sc.digitalPressed(PCE_6) ? GAMEPAD_MASK_B4 : 0) // Generic: P2, Switch: X, Xbox: Y
            //| (sc.digitalPressed(PCE_4) ? GAMEPAD_MASK_L1 : 0) // Generic: P4, Switch: L, Xbox: LB
            //| (sc.digitalPressed(PCE_3) ? GAMEPAD_MASK_R1 : 0) // Generic: P3, Switch: R, Xbox: RB
            //| (sc.digitalPressed(L) ? GAMEPAD_MASK_L2 : 0) // Generic: K4, Switch: ZL, Xbox: LT (Digital)
            //| (sc.digitalPressed(R) ? GAMEPAD_MASK_R2 : 0) // Generic: K3, Switch: ZR, Xbox: RT (Digital)
            | (sc.digitalPressed(PCE_SELECT) ? GAMEPAD_MASK_S1 : 0) // Generic: Select, Switch: -, Xbox: View
            | (sc.digitalPressed(PCE_RUN) ? GAMEPAD_MASK_S2 : 0) // Generic: Start, Switch: +, Xbox: Menu
            //| (sc.digitalPressed(LCLICK) ? GAMEPAD_MASK_L3 : 0) // All: Left Stick Click
            //| (sc.digitalPressed(RCLICK) ? GAMEPAD_MASK_R3 : 0) // All: Right Stick Click
          ;
          
          if (sc.deviceType() == PCE_DEVICE_PAD6) {
            state[i].buttons |=
                (sc.digitalPressed(PCE_5) ? GAMEPAD_MASK_B3 : 0) // Generic: P1, Switch: Y, Xbox: X
              | (sc.digitalPressed(PCE_6) ? GAMEPAD_MASK_B4 : 0) // Generic: P2, Switch: X, Xbox: Y
              | (sc.digitalPressed(PCE_4) ? GAMEPAD_MASK_L1 : 0) // Generic: P4, Switch: L, Xbox: LB
              | (sc.digitalPressed(PCE_3) ? GAMEPAD_MASK_R1 : 0) // Generic: P3, Switch: R, Xbox: RB
            ;
          }

          #ifdef ENABLE_REFLEX_PAD
            //Only used if not in multitap mode
            if (totalUsb == 2 && inputPort < 2) {
              const PceDeviceType_Enum padType = sc.deviceType();
              loopPadDisplayChars(inputPort, padType, &sc, false);
            }
          #endif
        }
      }
      
      #ifdef ENABLE_REFLEX_PAD
        for (uint8_t i = 0; i < 2; ++i) {
          //Only used if not in multitap mode
          if(totalUsb == 2 && lastPadType[i] != currentPadType[i] && currentPadType[i] == PCE_DEVICE_NONE)
            ShowDefaultPadPce(i, PCE_DEVICE_NONE);
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
    
      /*
      const uint8_t multitapPorts = 0;//pce1.getMultitapPorts() + pce2.getMultitapPorts();
      if (multitapPorts != 0) //todo implement multitap support
        sleepTime = (joyCount + 1) * 500;
      else
        sleepTime = 50;
      */
      
      return stateChanged; //joyCount != 0;
    }//end read

};
