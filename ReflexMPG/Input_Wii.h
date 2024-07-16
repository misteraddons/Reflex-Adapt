/*******************************************************************************
 * Wii input module for RetroZord / Reflex-Adapt.
 * By Matheus Fraguas (sonik-br)
 * 
 * Handles a single input port.
 * 
 * Works with?.
 *
 * Uses NintendoExtensionCtrl Lib
 * https://github.com/dmadison/NintendoExtensionCtrl/
 * 
 * Uses SoftWire Lib
 * https://github.com/felias-fogg/SoftI2CMaster
 *
 * Uses a modified version of MPG
 * https://github.com/sonik-br/MPG
 *
*/

//Wii pins - Port 1

//SoftWire config
//Also configured in:
//NXC_Comms.h

#define SCL_PIN 5// D9/A9/PB5
#define SCL_PORT PORTB
#define SDA_PIN 4// D8/A8/PB4
#define SDA_PORT PORTB

#define I2C_FASTMODE 0
#define I2C_TIMEOUT 1000
#define I2C_PULLUP 1

#include "RZInputModule.h"
#include "src/SoftWire/SoftWire.h"
#include "src/NintendoExtensionCtrl/NintendoExtensionCtrl.h"

#define WII_ANALOG_DEFAULT_CENTER 127U

class ReflexInputWii : public RZInputModule {
  private:
    ExtensionPort* wii;
    ClassicController::Shared* wii_classic;
    Nunchuk::Shared* wii_nchuk;
    #ifdef ENABLE_WII_GUITAR
      GuitarController::Shared* wii_guitar;
    #endif

    bool needDetectAnalogTrigger;

    void wiiResetAnalogMinMax() {
      //n64_x_min = -50;
      //n64_x_max = 50;
      //n64_y_min = -50;
      //n64_y_max = 50;
    }
    
    void wiiResetJoyValues(const uint8_t i) {
      if (i >= totalUsb)
        return;
      resetState(i);
      wiiResetAnalogMinMax();
    }

    #ifdef ENABLE_REFLEX_PAD
      const Pad padWii[15] = {
        {     1,  2, 8*6, FACEBTN_ON, FACEBTN_OFF }, //Y
        {  1<<1,  3, 9*6, FACEBTN_ON, FACEBTN_OFF }, //B
        {  1<<2,  2, 10*6, FACEBTN_ON, FACEBTN_OFF }, //A
        {  1<<3,  1, 9*6, FACEBTN_ON, FACEBTN_OFF }, //X
        {  1<<4,  0, 0*6, SHOULDERBTN_ON, SHOULDERBTN_OFF }, //L
        {  1<<5,  0, 8*6, SHOULDERBTN_ON, SHOULDERBTN_OFF }, //R
        {  1<<6,  0, 2*6, SHOULDERBTN_ON, SHOULDERBTN_OFF }, //ZL
        {  1<<7,  0, 10*6, SHOULDERBTN_ON, SHOULDERBTN_OFF }, //ZR
        {  1<<8,  2, 4*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF }, //MINUS
        {  1<<9,  2, 6*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF }, //PLUS
        { 1<<10,  2, 5*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF }, //HOME
        { 1<<11,  1, 1*6, UP_ON, UP_OFF },
        { 1<<12,  3, 1*6, DOWN_ON, DOWN_OFF },
        { 1<<13,  2, 0,   LEFT_ON, LEFT_OFF },
        { 1<<14,  2, 2*6, RIGHT_ON, RIGHT_OFF }
      };
    
      void ShowDefaultPadWii(const uint8_t index, const ExtensionType padType) {
        display.clear(padDivision[index].firstCol, padDivision[index].lastCol, oledDisplayFirstRow + 1, 7);
        display.setCursor(padDivision[index].firstCol, 7);
        
        switch(padType) {
          case(ExtensionType::Nunchuk):
            display.print(F("NUNCHUK"));
            break;
          case(ExtensionType::ClassicController):
            display.print(F("CLASSIC"));
            break;
    
          //non-supported devices will still display it's name
          case(ExtensionType::GuitarController):
            display.print(F("GUITAR"));
            return;
          case(ExtensionType::DrumController):
            display.print(F("DRUM"));
            return;
          case(ExtensionType::DJTurntableController):
            display.print(F("TURNTABLE"));
            return;
          case(ExtensionType::uDrawTablet):
            display.print(F("UDRAW"));
            return;
          case(ExtensionType::DrawsomeTablet):
            display.print(F("DRAWSOME"));
            return;
          case(ExtensionType::UnknownController):
            display.print(F("NOT SUPPORTED"));
            return;
          default:
            display.print(PSTR_TO_F(PSTR_NONE));
            return;
        }

        if (index < 2) {
          if(padType == ExtensionType::Nunchuk) {
            for(uint8_t x = 0; x < 2; ++x){
              const Pad pad = padWii[x];
              PrintPadChar(index, padDivision[index].firstCol, pad.col, pad.row, pad.padvalue, true, pad.on, pad.off, true);
            }
          } else { //classic
            for(uint8_t x = 0; x < (sizeof(padWii) / sizeof(Pad)); ++x){
              const Pad pad = padWii[x];
              PrintPadChar(index, padDivision[index].firstCol, pad.col, pad.row, pad.padvalue, true, pad.on, pad.off, true);
            }
          }
        }
      }
    #endif

