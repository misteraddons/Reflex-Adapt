/*******************************************************************************
 * Jaguar input module for RetroZord / Reflex-Adapt.
 * By Matheus Fraguas (sonik-br)
 * 
 * Handles a single input port.
 * 
 * Works with digital pad.
 *
 * Uses JaguarLib
 * https://github.com/sonik-br/JaguarLib
 *
 * Uses a modified version of MPG
 * https://github.com/sonik-br/MPG
 *
*/

#include "RZInputModule.h"
#include "src/JaguarLib/JaguarLib.h"

//Jaguar pins - Port 1
#define JAGPIN_J3_J4   13 //h1  // ORIGINALLY PIN 9
#define JAGPIN_J2_J5   10 //h2   // ORIGINALLY PIN 8
#define JAGPIN_J1_J6   7 //h3
#define JAGPIN_J0_J7   6 //h4
#define JAGPIN_B0_B2   5 //h5
#define JAGPIN_B1_B3   4 //h6
#define JAGPIN_J11_J15 9 //h7  // ORIGINALLY PIN 13
#define JAGPIN_J10_J14 8 //h8 PS1ACK  // ORIGINALLY PIN 10
#define JAGPIN_J9_J13  22 //h9 PS1GUN
#define JAGPIN_J8_J12  14 //12 PS1DATA

class ReflexInputJaguar : public RZInputModule {
  private:
    JagPort<JAGPIN_J3_J4, JAGPIN_J2_J5, JAGPIN_J1_J6, JAGPIN_J0_J7, JAGPIN_B0_B2, JAGPIN_B1_B3, JAGPIN_J11_J15, JAGPIN_J10_J14, JAGPIN_J9_J13, JAGPIN_J8_J12> jag1;

