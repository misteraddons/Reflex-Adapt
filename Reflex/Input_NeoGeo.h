/*******************************************************************************
 * NeoGeo input module for RetroZord / Reflex-Adapt.
 * By Matheus Fraguas (sonik-br)
 * 
 * Handles 1 input port.
 * 
 * Uses DigitalIO
 * https://github.com/greiman/DigitalIO
 *
 * Uses a modified version of MPG
 * https://github.com/sonik-br/MPG
 *
*/

#include "RZInputModule.h"
#include "src/DigitalIO/DigitalIO.h"

//Neogeo pins
//https://old.pinouts.ru/Game/NeoGeoJoystick_pinout.shtml
#define NEOGEOPIN_UP     15
#define NEOGEOPIN_DOWN   5
#define NEOGEOPIN_LEFT   13
#define NEOGEOPIN_RIGHT  6
#define NEOGEOPIN_A      16
#define NEOGEOPIN_B      7
#define NEOGEOPIN_C      4
#define NEOGEOPIN_D      8
#define NEOGEOPIN_SELECT 9
#define NEOGEOPIN_START  14

#define NEOGEO_TOTAL_PINS 10

class ReflexInputNeoGeo : public RZInputModule {
  private:
    DigitalPin<NEOGEOPIN_UP>     ngeo_U;
    DigitalPin<NEOGEOPIN_DOWN>   ngeo_D;
    DigitalPin<NEOGEOPIN_LEFT>   ngeo_L;
    DigitalPin<NEOGEOPIN_RIGHT>  ngeo_R;
    DigitalPin<NEOGEOPIN_A>      ngeo_BA;
    DigitalPin<NEOGEOPIN_B>      ngeo_BB;
    DigitalPin<NEOGEOPIN_C>      ngeo_BC;
    DigitalPin<NEOGEOPIN_D>      ngeo_BD;
    DigitalPin<NEOGEOPIN_SELECT> ngeo_SELECT;
    DigitalPin<NEOGEOPIN_START>  ngeo_START;

    #ifdef ENABLE_REFLEX_PAD
      const Pad padNeoGeo[NEOGEO_TOTAL_PINS] = {
        { 0x0001, 1, 1*6, UP_ON, UP_OFF },//UP
        { 0x0002, 3, 1*6, DOWN_ON, DOWN_OFF },//DOWN
        { 0x0004, 2, 0,   LEFT_ON, LEFT_OFF },//LEFT
        { 0x0008, 2, 2*6, RIGHT_ON, RIGHT_OFF },//RGHT
        { 0x0010, 3, 7*6, FACEBTN_ON, FACEBTN_OFF },//A
        { 0x0020, 2, 8*6, FACEBTN_ON, FACEBTN_OFF },//B
        { 0x0040, 2, 6*6, FACEBTN_ON, FACEBTN_OFF },//C
        { 0x0080, 1, 7*6, FACEBTN_ON, FACEBTN_OFF },//D
        { 0x0100, 2, 4*6, DASHBTN_ON, DASHBTN_OFF },//SELECT
        { 0x0200, 3, 4*6, DASHBTN_ON, DASHBTN_OFF }//START
      };
      
      void ShowDefaultPadNeoGeo() {
        const uint8_t index = 0;
        const uint8_t firstCol = 35;//padDivision[index].firstCol;
        const uint8_t lastCol = 127;//padDivision[index].lastCol;
    
        display.clear(padDivision[index].firstCol, padDivision[index].lastCol, oledDisplayFirstRow + 1, 7);
        display.setCursor(padDivision[index].firstCol, 7);
      
        for(uint8_t i = 0; i < NEOGEO_TOTAL_PINS; ++i){
          const Pad pad = padNeoGeo[i];
          PrintPadChar(index, firstCol, pad.col, pad.row, pad.padvalue, false, pad.on, pad.off, true);
        }
      }
    #endif

  public:
    ReflexInputNeoGeo() : RZInputModule() { }

    const char* getUsbId() override {
      static const char* usbId { "RZMNeoGeo" };
      return usbId;
    }

    const uint16_t getUsbVersion() override {
      return MODE_ID_NEOGEO;
    }

//    #ifdef ENABLE_REFLEX_PAD
//      void displayInputName() override {
//        display.setCol(7*6);
//        display.print(F("NEO-GEO"));
//      }
//    #endif

