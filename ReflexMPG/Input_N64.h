/*******************************************************************************
 * N64 input module for RetroZord / Reflex-Adapt.
 * By Matheus Fraguas (sonik-br)
 * 
 * Handles up to 2 input ports.
 * 
 * Works with analog pad.
 *
 * Uses a modified version of Nintendo Lib
 * https://github.com/NicoHood/Nintendo
 *
 * Uses a modified version of MPG
 * https://github.com/sonik-br/MPG
 *
*/

#include "RZInputModule.h"
#include "src/NicoHoodNintendo/Nintendo.h"

//N64 pins - Port 1
#define N64_1_DATA 21 // 9 old, 21 new

//N64 pins - Port 2
#define N64_2_DATA 5 // 20 old, 5 new

class ReflexInputN64 : public RZInputModule {
  private:
    CN64Controller* n64;//variable to hold current reading port

    CN64Controller* n64list[2] {
      new CN64Controller(N64_1_DATA),
      new CN64Controller(N64_2_DATA)
    };


    #ifdef N64_ANALOG_MAX
      #if N64_ANALOG_MAX == 0 //automatic adjustment. value starts at 50
        #define N64_INITIAL_ANALOG_MAX 50
        int8_t n64_x_min[2] { -N64_INITIAL_ANALOG_MAX, -N64_INITIAL_ANALOG_MAX };
        int8_t n64_x_max[2] { N64_INITIAL_ANALOG_MAX, N64_INITIAL_ANALOG_MAX };
        int8_t n64_y_min[2] { -N64_INITIAL_ANALOG_MAX, -N64_INITIAL_ANALOG_MAX };
        int8_t n64_y_max[2] { N64_INITIAL_ANALOG_MAX, N64_INITIAL_ANALOG_MAX };
      #else //fixed range
        const int8_t n64_x_min { -N64_ANALOG_MAX };
        const int8_t n64_x_max { N64_ANALOG_MAX };
        const int8_t n64_y_min { -N64_ANALOG_MAX };
        const int8_t n64_y_max { N64_ANALOG_MAX };
      #endif
    #endif

    #ifdef ENABLE_REFLEX_PAD
    enum PadButton {
        N64_BTN_A       = 1 << 15,
        N64_BTN_B       = 1 << 14,
        N64_BTN_Z       = 1 << 13,
        N64_BTN_START   = 1 << 12,
        N64_BTN_UP      = 1 << 11,
        N64_BTN_DOWN    = 1 << 10,
        N64_BTN_LEFT    = 1 << 9,
        N64_BTN_RIGHT   = 1 << 8,
        N64_BTN_LRSTART = 1 << 7, // This is set when L+R+Start are pressed (and BTN_START is not)
        /* Unused   = 1 << 6, */
        N64_BTN_L       = 1 << 5,
        N64_BTN_R       = 1 << 4,
        N64_BTN_C_UP    = 1 << 3,
        N64_BTN_C_DOWN  = 1 << 2,
        N64_BTN_C_LEFT  = 1 << 1,
        N64_BTN_C_RIGHT = 1 << 0
      };
      const Pad padN64[14] = {
        { (uint32_t)N64_BTN_UP,      1, 1*6, UP_ON, UP_OFF },
        { (uint32_t)N64_BTN_DOWN,    3, 1*6, DOWN_ON, DOWN_OFF },
        { (uint32_t)N64_BTN_LEFT,    2, 0,   LEFT_ON, LEFT_OFF },
        { (uint32_t)N64_BTN_RIGHT,   2, 2*6, RIGHT_ON, RIGHT_OFF },
        { (uint32_t)N64_BTN_START,   2, 4*6, FACEBTN_ON, FACEBTN_OFF },
        { (uint32_t)N64_BTN_Z,       3, 4*6, SHOULDERBTN_ON, SHOULDERBTN_OFF },
        { (uint32_t)N64_BTN_B,       2, 6*6-2, FACEBTN_ON, FACEBTN_OFF }, //3, 9*6
        { (uint32_t)N64_BTN_A,       3, 7*6-2, FACEBTN_ON, FACEBTN_OFF }, //3, 8*6
        { (uint32_t)N64_BTN_C_UP,    1, 8*6, UP_ON, UP_OFF },
        { (uint32_t)N64_BTN_C_DOWN,  3, 8*6, DOWN_ON, DOWN_OFF },
        { (uint32_t)N64_BTN_C_LEFT,  2, 7*6,   LEFT_ON, LEFT_OFF },
        { (uint32_t)N64_BTN_C_RIGHT, 2, 9*6, RIGHT_ON, RIGHT_OFF },
        { (uint32_t)N64_BTN_R,       0, 8*6, SHOULDERBTN_ON, SHOULDERBTN_OFF },
        { (uint32_t)N64_BTN_L,       0, 1*6, SHOULDERBTN_ON, SHOULDERBTN_OFF }
      };
    