    #ifdef ENABLE_REFLEX_PAD
      const Pad padJaguar[21] = {
        { JAG_PAD_UP,    0, (1+4)*6, UP_ON, UP_OFF },
        { JAG_PAD_DOWN,  2, (1+4)*6, DOWN_ON, DOWN_OFF },
        { JAG_PAD_LEFT,  1, (0+4)*6, LEFT_ON, LEFT_OFF },
        { JAG_PAD_RIGHT, 1, (2+4)*6, RIGHT_ON, RIGHT_OFF },
        { JAG_B,         (3-2), (8+7)*6, FACEBTN_ON, FACEBTN_OFF },
        { JAG_A,         (3-3), (9+7)*6, FACEBTN_ON, FACEBTN_OFF },
        { JAG_C,         (3-1), (7+7)*6, FACEBTN_ON, FACEBTN_OFF },
        { JAG_PAUSE,     2-2, (4.5+5)*6, DASHBTN_ON, DASHBTN_OFF },
        { JAG_OPTION,    2-2, (5.5+5)*6, DASHBTN_ON, DASHBTN_OFF },
        { JAG_1,         1, (12-3)*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
        { JAG_2,         1, (13-3)*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
        { JAG_3,         1, (14-3)*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
        { JAG_4,         2, (12-3)*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },//4 L
        { JAG_5,         2, (13-3)*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
        { JAG_6,         2, (14-3)*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },//6 R
        { JAG_7,         3, (12-3)*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },//Z 7
        { JAG_8,         3, (13-3)*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },//Y 8
        { JAG_9,         3, (14-3)*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },//X 9
        { JAG_STAR,      4, (12-3)*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
        { JAG_0,         4, (13-3)*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
        { JAG_HASH,      4, (14-3)*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF }
      };
    
      void loopPadDisplayChars(const uint8_t index, const JagDeviceType_Enum padType, const void* sc, const bool force) {
        for(uint8_t i = 0; i < (sizeof(padJaguar) / sizeof(Pad)); ++i){
          const Pad pad = padJaguar[i];
          PrintPadChar(index, padDivision[index].firstCol, pad.col, pad.row, pad.padvalue, !sc || static_cast<const JagController*>(sc)->digitalPressed(static_cast<JagDigital_Enum>(pad.padvalue)), pad.on, pad.off, force);
        }
      }
    
      void ShowDefaultPadJaguar(const uint8_t index, const JagDeviceType_Enum padType) {
        display.clear(padDivision[index].firstCol, padDivision[index].lastCol, oledDisplayFirstRow + 1, 7);
        display.setCursor(padDivision[index].firstCol, 7);
        
        switch(padType) {
          case JAG_DEVICE_PAD:
            display.print(PSTR_TO_F(PSTR_DIGITAL));
            break;
          default:
            display.print(PSTR_TO_F(PSTR_NONE));
            return;
        }
      
        if (index < 2) {
          loopPadDisplayChars(index, padType, NULL, true);
        }
      }
    #endif

  public:
    ReflexInputJaguar() : RZInputModule() { }

    const char* getUsbId() override {
      static const char* usbId { "RZMJag" };
      return usbId;
    }

    const uint16_t getUsbVersion() override {
      return MODE_ID_JAGUAR;
    }

//    #ifdef ENABLE_REFLEX_PAD
//      void displayInputName() override {
//        display.setCol(7*6);
//        display.print(F("JAGUAR"));
//      }
//    #endif

    void setup() override {
      //Init the jaguar class
      jag1.begin();
      
      delay(50);
    }

    void setup2() override { }

    bool read() override {
      static uint8_t lastControllerCount = 0;
      #ifdef ENABLE_REFLEX_PAD
        static JagDeviceType_Enum lastPadType[] = { JAG_DEVICE_NOTSUPPORTED };
        JagDeviceType_Enum currentPadType[] = { JAG_DEVICE_NONE };
      #endif
      bool stateChanged = false;
      
      //Read each jaguar port
      jag1.update();
    
      //Get the number of connected controllers on each port
      const uint8_t joyCount = jag1.getControllerCount();
      
      for (uint8_t i = 0; i < joyCount; ++i) {
        if (i == totalUsb)
          break;
        
        //Get the data for the specific controller from a port
        const uint8_t inputPort = 0;
        const JagController& sc = jag1.getJagController(i);
    
        #ifdef ENABLE_REFLEX_PAD
          currentPadType[inputPort] = sc.deviceType();
        #endif
        
        //Only process data if state changed from previous read
        if (sc.stateChanged()) {
          stateChanged = true;
          
          //Controller just connected. Also can happen when changing mode on 6btn pad
          if (sc.deviceJustChanged()) {
            resetState(i);
            #ifdef ENABLE_REFLEX_PAD
              ShowDefaultPadJaguar(inputPort, sc.deviceType());
            #endif
          }
          
          state[i].dpad = 0
            | (sc.digitalPressed(JAG_PAD_UP)    ? GAMEPAD_MASK_UP    : 0)
            | (sc.digitalPressed(JAG_PAD_DOWN)  ? GAMEPAD_MASK_DOWN  : 0)
            | (sc.digitalPressed(JAG_PAD_LEFT)  ? GAMEPAD_MASK_LEFT  : 0)
            | (sc.digitalPressed(JAG_PAD_RIGHT) ? GAMEPAD_MASK_RIGHT : 0)
          ;

          state[i].buttons = 0
            | (sc.digitalPressed(JAG_B) ? GAMEPAD_MASK_B1 : 0) // Generic: K1, Switch: B, Xbox: A
            | (sc.digitalPressed(JAG_A) ? GAMEPAD_MASK_B2 : 0) // Generic: K2, Switch: A, Xbox: B
            | (sc.digitalPressed(JAG_C) ? GAMEPAD_MASK_B3 : 0) // Generic: P1, Switch: Y, Xbox: X
            | (sc.digitalPressed(JAG_8) ? GAMEPAD_MASK_B4 : 0) // Generic: P2, Switch: X, Xbox: Y
            | (sc.digitalPressed(JAG_7) ? GAMEPAD_MASK_L1 : 0) // Generic: P4, Switch: L, Xbox: LB
            | (sc.digitalPressed(JAG_9) ? GAMEPAD_MASK_R1 : 0) // Generic: P3, Switch: R, Xbox: RB
            | (sc.digitalPressed(JAG_4) ? GAMEPAD_MASK_L2 : 0) // Generic: K4, Switch: ZL, Xbox: LT (Digital)
            | (sc.digitalPressed(JAG_6) ? GAMEPAD_MASK_R2 : 0) // Generic: K3, Switch: ZR, Xbox: RT (Digital)
            | (sc.digitalPressed(JAG_OPTION) ? GAMEPAD_MASK_S1 : 0) // Generic: Select, Switch: -, Xbox: View
            | (sc.digitalPressed(JAG_PAUSE) ? GAMEPAD_MASK_S2 : 0) // Generic: Start, Switch: +, Xbox: Menu
            | (sc.digitalPressed(JAG_1) ? GAMEPAD_MASK_L3 : 0) // All: Left Stick Click
            | (sc.digitalPressed(JAG_3) ? GAMEPAD_MASK_R3 : 0) // All: Right Stick Click
            | (sc.digitalPressed(JAG_5) ? GAMEPAD_MASK_A1 : 0) // Switch: Home, Xbox: Guide
            //| (sc.digitalPressed(JAG_0) ? GAMEPAD_MASK_A2 : 0) // Switch: Capture
          ;

          //Extra buttons. Used only on hid mode
          state[i].aux = 0
            | (sc.digitalPressed(JAG_0)    ? (1 << 0) : 0)
            | (sc.digitalPressed(JAG_2)    ? (1 << 1) : 0)
            | (sc.digitalPressed(JAG_STAR) ? (1 << 2) : 0)
            | (sc.digitalPressed(JAG_HASH) ? (1 << 3) : 0)
          ;
          
          #ifdef ENABLE_REFLEX_PAD
            if (inputPort < 2) {
              const JagDeviceType_Enum padType = sc.deviceType();
              loopPadDisplayChars(inputPort, padType, &sc, false);
            }
          #endif
        }
      }
      
      #ifdef ENABLE_REFLEX_PAD
        for (uint8_t i = 0; i < 1; ++i) {
          if(lastPadType[i] != currentPadType[i] && currentPadType[i] == JAG_DEVICE_NONE)
            ShowDefaultPadJaguar(i, JAG_DEVICE_NONE);
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
    
      return stateChanged; //joyCount != 0;
    }//end read

};