    void setup() override {
      //init input pins with pull-up
      ngeo_U.config(INPUT, HIGH);
      ngeo_D.config(INPUT, HIGH);
      ngeo_L.config(INPUT, HIGH);
      ngeo_R.config(INPUT, HIGH);
      ngeo_BA.config(INPUT, HIGH);
      ngeo_BB.config(INPUT, HIGH);
      ngeo_BC.config(INPUT, HIGH);
      ngeo_BD.config(INPUT, HIGH);
      ngeo_SELECT.config(INPUT, HIGH);
      ngeo_START.config(INPUT, HIGH);
      
      sleepTime = 0;
      
      delay(20);
    }

    void setup2() override { }

    bool read() override {
      static uint16_t lastState = 0x03FF;
      bool stateChanged = false;
      
      #ifdef ENABLE_REFLEX_PAD
      static bool firstTime = true;
      if(firstTime) {
        firstTime = false;
        ShowDefaultPadNeoGeo();
      }
      #endif
    
      //Read the neogeo port
      const uint16_t portState = (
          (ngeo_START << 9)
        | (ngeo_SELECT << 8)
        | (ngeo_BD << 7)
        | (ngeo_BC << 6)
        | (ngeo_BB << 5)
        | (ngeo_BA << 4)
        | (ngeo_R << 3)
        | (ngeo_L << 2)
        | (ngeo_D << 1)
        | ngeo_U);
      
      //Only process data if state changed from previous read
      if (lastState != portState) {
        stateChanged = true;
        lastState = portState;

        state[0].dpad = 0
          | (~portState & 0x01 ? GAMEPAD_MASK_UP    : 0)
          | (~portState & 0x02 ? GAMEPAD_MASK_DOWN  : 0)
          | (~portState & 0x04 ? GAMEPAD_MASK_LEFT  : 0)
          | (~portState & 0x08 ? GAMEPAD_MASK_RIGHT : 0)
        ;
    
        state[0].buttons = 0
          | (~portState & 0x0010 ? GAMEPAD_MASK_B1 : 0) // Generic: K1, Switch: B, Xbox: A
          | (~portState & 0x0020 ? GAMEPAD_MASK_B2 : 0) // Generic: K2, Switch: A, Xbox: B
          | (~portState & 0x0040 ? GAMEPAD_MASK_B3 : 0) // Generic: P1, Switch: Y, Xbox: X
          | (~portState & 0x0080 ? GAMEPAD_MASK_B4 : 0) // Generic: P2, Switch: X, Xbox: Y
          //| (~portState & 0x0010 ? GAMEPAD_MASK_L1 : 0) // Generic: P4, Switch: L, Xbox: LB
          //| (~portState & 0x0010 ? GAMEPAD_MASK_R1 : 0) // Generic: P3, Switch: R, Xbox: RB
          //| (~portState & 0x0010 ? GAMEPAD_MASK_L2 : 0) // Generic: K4, Switch: ZL, Xbox: LT (Digital)
          //| (~portState & 0x0010 ? GAMEPAD_MASK_R2 : 0) // Generic: K3, Switch: ZR, Xbox: RT (Digital)
          | (~portState & 0x0100 ? GAMEPAD_MASK_S1 : 0) // Generic: Select, Switch: -, Xbox: View
          | (~portState & 0x0200 ? GAMEPAD_MASK_S2 : 0) // Generic: Start, Switch: +, Xbox: Menu
          //| (~portState & 0x0010 ? GAMEPAD_MASK_L3 : 0) // All: Left Stick Click
          //| (~portState & 0x0010 ? GAMEPAD_MASK_R3 : 0) // All: Right Stick Click
        ;
    
        #ifdef ENABLE_REFLEX_PAD
          for(uint8_t x = 0; x < NEOGEO_TOTAL_PINS; ++x) {
            const bool isPressed = !(portState & (1 << x));
            const Pad pad = padNeoGeo[x];
            PrintPadChar(0, 35, pad.col, pad.row, pad.padvalue, isPressed, pad.on, pad.off);
          }
        #endif
      }
    
      return stateChanged;
    }//end read

};
