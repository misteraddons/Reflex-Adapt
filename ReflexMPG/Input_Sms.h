/*******************************************************************************
 * Master System and Japanese-PC input module for RetroZord / Reflex-Adapt.
 * By Matheus Fraguas (sonik-br)
 * 
 * Handles up to 2 input ports.
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

//SMS pins - Port 1
#define SMS1_TH 4
#define SMS1_TR 13
#define SMS1_TL 5
#define SMS1_U  9
#define SMS1_D  8
#define SMS1_L  7
#define SMS1_R  6

//SMS pins - Port 2
#define SMS2_TH 16
#define SMS2_TR 10
#define SMS2_TL 14
#define SMS2_U  20
#define SMS2_D  19
#define SMS2_L  18
#define SMS2_R  15

class ReflexInputSms : public RZInputModule {
  private:
    DigitalPin<SMS1_U>  sms1_U;
    DigitalPin<SMS1_D>  sms1_D;
    DigitalPin<SMS1_L>  sms1_L;
    DigitalPin<SMS1_R>  sms1_R;
    DigitalPin<SMS1_TL> sms1_TL;
    DigitalPin<SMS1_TR> sms1_TR;
    DigitalPin<SMS1_TH> sms1_TH;

    DigitalPin<SMS2_U>  sms2_U;
    DigitalPin<SMS2_D>  sms2_D;
    DigitalPin<SMS2_L>  sms2_L;
    DigitalPin<SMS2_R>  sms2_R;
    DigitalPin<SMS2_TL> sms2_TL;
    DigitalPin<SMS2_TR> sms2_TR;
    DigitalPin<SMS2_TH> sms2_TH;

    bool isJPC;

    uint8_t getSmsData(const uint8_t inputPort) {
      if (isJPC) {
        return inputPort
          ? ((sms2_TL << 5) | (sms2_TH << 4) | (sms2_R << 3) | (sms2_L << 2) | (sms2_D << 1) | sms2_U)
          : ((sms1_TL << 5) | (sms1_TH << 4) | (sms1_R << 3) | (sms1_L << 2) | (sms1_D << 1) | sms1_U)
        ;
      } else {
        return inputPort
          ? ((sms2_TR << 5) | (sms2_TL << 4) | (sms2_R << 3) | (sms2_L << 2) | (sms2_D << 1) | sms2_U)
          : ((sms1_TR << 5) | (sms1_TL << 4) | (sms1_R << 3) | (sms1_L << 2) | (sms1_D << 1) | sms1_U)
        ;
      }
    }

    #ifdef ENABLE_REFLEX_PAD
      const Pad padSms[8] = {
        { 0x0001, 1, 1*6, UP_ON, UP_OFF },           //Up
        { 0x0002, 3, 1*6, DOWN_ON, DOWN_OFF },       //Down
        { 0x0004, 2, 0,   LEFT_ON, LEFT_OFF },       //Left
        { 0x0008, 2, 2*6, RIGHT_ON, RIGHT_OFF },     //Right
        { 0x0010, 3, 7*6, FACEBTN_ON, FACEBTN_OFF }, //A
        { 0x0020, 3, 8*6, FACEBTN_ON, FACEBTN_OFF }, //B
        //FM Towns Marty specific
        { 0x0040, 2, 4*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF }, //Select
        { 0x0080, 2, 5*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF }  //Run
      };
      
      void ShowDefaultPadSms(const uint8_t index) {
        const uint8_t firstCol = padDivision[index].firstCol;  
        display.clear(firstCol, padDivision[index].lastCol, oledDisplayFirstRow + 1, 7);
        display.setCursor(firstCol, 7);

        const uint8_t total = isJPC ? 8 : 6;
        for(uint8_t x = 0; x < total; ++x){
          const Pad pad = padSms[x];
          PrintPadChar(index, firstCol, pad.col, pad.row, pad.padvalue, false, pad.on, pad.off, true);
        }
      }
    #endif

  public:
    ReflexInputSms() : RZInputModule() { }

    const char* getUsbId() override {
      static const char* usbId1 { "RZMSMS" };
      static const char* usbId2 { "RZMJPC" };
      return isJPC ? usbId2 : usbId1;
    }

    const uint16_t getUsbVersion() override {
      return isJPC ? MODE_ID_JPC : MODE_ID_SMS;
    }

//    #ifdef ENABLE_REFLEX_PAD
//      void displayInputName() override {
//        display.setCol(7*6);
//        display.print(F("NEO-GEO"));
//      }
//    #endif

    void setup() override {
      //init input pins with pull-up
      sms1_U.config(INPUT, HIGH);
      sms1_D.config(INPUT, HIGH);
      sms1_L.config(INPUT, HIGH);
      sms1_R.config(INPUT, HIGH);
      sms1_TL.config(INPUT, HIGH);
      //sms1_TR.config(INPUT, HIGH);
      //sms1_TH.config(INPUT, HIGH);

      sms2_U.config(INPUT, HIGH);
      sms2_D.config(INPUT, HIGH);
      sms2_L.config(INPUT, HIGH);
      sms2_R.config(INPUT, HIGH);
      sms2_TL.config(INPUT, HIGH);
      //sms2_TR.config(INPUT, HIGH);
      //sms2_TH.config(INPUT, HIGH);

      isJPC = false;
      #ifdef ENABLE_REFLEX_JPC
        isJPC = deviceMode == RZORD_JPC;
      #endif

      if (isJPC) {
        sms1_TH.config(INPUT, HIGH);
        sms2_TH.config(INPUT, HIGH);
        sms1_TR.config(OUTPUT, LOW);
        sms2_TR.config(OUTPUT, LOW);
      } else {
        sms1_TR.config(INPUT, HIGH);
        sms2_TR.config(INPUT, HIGH);    
      }
      
      sleepTime = 50;
      totalUsb = 2;
      
      delay(20);
    }

    void setup2() override { }

    bool read() override {
      static uint8_t lastState[] = { 0x3F, 0x3F };
      bool stateChanged = false;

      #ifdef ENABLE_REFLEX_PAD
        static bool firstTime = true;
        if(firstTime) {
          firstTime = false;
          for(uint8_t i = 0; i < 2; ++i) {
            ShowDefaultPadSms(i);
          }
        }
      #endif

      for (uint8_t inputPort = 0; inputPort < totalUsb; ++inputPort) {
        //Read the sms port
        uint8_t portState = getSmsData(inputPort);

        if (isJPC) {
          bitWrite(portState, 6, portState & 0x03); //up and down = select
          bitWrite(portState, 7, portState & 0x0C); //left and right = run
        }

        //Only process data if state changed from previous read
        if (lastState[inputPort] != portState) {
          stateChanged = true;
          lastState[inputPort] = portState;

          state[0].dpad = 0
            | (~portState & 0x01 ? GAMEPAD_MASK_UP    : 0)
            | (~portState & 0x02 ? GAMEPAD_MASK_DOWN  : 0)
            | (~portState & 0x04 ? GAMEPAD_MASK_LEFT  : 0)
            | (~portState & 0x08 ? GAMEPAD_MASK_RIGHT : 0)
          ;

          state[0].buttons = 0
            | (~portState & 0x10 ? GAMEPAD_MASK_B1 : 0) // Generic: K1, Switch: B, Xbox: A
            | (~portState & 0x20 ? GAMEPAD_MASK_B2 : 0) // Generic: K2, Switch: A, Xbox: B
            //| (~portState & 0x40 ? GAMEPAD_MASK_B3 : 0) // Generic: P1, Switch: Y, Xbox: X
            //| (~portState & 0x80 ? GAMEPAD_MASK_B4 : 0) // Generic: P2, Switch: X, Xbox: Y
            //| (~portState & 0x01 ? GAMEPAD_MASK_L1 : 0) // Generic: P4, Switch: L, Xbox: LB
            //| (~portState & 0x10 ? GAMEPAD_MASK_R1 : 0) // Generic: P3, Switch: R, Xbox: RB
            //| (~portState & 0x10 ? GAMEPAD_MASK_L2 : 0) // Generic: K4, Switch: ZL, Xbox: LT (Digital)
            //| (~portState & 0x10 ? GAMEPAD_MASK_R2 : 0) // Generic: K3, Switch: ZR, Xbox: RT (Digital)
            | (isJPC && (~portState & 0x40) ? GAMEPAD_MASK_S1 : 0) // Generic: Select, Switch: -, Xbox: View
            | (isJPC && (~portState & 0x80) ? GAMEPAD_MASK_S2 : 0) // Generic: Start, Switch: +, Xbox: Menu
            //| (~portState & 0x10 ? GAMEPAD_MASK_L3 : 0) // All: Left Stick Click
            //| (~portState & 0x10 ? GAMEPAD_MASK_R3 : 0) // All: Right Stick Click
          ;

          #ifdef ENABLE_REFLEX_PAD
            const uint8_t firstCol = padDivision[inputPort].firstCol;
            const uint8_t total = isJPC ? 8 : 6;
            for(uint8_t i = 0; i < total; ++i) {
              const bool isPressed = !(portState & (1 << i));
              const Pad pad = padSms[i];
              PrintPadChar(inputPort, firstCol, pad.col, pad.row, pad.padvalue, isPressed, pad.on, pad.off);
            }
          #endif
        }
      }

      return stateChanged;
    }//end read

};
