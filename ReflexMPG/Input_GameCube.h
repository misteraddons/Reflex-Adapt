/*******************************************************************************
 * GameCube input module for RetroZord / Reflex-Adapt.
 * By Matheus Fraguas (sonik-br)
 * 
 * Handles up to 2 input ports.
 * 
 * Works with analog pad.
 *
 * Uses Nintendo Lib
 * https://github.com/NicoHood/Nintendo
 *
 * Uses a modified version of MPG
 * https://github.com/sonik-br/MPG
 *
*/

#include "RZInputModule.h"
#include "src/NicoHoodNintendo/Nintendo.h"

//GameCube pins - Port 1
#define GC_1_DATA 21 // 9 old, 21 new

//GameCube pins - Port 2
#define GC_2_DATA 5 // 20 old, 5 new

class ReflexInputGameCube : public RZInputModule {
  private:
    CGamecubeController* gclist[2] = {
      new CGamecubeController(GC_1_DATA),
      new CGamecubeController(GC_2_DATA)
    };

    #ifdef GC_ANALOG_MAX //fixed range
      const uint8_t gc_min = 255-GC_ANALOG_MAX;
      const uint8_t gc_max = GC_ANALOG_MAX;
    #endif

    #ifdef ENABLE_REFLEX_PAD
    enum GCPadButton {
        GCBTN_START   = 1 << 12,
        GCBTN_X       = 1 << 11,
        GCBTN_Y       = 1 << 10,    
        GCBTN_B       = 1 << 9,
        GCBTN_A       = 1 << 8,
        /* Unused   = 1 << 7, */
        GCBTN_L       = 1 << 6,
        GCBTN_R       = 1 << 5,
        GCBTN_Z       = 1 << 4,
        GCBTN_UP      = 1 << 3,
        GCBTN_DOWN    = 1 << 2,
        GCBTN_RIGHT   = 1 << 1,
        GCBTN_LEFT    = 1 << 0
      };
      const Pad padGC[12] = {
        { (uint32_t)GCBTN_UP,    1, 1*6, UP_ON, UP_OFF },
        { (uint32_t)GCBTN_DOWN,  3, 1*6, DOWN_ON, DOWN_OFF },
        { (uint32_t)GCBTN_LEFT,  2, 0,   LEFT_ON, LEFT_OFF },
        { (uint32_t)GCBTN_RIGHT, 2, 2*6, RIGHT_ON, RIGHT_OFF },
        { (uint32_t)GCBTN_START, 2, 4*6, FACEBTN_ON, FACEBTN_OFF },
        { (uint32_t)GCBTN_Z,     0, 7*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
        { (uint32_t)GCBTN_X,     2, 8*6, FACEBTN_ON, FACEBTN_OFF },
        { (uint32_t)GCBTN_A,     3, 8*6, FACEBTN_ON, FACEBTN_OFF },
        { (uint32_t)GCBTN_B,     3, 7*6, FACEBTN_ON, FACEBTN_OFF },
        { (uint32_t)GCBTN_Y,     3, 9*6, FACEBTN_ON, FACEBTN_OFF },
        { (uint32_t)GCBTN_R,     0, 8*6, SHOULDERBTN_ON, SHOULDERBTN_OFF },
        { (uint32_t)GCBTN_L,     0, 1*6, SHOULDERBTN_ON, SHOULDERBTN_OFF }
      };
    
      void showDefaultPadGameCube(const uint8_t index, const bool haveController) {
        display.clear(padDivision[index].firstCol, padDivision[index].lastCol, oledDisplayFirstRow + 1, 7);
        display.setCursor(padDivision[index].firstCol, 7);
        
        switch(haveController) {
          case true:
            display.print(PSTR_TO_F(PSTR_PAD));
            break;
          default:
            display.print(PSTR_TO_F(PSTR_NONE));
            return;
        }
      
        if (index < 2) {
          for(uint8_t x = 0; x < (sizeof(padGC) / sizeof(Pad)); ++x){
            const Pad pad = padGC[x];
            PrintPadChar(index, padDivision[index].firstCol, pad.col, pad.row, pad.padvalue, true, pad.on, pad.off, true);
          }
        }
      }
    #endif
    
  public:

    ReflexInputGameCube() : RZInputModule() { }

    const char* getUsbId() override {
      static const char* usbId { "RZMNGC" };
      return usbId;
    }

    const uint16_t getUsbVersion() override {
      return MODE_ID_GC;
    }

//    #ifdef ENABLE_REFLEX_PAD
//      void displayInputName() override {
//        display.setCol(6*6);
//        display.print(F("GAMECUBE"));
//      }
//    #endif

    void setup() override {
      delay(5);
    
      totalUsb = 2;
    
      for (uint8_t i = 0; i < totalUsb; ++i) {
        gclist[i]->begin();
        hasLeftAnalogStick[i] = true;
        hasRightAnalogStick[i] = true;
      }
      
      delay(50);
    }

    void setup2() override { }

    bool read() override {
      static uint8_t lastControllerCount = 0;
      static bool haveController[] = { false, false };
      static bool isEnabled[] = { false, false };
      static uint16_t oldButtons[] = { 0, 0 };
      static int8_t oldX[] = { 0, 0 };
      static int8_t oldY[] = { 0, 0 };
      static int8_t oldCX[] = { 0, 0 };
      static int8_t oldCY[] = { 0, 0 };
    
      #ifdef ENABLE_REFLEX_PAD
        static bool firstTime = true;
        if(firstTime) {
          firstTime = false;
          showDefaultPadGameCube(0, false);
        }
      #endif
    
      //uint8_t outputIndex = 0;
  
      //nothing detected yet
      if (!isEnabled[0] && !isEnabled[1]) {
        for (uint8_t i = 0; i < 2; ++i) {
          isEnabled[i] = haveController[i] || (haveController[i] = gclist[i]->read());
        }
        
        #ifdef ENABLE_REFLEX_PAD
  
        setOledDisplay(true);
  
        display.clear(0, 127, 7, 7);
        display.setRow(7);
  
        for (uint8_t i = 0; i < 2; ++i) {
          display.setCol(padDivision[i].firstCol);
          if (!isEnabled[0] && !isEnabled[1])
            display.print(PSTR_TO_F(PSTR_NONE));  
          else if (!isEnabled[i])
            display.print(PSTR_TO_F(PSTR_NA));
        }
        for (uint8_t i = 0; i < 2; ++i) {
          if(isEnabled[i])
            showDefaultPadGameCube(i, true);
        }

        #endif
      }
    
    
      bool stateChanged[] = { false, false };
      Gamecube_Data_t gcdata[2];
    
      //read all ports
      
      for (uint8_t i = 0; i < totalUsb; ++i) {
        //const CGamecubeController* cube = gclist[i];
        //isReadSuccess[i] = false;
    
        if (!isEnabled[i])
          continue;
    
        if (gclist[i]->read ()) {
          gcdata[i] = gclist[i]->getData ();
          const bool haveControllerNow = gcdata[i].status.device != NINTENDO_DEVICE_GC_NONE;

          if (options.inputMode == INPUT_MODE_XINPUT && haveControllerNow) {
            gclist[i]->setRumble(rumble[i].left_power != 0x0 || rumble[i].right_power != 0x0);
          }
    
          //controller just connected?
          if(!haveController[i] && haveControllerNow) {
            resetState(i);
            sleepTime = DEFAULT_SLEEP_TIME;
            #ifdef ENABLE_REFLEX_PAD
              showDefaultPadGameCube(i, true);
            #endif
          }
          haveController[i] = haveControllerNow;
        } else {
          //controller just removed?
          if(haveController) {
            resetState(i);
            sleepTime = 50000;
            #ifdef ENABLE_REFLEX_PAD
              showDefaultPadGameCube(i, false);
            #endif
          }
          haveController[i] = false;
        }
      }
    
      for (uint8_t i = 0; i < totalUsb; ++i) {
        if (haveController[i]) {
          //controller read sucess
      
          const uint16_t digitalData = (gcdata[i].report.buttons0 << 8) | (gcdata[i].report.buttons1);
          const bool buttonsChanged = digitalData != oldButtons[i];
          const bool analogChanged = gcdata[i].report.xAxis != oldX[i] || gcdata[i].report.yAxis != oldY[i] || gcdata[i].report.cxAxis != oldCX[i] || gcdata[i].report.cyAxis != oldCY[i];
    
          if (buttonsChanged || analogChanged) { //state changed?
            stateChanged[i] = buttonsChanged;
            hasAnalogTriggers[i] = false;

            state[i].dpad = 0
              | (gcdata[i].report.dup    ? GAMEPAD_MASK_UP    : 0)
              | (gcdata[i].report.ddown  ? GAMEPAD_MASK_DOWN  : 0)
              | (gcdata[i].report.dleft  ? GAMEPAD_MASK_LEFT  : 0)
              | (gcdata[i].report.dright ? GAMEPAD_MASK_RIGHT : 0)
            ;

            state[i].buttons = 0
              | (gcdata[i].report.b ? GAMEPAD_MASK_B1 : 0) // Generic: K1, Switch: B, Xbox: A
              | (gcdata[i].report.a ? GAMEPAD_MASK_B2 : 0) // Generic: K2, Switch: A, Xbox: B
              | (gcdata[i].report.y ? GAMEPAD_MASK_B3 : 0) // Generic: P1, Switch: Y, Xbox: X
              | (gcdata[i].report.x ? GAMEPAD_MASK_B4 : 0) // Generic: P2, Switch: X, Xbox: Y
              //| (L      ? GAMEPAD_MASK_L1 : 0) // Generic: P4, Switch: L, Xbox: LB
              | (gcdata[i].report.z      ? GAMEPAD_MASK_R1 : 0) // Generic: P3, Switch: R, Xbox: RB
              | (gcdata[i].report.left  > 50      ? GAMEPAD_MASK_L2 : 0) // Generic: K4, Switch: ZL, Xbox: LT (Digital)
              | (gcdata[i].report.right > 50      ? GAMEPAD_MASK_R2 : 0) // Generic: K3, Switch: ZR, Xbox: RT (Digital)
              //| (gcdata[i].report.z     ? GAMEPAD_MASK_S1 : 0) // Generic: Select, Switch: -, Xbox: View
              | (gcdata[i].report.start ? GAMEPAD_MASK_S2 : 0) // Generic: Start, Switch: +, Xbox: Menu
              //| (sc.digitalPressed(LCLICK) ? GAMEPAD_MASK_L3 : 0) // All: Left Stick Click
              //| (sc.digitalPressed(RCLICK) ? GAMEPAD_MASK_R3 : 0) // All: Right Stick Click
            ;

            //Enable analog trigger?
            if (canUseAnalogTrigger()) {
              //todo: map min/max value
              hasAnalogTriggers[i] = true;
              
              uint8_t lt = static_cast<uint8_t>(gcdata[i].report.left);  //Xbox: LT
              uint8_t rt = static_cast<uint8_t>(gcdata[i].report.right); //Xbox: RT

              if (lt < 32) lt = 0;
              if (rt < 32) rt = 0;
              lt = gcdata[i].report.l ? 0xFF : lt;
              rt = gcdata[i].report.r ? 0xFF : rt;
              state[i].lt = lt;
              state[i].rt = rt;
            }


            //how to handle analog range?
            #ifdef GC_ANALOG_MAX
              //limit it's range
              if(gcdata[i].report.xAxis < gc_min)
                gcdata[i].report.xAxis = gc_min;
              else if(gcdata[i].report.xAxis > gc_max)
                gcdata[i].report.xAxis = gc_max;
              if(gcdata[i].report.yAxis < gc_min)
                gcdata[i].report.yAxis = gc_min;
              else if(gcdata[i].report.yAxis > gc_max)
                gcdata[i].report.yAxis = gc_max;
    
              if(gcdata[i].report.cxAxis < gc_min)
                gcdata[i].report.cxAxis = gc_min;
              else if(gcdata[i].report.cxAxis > gc_max)
                gcdata[i].report.cxAxis = gc_max;
              if(gcdata[i].report.cyAxis < gc_min)
                gcdata[i].report.cyAxis = gc_min;
              else if(gcdata[i].report.cyAxis > gc_max)
                gcdata[i].report.cyAxis = gc_max;
                
              state[i].lx = convertAnalog(static_cast<uint8_t>(map(gcdata[i].report.xAxis,  gc_min, gc_max, 0, 255)));
              state[i].ly = convertAnalog(static_cast<uint8_t>(map(gcdata[i].report.yAxis,  gc_max, gc_min, 0, 255)));
              state[i].rx = convertAnalog(static_cast<uint8_t>(map(gcdata[i].report.cxAxis, gc_min, gc_max, 0, 255)));
              state[i].ry = convertAnalog(static_cast<uint8_t>(map(gcdata[i].report.cyAxis, gc_max, gc_min, 0, 255)));
            #else //use raw value
              state[i].lx = convertAnalog(static_cast<uint8_t>( gcdata[i].report.xAxis));
              state[i].ly = convertAnalog(static_cast<uint8_t>(~gcdata[i].report.yAxis));
              state[i].rx = convertAnalog(static_cast<uint8_t>( gcdata[i].report.cxAxis));
              state[i].ry = convertAnalog(static_cast<uint8_t>(~gcdata[i].report.cyAxis));
            #endif
      
            #ifdef ENABLE_REFLEX_PAD
              //const uint8_t startCol = inputPort == 0 ? 0 : 11*6;
              for(uint8_t x = 0; x < (sizeof(padGC) / sizeof(Pad)); ++x){
                const Pad pad = padGC[x];
                PrintPadChar(i, padDivision[i].firstCol, pad.col, pad.row, pad.padvalue, digitalData & pad.padvalue, pad.on, pad.off);
              }
            #endif
      
            //keep values
            oldButtons[i] = digitalData;
            oldX[i] = gcdata[i].report.xAxis;
            oldY[i] = gcdata[i].report.yAxis;
            oldCX[i] = gcdata[i].report.cxAxis;
            oldCY[i] = gcdata[i].report.cyAxis;
          }//end if statechanged
        }//end havecontroller
      }//end for
    
      return stateChanged[0] || stateChanged[1]; //joyCount != 0;
    }//end read

};