  public:
    ReflexInputWii() : RZInputModule() { }

    const char* getUsbId() override {
      static const char* usbId { "RZMWii" };
      return usbId;
    }

    const uint16_t getUsbVersion() override {
      return MODE_ID_WII;
    }

//    #ifdef ENABLE_REFLEX_PAD
//      void displayInputName() override {
//        display.setCol(9*6);
//        display.print(F("WII"));
//      }
//    #endif



    void setup() override {
      delay(5);
    
      //SoftWire initialization
      Wire.begin();
    
      //Will use 400khz if I2C_FASTMODE 1.
      //Wire.setClock(100000UL);
      //Wire.setClock(400000UL);
    
      wii = new ExtensionPort(Wire);
      wii_classic = new ClassicController::Shared(*wii);
      wii_nchuk = new Nunchuk::Shared(*wii);
      #ifdef ENABLE_WII_GUITAR
        wii_guitar = new GuitarController::Shared(*wii);
      #endif
    
      wii->begin();

      hasLeftAnalogStick[0] = true;
      hasRightAnalogStick[0] = true;

      needDetectAnalogTrigger = true;

      delay(50);
    }

    void setup2() override {
      needDetectAnalogTrigger = true;
    }

    bool read() override {
      static uint8_t lastControllerCount = 0;
      static boolean haveController = false;
      static bool isClassicAnalog = false;
      static uint16_t oldButtons = 0;
      static uint8_t oldHat = 0;
      static uint8_t oldLX = WII_ANALOG_DEFAULT_CENTER, oldLY = WII_ANALOG_DEFAULT_CENTER;
      static uint8_t oldRX = WII_ANALOG_DEFAULT_CENTER, oldRY = WII_ANALOG_DEFAULT_CENTER;
      static uint8_t oldLT = 0, oldRT = 0;

      #ifndef WII_ANALOG_RAW
        //Trigger L and R range
        //classic: I've seen it go from 5 to 255. More common is 5 to 245
        //classic pro: 0 or 248
        const uint8_t wii_analog_stick_min = 25;
        const uint8_t wii_analog_stick_max = 230;
        const uint8_t wii_analog_trigger_min = 14;
        static uint8_t wii_lx_shift = 0;
        static uint8_t wii_ly_shift = 0;
        static uint8_t wii_rx_shift = 0;
        static uint8_t wii_ry_shift = 0;
      #endif

     
      #ifdef ENABLE_REFLEX_PAD
        static ExtensionType lastPadType[] = { ExtensionType::UnknownController };
        ExtensionType currentPadType[] = { ExtensionType::NoController };
      #endif
    
      #ifdef ENABLE_REFLEX_PAD
        static bool firstTime = true;
        if(firstTime) {
          firstTime = false;
          ShowDefaultPadWii(0, ExtensionType::NoController);
        }
      #endif
    
      bool stateChanged = false;
    
    
      if (!haveController) {
        if (wii->connect()) {
          //controller connected
          haveController = true;
          needDetectAnalogTrigger = true;
          isClassicAnalog = false;
          wiiResetJoyValues(0);
          #ifdef ENABLE_REFLEX_PAD
            ShowDefaultPadWii(0, wii->getControllerType());
          #endif
          sleepTime = DEFAULT_SLEEP_TIME;
        } else {
          sleepTime = 50000;
        }
      } else {
        if (!wii->update()) {
          //controller removed
          haveController = false;
          hasAnalogTriggers[0] = false;
          sleepTime = 50000;
          wiiResetJoyValues(0);
          oldButtons = 0; //reset previous state
          #ifdef ENABLE_REFLEX_PAD
            ShowDefaultPadWii(0, ExtensionType::NoController);
          #endif
        } else {
          //controller read sucess
          
          uint16_t buttonData = 0;
          uint8_t hatData = 0;
          uint8_t leftX = 0;
          uint8_t leftY = 0;
          uint8_t rightX = 0;
          uint8_t rightY = 0;
          uint8_t lt = 255;
          uint8_t rt = 255;
    
          const ExtensionType conType = wii->getControllerType();
    
          //only handle data from Classic or Nunchuk
          if(conType == ExtensionType::ClassicController || conType == ExtensionType::Nunchuk
#ifdef ENABLE_WII_GUITAR
          || conType == ExtensionType::GuitarController
#endif
          ) {
    
            if(conType == ExtensionType::ClassicController) {
              bitWrite(hatData, 0, !wii_classic->dpadUp());
              bitWrite(hatData, 1, !wii_classic->dpadDown());
              bitWrite(hatData, 2, !wii_classic->dpadLeft());
              bitWrite(hatData, 3, !wii_classic->dpadRight());
              
              bitWrite(buttonData, 0, wii_classic->buttonY());
              bitWrite(buttonData, 1, wii_classic->buttonB());
              bitWrite(buttonData, 2, wii_classic->buttonA());
              bitWrite(buttonData, 3, wii_classic->buttonX());
              bitWrite(buttonData, 4, wii_classic->buttonL());
              bitWrite(buttonData, 5, wii_classic->buttonR());
              bitWrite(buttonData, 6, wii_classic->buttonZL());
              bitWrite(buttonData, 7, wii_classic->buttonZR());
              bitWrite(buttonData, 8, wii_classic->buttonMinus());
              bitWrite(buttonData, 9, wii_classic->buttonPlus());
              bitWrite(buttonData, 10, wii_classic->buttonHome());

              leftX = wii_classic->leftJoyX();
              leftY = wii_classic->leftJoyY();
              rightX = wii_classic->rightJoyX();
              rightY = wii_classic->rightJoyY();
              lt = wii_classic->triggerL();
              rt = wii_classic->triggerR();


              if (needDetectAnalogTrigger) {
                needDetectAnalogTrigger = false;

              #ifndef WII_ANALOG_RAW
                wii_lx_shift = (leftX >= 127) ? (leftX - 127) : (127 - leftX);
                wii_ly_shift = (leftY >= 127) ? (leftY - 127) : (127 - leftY);
                wii_rx_shift = (rightX >= 127) ? (rightX - 127) : (127 - rightX);
                wii_ry_shift = (rightY >= 127) ? (rightY - 127) : (127 - rightY);
              #endif


                //if any "analog" trigger is zero, we can assume it's a digital device. need to test with more controllers.
                isClassicAnalog = !(lt == 0 || rt == 0);
                if (isClassicAnalog && canUseAnalogTrigger()) {
                  hasAnalogTriggers[0] = true;
                }
              }

//              if (canUseAnalogTrigger()) {
//                hasAnalogTriggers[0] = true;
//              }

              state[0].dpad = 0
                | (wii_classic->dpadUp()    ? GAMEPAD_MASK_UP    : 0)
                | (wii_classic->dpadDown()  ? GAMEPAD_MASK_DOWN  : 0)
                | (wii_classic->dpadLeft()  ? GAMEPAD_MASK_LEFT  : 0)
                | (wii_classic->dpadRight() ? GAMEPAD_MASK_RIGHT : 0)
              ;

              state[0].buttons = 0
                | (wii_classic->buttonB()  ? GAMEPAD_MASK_B1 : 0) // Generic: K1, Switch: B, Xbox: A
                | (wii_classic->buttonA()  ? GAMEPAD_MASK_B2 : 0) // Generic: K2, Switch: A, Xbox: B
                | (wii_classic->buttonY()  ? GAMEPAD_MASK_B3 : 0) // Generic: P1, Switch: Y, Xbox: X
                | (wii_classic->buttonX()  ? GAMEPAD_MASK_B4 : 0) // Generic: P2, Switch: X, Xbox: Y
//                | (wii_classic->buttonZL() ? GAMEPAD_MASK_L1 : 0) // Generic: P4, Switch: L, Xbox: LB
//                | (wii_classic->buttonZR() ? GAMEPAD_MASK_R1 : 0) // Generic: P3, Switch: R, Xbox: RB
//                | (wii_classic->buttonL()  ? GAMEPAD_MASK_L2 : 0) // Generic: K4, Switch: ZL, Xbox: LT (Digital)
//                | (wii_classic->buttonR()  ? GAMEPAD_MASK_R2 : 0) // Generic: K3, Switch: ZR, Xbox: RT (Digital)
                | (wii_classic->buttonMinus() ? GAMEPAD_MASK_S1 : 0) // Generic: Select, Switch: -, Xbox: View
                | (wii_classic->buttonPlus() ? GAMEPAD_MASK_S2 : 0) // Generic: Start, Switch: +, Xbox: Menu
                //| (LCLICK ? GAMEPAD_MASK_L3 : 0) // All: Left Stick Click
                //| (RCLICK ? GAMEPAD_MASK_R3 : 0) // All: Right Stick Click
                | (wii_classic->buttonHome() ? GAMEPAD_MASK_A1 : 0) // Switch: Home, Xbox: Guide
              ;

              //swap L/ZL
              if (isClassicAnalog) { //classic controller with analog triggers (RVL-005)
                state[0].buttons = state[0].buttons
                  | (wii_classic->buttonZL() ? GAMEPAD_MASK_L1 : 0) // Generic: P4, Switch: L, Xbox: LB
                  | (wii_classic->buttonZR() ? GAMEPAD_MASK_R1 : 0) // Generic: P3, Switch: R, Xbox: RB
                  | (wii_classic->buttonL()  ? GAMEPAD_MASK_L2 : 0) // Generic: K4, Switch: ZL, Xbox: LT (Digital)
                  | (wii_classic->buttonR()  ? GAMEPAD_MASK_R2 : 0) // Generic: K3, Switch: ZR, Xbox: RT (Digital)
//                  | (lt >= wii_analog_trigger_min  ? GAMEPAD_MASK_L2 : 0) // Generic: K4, Switch: ZL, Xbox: LT (Digital)
//                  | (rt >= wii_analog_trigger_min  ? GAMEPAD_MASK_R2 : 0) // Generic: K3, Switch: ZR, Xbox: RT (Digital)
                ;
              } else {
                state[0].buttons = state[0].buttons
                  | (wii_classic->buttonL()  ? GAMEPAD_MASK_L1 : 0) // Generic: P4, Switch: L, Xbox: LB
                  | (wii_classic->buttonR()  ? GAMEPAD_MASK_R1 : 0) // Generic: P3, Switch: R, Xbox: RB
                  | (wii_classic->buttonZL() ? GAMEPAD_MASK_L2 : 0) // Generic: K4, Switch: ZL, Xbox: LT (Digital)
                  | (wii_classic->buttonZR() ? GAMEPAD_MASK_R2 : 0) // Generic: K3, Switch: ZR, Xbox: RT (Digital)
                ;
              }
    
#ifdef ENABLE_WII_GUITAR
            } else if (conType == ExtensionType::GuitarController) {
              bitWrite(buttonData, 0, wii_guitar->fretBlue());
              bitWrite(buttonData, 1, wii_guitar->fretRed());
              bitWrite(buttonData, 2, wii_guitar->fretGreen());
              bitWrite(buttonData, 3, wii_guitar->fretYellow());
              bitWrite(buttonData, 4, wii_guitar->fretOrange());
              
              bitWrite(buttonData, 8, wii_classic->buttonMinus());
              bitWrite(buttonData, 9, wii_classic->buttonPlus());
    
              bitWrite(hatData, 0, !wii_guitar->strumUp());
              bitWrite(hatData, 1, !wii_guitar->strumDown());
              bitWrite(hatData, 2, 1);
              bitWrite(hatData, 3, 1);
    
              if (wii_guitar->supportsTouchbar()) {
                //uint8_t tbar = wii_guitar->touchbar();
                bitWrite(buttonData, 5, wii_guitar->touchBlue());
                bitWrite(buttonData, 6, wii_guitar->touchRed());
                bitWrite(buttonData, 7, wii_guitar->touchGreen());
                bitWrite(buttonData, 10, wii_guitar->touchYellow());
                bitWrite(buttonData, 11, wii_guitar->touchOrange());
              }
    
              //analog stick (0-63)
              leftX = map(wii_guitar->joyX(), 0, 63, 0, 255);
              leftY = map(wii_guitar->joyY(), 0, 63, 0, 255);
    
              //whammy bar (0-31, starting around 15-16)
              //on my GH:WoR guitar it goes from 15 to 25
              //rightX = map(wii_guitar->whammyBar(), 0, 31, 0, 255);
              //rightX = map(wii_guitar->whammyBar(), 15, 31, 0, 255);
              rightX = map(wii_guitar->whammyBar(), 15, 25, 0, 255);
#endif //ENABLE_WII_GUITAR
    
            } else { //nunchuk
              bitWrite(buttonData, 0, wii_nchuk->buttonC());
              bitWrite(buttonData, 1, wii_nchuk->buttonZ());
              state[0].buttons = 0
                | (wii_nchuk->buttonC() ? GAMEPAD_MASK_B1 : 0) // Generic: K1, Switch: B, Xbox: A
                | (wii_nchuk->buttonZ() ? GAMEPAD_MASK_B2 : 0) // Generic: K2, Switch: A, Xbox: B
              ;
    
              leftX = wii_nchuk->joyX();
              leftY = wii_nchuk->joyY();
            }
            
          }//end if classiccontroller / nunckuk
    
          bool buttonsChanged = buttonData != oldButtons || hatData != oldHat;
          bool analogChanged = (leftX != oldLX) || (leftY != oldLY) || (rightX != oldRX) || (rightY != oldRY) || (lt != oldLT) || (rt != oldRT);
    
          if (buttonsChanged || analogChanged) { //state changed?
            stateChanged = buttonsChanged;

            #ifndef WII_ANALOG_RAW
              //shift to center
              leftX -= wii_lx_shift;
              leftY -= wii_ly_shift;
              rightX -= wii_rx_shift;
              rightY -= wii_ry_shift;
              
              //clamp
              if (leftX < wii_analog_stick_min) leftX = wii_analog_stick_min;
              else if (leftX > wii_analog_stick_max) leftX = wii_analog_stick_max;
              if (leftY < wii_analog_stick_min) leftY = wii_analog_stick_min;
              else if (leftY > wii_analog_stick_max) leftY = wii_analog_stick_max;

              if (rightX < wii_analog_stick_min) rightX = wii_analog_stick_min;
              else if (rightX > wii_analog_stick_max) rightX = wii_analog_stick_max;
              if (rightY < wii_analog_stick_min) rightY = wii_analog_stick_min;
              else if (rightY > wii_analog_stick_max) rightY = wii_analog_stick_max;

              //scale
              leftX =  map(leftX,  wii_analog_stick_min, wii_analog_stick_max, 0, 255);
              leftY =  map(leftY,  wii_analog_stick_min, wii_analog_stick_max, 0, 255);
              rightX = map(rightX, wii_analog_stick_min, wii_analog_stick_max, 0, 255);
              rightY = map(rightY, wii_analog_stick_min, wii_analog_stick_max, 0, 255);
            #endif
                
            state[0].lx = convertAnalog( leftX);
            state[0].ly = convertAnalog((uint8_t)~leftY);
            state[0].rx = convertAnalog( rightX);
            state[0].ry = convertAnalog((uint8_t)~rightY);
            #ifndef WII_ANALOG_RAW
              if (isClassicAnalog) {
                if (lt < wii_analog_trigger_min) lt = 0;
                if (rt < wii_analog_trigger_min) rt = 0;
                lt = wii_classic->buttonL() ? 0xFF : lt;
                rt = wii_classic->buttonR() ? 0xFF : rt;
              }
            #endif

              state[0].lt = lt;
              state[0].rt = rt;


//            display.clear();
//            display.println(leftX);
//            display.println(leftY);
//            delay(200);
                
            #ifdef ENABLE_REFLEX_PAD
              //const uint8_t startCol = inputPort == 0 ? 0 : 11*6;
              const uint8_t nbuttons = (conType == ExtensionType::ClassicController || conType == ExtensionType::GuitarController) ? 15 : 2;
              for(uint8_t x = 0; x < nbuttons; ++x){
                const Pad pad = padWii[x];
                if(x < 11) //buttons
                  PrintPadChar(0, padDivision[0].firstCol, pad.col, pad.row, pad.padvalue, bitRead(buttonData, x), pad.on, pad.off);
                else //dpad
                  PrintPadChar(0, padDivision[0].firstCol, pad.col, pad.row, pad.padvalue, !bitRead(hatData, x-11), pad.on, pad.off);
              }
            #endif
            
            //keep values
            oldButtons = buttonData;
            oldHat = hatData;
            oldLX = leftX;
            oldLY = leftY;
            oldRX = rightX;
            oldRY = rightY;
            oldLT = lt;
            oldRT = rt;
          }//end if statechanged
    
        }//end else pad read
      }//end havecontroller
    
    
      return stateChanged; //joyCount != 0;
    }//end read

};