      void showDefaultPadN64(const uint8_t index, const bool haveController) {
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
          for(uint8_t x = 0; x < (sizeof(padN64) / sizeof(Pad)); ++x){
            const Pad pad = padN64[x];
            PrintPadChar(index, padDivision[index].firstCol, pad.col, pad.row, pad.padvalue, false, pad.on, pad.off, true);
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
      resetState(i);
      n64ResetAnalogMinMax(i);
    }

  public:
    ReflexInputN64() : RZInputModule() { }

    const char* getUsbId() override {
      static const char* usbId { "RZMN64" };
      return usbId;
    }

    const uint16_t getUsbVersion() override {
      return MODE_ID_N64;
    }

//    #ifdef ENABLE_REFLEX_PAD
//      void displayInputName() override {
//        display.setCol(9*6);
//        display.print(F("N64"));
//      }
//    #endif


    void setup() override {
      delay(5);
    
      totalUsb = 2;

      for (uint8_t i = 0; i < totalUsb; ++i)
        hasLeftAnalogStick[i] = true;

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
          for (uint8_t i = 0; i < 2; ++i) {
            //isEnabled[i] = haveController[i] || (haveController[i] = n64list[i]->read());
            
            //isEnabled[i] = n64list[i]->read();
            isEnabled[i] = haveController[i] || (haveController[i] = n64list[i]->begin());
            
            if (isEnabled[0] || isEnabled[1])
              delayMicroseconds(sleepTime);
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
            else
              showDefaultPadN64(i, true);
          }
          
          #endif
        }
    
    
      bool stateChanged[] = { false, false };
      N64_Data_t n64data[2];
    
      //read all ports
      
//      for (uint8_t i = 0; i < totalUsb; ++i) {
//        n64 = n64list[i];
//        //isReadSuccess[i] = false;
//    
//        if (!isEnabled[i])
//          continue;
//          
//        if (n64->read ()) {
//          n64data[i] = n64->getData ();
//          const bool haveControllerNow = n64data[i].status.device != NINTENDO_DEVICE_N64_NONE;
//          //controller just connected?
//          if(!haveController[i] && haveControllerNow) {
//            n64ResetJoyValues(i);
//            #ifdef ENABLE_REFLEX_PAD
//              showDefaultPadN64(i, true);
//            #endif
//          }
//          haveController[i] = haveControllerNow;
//
//          if (options.inputMode == INPUT_MODE_XINPUT && haveControllerNow) {
//            n64->setRumble(rumble[i].left_power != 0x0 || rumble[i].right_power != 0x0);
//          }
//          
//        } else {
//          //controller just removed?
//          if(haveController) {
//            n64ResetJoyValues(i);
//            #ifdef ENABLE_REFLEX_PAD
//              showDefaultPadN64(i, false);
//            #endif
//          }
//          haveController[i] = false;
//        }
//      }
static bool isReadSuccess[] = {false,false};
      for (uint8_t i = 0; i < totalUsb; ++i) {
        n64 = n64list[i];
        isReadSuccess[i] = false;
  
        if (!isEnabled[i])
          continue;
        
        if (!haveController[i]) {
          if (n64->begin()) {
            haveController[i] = true;
//            tryEnableRumble();
            n64ResetJoyValues(i);
            #ifdef ENABLE_REFLEX_PAD
              showDefaultPadN64(i, true);
            #endif
          }
        } else {
//          const PsxControllerProtocol proto = psx->getProtocol();
//          if(lastProto[i] != proto)
//            tryEnableRumble();
//          lastProto[i] = proto;
//          #ifdef PSX_COMBINE_RUMBLE
//            psx->setRumble ((rumble[i].left_power | rumble[i].right_power) != 0x0, (rumble[i].left_power | rumble[i].right_power));
//          #else
//            psx->setRumble (rumble[i].right_power != 0x0, rumble[i].left_power);
//          #endif
          isReadSuccess[i] = n64->read() && n64->getData().status.device != NINTENDO_DEVICE_N64_NONE;

          if (isReadSuccess[i] && options.inputMode == INPUT_MODE_XINPUT) {
            n64->setRumble(rumble[i].left_power != 0x0 || rumble[i].right_power != 0x0);
          }
          
          //controller just removed?
          if (!isReadSuccess[i]){
            haveController[i] = false;
            n64ResetJoyValues(i);
            #ifdef ENABLE_REFLEX_PAD
              showDefaultPadN64(i, false);
            #endif
          }
        }
      }















    
      for (uint8_t i = 0; i < totalUsb; ++i) {
        if (haveController[i]) {
          //controller read sucess
      
          const uint16_t digitalData = (n64data[i].report.buttons0 << 12) + (n64data[i].report.dpad << 8) + (n64data[i].report.buttons1 << 4) + n64data[i].report.cpad;
          const bool buttonsChanged = digitalData != oldButtons[i];
          const bool analogChanged = n64data[i].report.xAxis != oldX[i] || n64data[i].report.yAxis != oldY[i];
      
          if (buttonsChanged || analogChanged) { //state changed?
            stateChanged[i] = buttonsChanged;
            
            //L+R+START internally resets the analog stick. We also need to reset it's min/max value;
            if (n64data[i].report.low0)
              n64ResetAnalogMinMax(i);

            state[i].dpad = 0
              | (n64data[i].report.dup    ? GAMEPAD_MASK_UP    : 0)
              | (n64data[i].report.ddown  ? GAMEPAD_MASK_DOWN  : 0)
              | (n64data[i].report.dleft  ? GAMEPAD_MASK_LEFT  : 0)
              | (n64data[i].report.dright ? GAMEPAD_MASK_RIGHT : 0)
            ;

            state[i].buttons = 0
              | (n64data[i].report.cdown  ? GAMEPAD_MASK_B1 : 0) // Generic: K1, Switch: B, Xbox: A
              | (n64data[i].report.cright ? GAMEPAD_MASK_B2 : 0) // Generic: K2, Switch: A, Xbox: B
              | (n64data[i].report.cleft  ? GAMEPAD_MASK_B3 : 0) // Generic: P1, Switch: Y, Xbox: X
              | (n64data[i].report.cup    ? GAMEPAD_MASK_B4 : 0) // Generic: P2, Switch: X, Xbox: Y
              | (n64data[i].report.l      ? GAMEPAD_MASK_L1 : 0) // Generic: P4, Switch: L, Xbox: LB
              | (n64data[i].report.r      ? GAMEPAD_MASK_R1 : 0) // Generic: P3, Switch: R, Xbox: RB
              | (n64data[i].report.b      ? GAMEPAD_MASK_L2 : 0) // Generic: K4, Switch: ZL, Xbox: LT (Digital)
              | (n64data[i].report.a      ? GAMEPAD_MASK_R2 : 0) // Generic: K3, Switch: ZR, Xbox: RT (Digital)
              | (n64data[i].report.z      ? GAMEPAD_MASK_S1 : 0) // Generic: Select, Switch: -, Xbox: View
              | (n64data[i].report.start  ? GAMEPAD_MASK_S2 : 0) // Generic: Start, Switch: +, Xbox: Menu
              //| (sc.digitalPressed(LCLICK) ? GAMEPAD_MASK_L3 : 0) // All: Left Stick Click
              //| (sc.digitalPressed(RCLICK) ? GAMEPAD_MASK_R3 : 0) // All: Right Stick Click
            ;
      
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
                
                state[i].lx = convertAnalog(static_cast<uint8_t>(map(n64data[i].report.xAxis, n64_x_min[i], n64_x_max[i], 0, 255)));
                state[i].ly = convertAnalog(static_cast<uint8_t>(map(n64data[i].report.yAxis, n64_y_max[i], n64_y_min[i], 0, 255)));
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
                  
                state[i].lx = convertAnalog(static_cast<uint8_t>(map(n64data[i].report.xAxis, n64_x_min, n64_x_max, 0, 255)));
                state[i].ly = convertAnalog(static_cast<uint8_t>(map(n64data[i].report.yAxis, n64_y_max, n64_y_min, 0, 255)));
              #endif
              
            #else //use raw value
              state[i].lx = convertAnalog(static_cast<uint8_t>( n64data[i].report.xAxis + 128U));
              state[i].ly = convertAnalog(static_cast<uint8_t>(~n64data[i].report.yAxis + 128U));
            #endif
      
            #ifdef ENABLE_REFLEX_PAD
              for(uint8_t x = 0; x < (sizeof(padN64) / sizeof(Pad)); ++x){
                const Pad pad = padN64[x];
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
    }//end read

};
