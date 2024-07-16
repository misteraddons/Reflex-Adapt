/*******************************************************************************
 * PlayStation input module for RetroZord / Reflex-Adapt.
 * By Matheus Fraguas (sonik-br)
 * 
 * Handles up to 2 input ports.
 *
 * Uses PsxNewLib
 * https://github.com/SukkoPera/PsxNewLib
 *
 * Uses a modified version of MPG
 * https://github.com/sonik-br/MPG
 *
*/

#include "RZInputModule.h"
#include "src/DigitalIO/DigitalIO.h"
#include "src/PsxNewLib/PsxControllerHwSpi.h"


//Guncon config
//0=Mouse, 1=Joy, 2=Joy OffScreenEdge (MiSTer)
//#define GUNCON_FORCE_MODE 2

//NeGcon config
//0=Default, 1=MiSTer Wheel format with paddle
//#define NEGCON_FORCE_MODE 1
//If you dont want to force a mode but instead change the default:
//Don't enable the force mode and edit the isNeGconMiSTer variable below as you desire.

#if REFLEX_PIN_VERSION == 1
  const byte PIN_PS1_ATT = 2;
#else
  const byte PIN_PS1_ATT = 21;//A3
#endif

const byte PIN_PS2_ATT = 5;
const byte PIN_PS3_ATT = 10;
const byte PIN_PS4_ATT = 18;//A0
const byte PIN_PS5_ATT = 19;//A1
const byte PIN_PS6_ATT = 20;//A2

const uint8_t SPECIALMASK_POPN = 0xE;
const uint8_t SPECIALMASK_JET  = 0xC;

static const uint8_t PS_INTERVAL_DEFAULT  = 250;
static const uint16_t PS_INTERVAL_JET  = 1000;
static const uint16_t PS_INTERVAL_DS2  = 3000;

class ReflexInputPsx : public RZInputModule {
  private:
    PsxController* psx;//variable to hold current reading port

    PsxController* psxlist[2] {
      new PsxControllerHwSpi<PIN_PS1_ATT>(),
      new PsxControllerHwSpi<PIN_PS2_ATT>()
    };

    bool isNeGcon { false };
    bool isJogcon { false };
    bool isGuncon { false };
    bool isNeGconMiSTer { false };
    uint8_t specialDpadMask { 0x0 };
    bool haveController[2] { false, false };
    PsxControllerProtocol lastProto[2] { PSPROTO_UNKNOWN, PSPROTO_UNKNOWN };
    bool enableMouseMove { false }; //used on guncon and jogcon modes
    uint8_t outputIndex { 0 };

    void tryEnableRumble() {
      //if ((options.inputMode == INPUT_MODE_XINPUT || detectDS2) && !isJogcon){ //try to enable rumble
      if (!isJogcon){ //try to enable rumble
        if (psx->enterConfigMode ()) {
          psx->enableRumble ();
          if (psx->getControllerType () == PSCTRL_DUALSHOCK2) {
            sleepTime = PS_INTERVAL_DS2;
          }
          psx->exitConfigMode ();
        }
      }
    }

    //Include sub-modules as private
    #ifdef GUNCON_SUPPORT
      #include "Input_Psx_Guncon.h"
    #endif
    #ifdef NEGCON_SUPPORT
      #include "Input_Psx_Negcon.h"
    #endif
    #ifdef JOGCON_SUPPORT
      #include "Input_Psx_Jogcon.h"
    #endif

    #ifdef ENABLE_REFLEX_PAD
      const Pad padPsx[16] = {
        { PSB_SELECT,    2, 4*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
        { PSB_L3,        3, 4*6, FACEBTN_ON, FACEBTN_OFF },
        { PSB_R3,        3, 5*6, FACEBTN_ON, FACEBTN_OFF },
        { PSB_START,     2, 5*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
        { PSB_PAD_UP,    1, 1*6, UP_ON, UP_OFF },
        { PSB_PAD_RIGHT, 2, 2*6, RIGHT_ON, RIGHT_OFF },
        { PSB_PAD_DOWN,  3, 1*6, DOWN_ON, DOWN_OFF },
        { PSB_PAD_LEFT,  2, 0,   LEFT_ON, LEFT_OFF },
        { PSB_L2,        0, 2*6, SHOULDERBTN_ON, SHOULDERBTN_OFF },
        { PSB_R2,        0, 9*6, SHOULDERBTN_ON, SHOULDERBTN_OFF },
        { PSB_L1,        0, 0*6, SHOULDERBTN_ON, SHOULDERBTN_OFF },
        { PSB_R1,        0, 7*6, SHOULDERBTN_ON, SHOULDERBTN_OFF },
        { PSB_TRIANGLE,  1, 8*6, FACEBTN_ON, FACEBTN_OFF },
        { PSB_CIRCLE,    2, 9*6, FACEBTN_ON, FACEBTN_OFF },
        { PSB_CROSS,     3, 8*6, FACEBTN_ON, FACEBTN_OFF },
        { PSB_SQUARE,    2, 7*6, FACEBTN_ON, FACEBTN_OFF }
      };

      const Pad padPsxPopN[11] = {
        { PSB_SELECT,    0, 3*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
        { PSB_START,     0, 5*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
        { PSB_CIRCLE,    1, 1*6, FACEBTN_ON, FACEBTN_OFF },
        { PSB_CROSS,     1, 3*6, FACEBTN_ON, FACEBTN_OFF },
        { PSB_SQUARE,    1, 5*6, FACEBTN_ON, FACEBTN_OFF },
        { PSB_PAD_UP,    1, 7*6, FACEBTN_ON, FACEBTN_OFF },
        { PSB_TRIANGLE,  2, 0*6, FACEBTN_ON, FACEBTN_OFF },
        { PSB_R1,        2, 2*6, FACEBTN_ON, FACEBTN_OFF },
        { PSB_L1,        2, 4*6, FACEBTN_ON, FACEBTN_OFF },
        { PSB_R2,        2, 6*6, FACEBTN_ON, FACEBTN_OFF },
        { PSB_L2,        2, 8*6, FACEBTN_ON, FACEBTN_OFF }
      };
    
      void loopPadDisplayCharsPsx(const uint8_t index, const PsxControllerProtocol padType, void* p, const bool force) {
//        for(uint8_t i = 0; i < (sizeof(padPsx) / sizeof(Pad)); ++i){
//          if(padType != PSPROTO_DUALSHOCK && padType != PSPROTO_DUALSHOCK2 && (i == 1 || i == 2))
//            continue;
//          if(padType == PSPROTO_NEGCON && (i == 0 || i == 8 || i == 9))
//            continue;
//          const Pad pad = padPsx[i];
//          PrintPadChar(index, padDivision[index].firstCol, pad.col, pad.row, pad.padvalue, p && static_cast<PsxController*>(p)->buttonPressed(static_cast<PsxButton>(pad.padvalue)), pad.on, pad.off, force);
//        }
        if(specialDpadMask == SPECIALMASK_POPN) {
          for(uint8_t i = 0; i < (sizeof(padPsxPopN) / sizeof(Pad)); ++i){
            const Pad pad = padPsxPopN[i];
            PrintPadChar(index, padDivision[index].firstCol, pad.col, pad.row, pad.padvalue, p && static_cast<PsxController*>(p)->buttonPressed(static_cast<PsxButton>(pad.padvalue)), pad.on, pad.off, force);
          }
        } else {
          for(uint8_t i = 0; i < (sizeof(padPsx) / sizeof(Pad)); ++i){
            if(padType != PSPROTO_DUALSHOCK && padType != PSPROTO_DUALSHOCK2 && (i == 1 || i == 2))
              continue;
            if(padType == PSPROTO_NEGCON && (i == 0 || i == 8 || i == 9))
              continue;
            const Pad pad = padPsx[i];
            PrintPadChar(index, padDivision[index].firstCol, pad.col, pad.row, pad.padvalue, p && static_cast<PsxController*>(p)->buttonPressed(static_cast<PsxButton>(pad.padvalue)), pad.on, pad.off, force);
          }
      }
    }
    
      void ShowDefaultPadPsx(const uint8_t index, const PsxControllerProtocol padType) {
        display.clear(padDivision[index].firstCol, padDivision[index].lastCol, oledDisplayFirstRow + 1, 7);
        display.setCursor(padDivision[index].firstCol, 7);
    
        switch(padType) {
          case PSPROTO_DIGITAL:
          case PSPROTO_NEGCON:
            if (specialDpadMask == SPECIALMASK_POPN)
              display.print(F("POP N"));
            else
              display.print(isNeGcon ? F("NEGCON") : PSTR_TO_F(PSTR_DIGITAL));
            break;
          case PSPROTO_DUALSHOCK:
            if (specialDpadMask == SPECIALMASK_JET){
              display.print(F("JET"));
            } else {
              display.print(F("DUALSHOCK"));
            }
            break;
          //case PSPROTO_DUALSHOCK2:
          //  display.print(F("DUALSHOCK2"));
          //  break;
          case PSPROTO_FLIGHTSTICK:
            display.print(F("FLIGHTSTICK"));
            break;
          //case PSPROTO_NEGCON:
          //  display.print(F("NEGCON-"));
          //  display.print(isNeGcon ? F("ANALOG") : PSTR_TO_F(PSTR_DIGITAL));
          //  break;
          case PSPROTO_JOGCON:
            display.print(F("JOGCON"));
            break;
          default:
            display.print(PSTR_TO_F(PSTR_NONE));
            return;
        }
      
        if (index < 2) {
          loopPadDisplayCharsPsx(index, padType, NULL, true);
        }
      }
    #endif

    void handleDpad() {
      state[outputIndex].dpad = 0
        | (psx->buttonPressed(PSB_PAD_UP)    ? GAMEPAD_MASK_UP    : 0)
        | (psx->buttonPressed(PSB_PAD_DOWN)  ? GAMEPAD_MASK_DOWN  : 0)
        | (psx->buttonPressed(PSB_PAD_LEFT)  ? GAMEPAD_MASK_LEFT  : 0)
        | (psx->buttonPressed(PSB_PAD_RIGHT) ? GAMEPAD_MASK_RIGHT : 0)
      ;
      if (specialDpadMask)
        state[outputIndex].dpad = (state[outputIndex].dpad | specialDpadMask) & 0xF;
    }

    bool loopDualShock() {
      static byte lastLX[] = { ANALOG_IDLE_VALUE, ANALOG_IDLE_VALUE };
      static byte lastLY[] = { ANALOG_IDLE_VALUE, ANALOG_IDLE_VALUE };
      static byte lastRX[] = { ANALOG_IDLE_VALUE, ANALOG_IDLE_VALUE };
      static byte lastRY[] = { ANALOG_IDLE_VALUE, ANALOG_IDLE_VALUE };
    
      #ifdef ENABLE_REFLEX_PAD
        static PsxControllerProtocol lastPadType[] = { PSPROTO_UNKNOWN, PSPROTO_UNKNOWN };
        PsxControllerProtocol currentPadType[] = { PSPROTO_UNKNOWN, PSPROTO_UNKNOWN };
        const uint8_t inputPort = outputIndex; //todo fix
      #endif
    
      byte analogX = ANALOG_IDLE_VALUE;
      byte analogY = ANALOG_IDLE_VALUE;
      //word convertedX, convertedY;
    
      const bool digitalStateChanged = psx->buttonsChanged();//check if any digital value changed (dpad and buttons)
      bool stateChanged = digitalStateChanged;
      
      const PsxControllerProtocol proto = psx->getProtocol();
    
      #ifdef ENABLE_REFLEX_PAD
        if(proto != lastPadType[inputPort])
          ShowDefaultPadPsx(inputPort, proto);
        currentPadType[inputPort] = proto;
      #endif
    
      
      switch (proto) {
      case PSPROTO_DIGITAL:
        //if (!stateChanged)
        //  return false;
      case PSPROTO_NEGCON:
      case PSPROTO_DUALSHOCK:
      case PSPROTO_DUALSHOCK2:
      case PSPROTO_FLIGHTSTICK:
      {
        handleDpad();
    
//        uint16_t buttonData = 0;
        //controller buttons
        state[outputIndex].buttons = 0
          | (psx->buttonPressed(PSB_CROSS)    ? GAMEPAD_MASK_B1 : 0) // Generic: K1, Switch: B, Xbox: A
          | (psx->buttonPressed(PSB_CIRCLE)   ? GAMEPAD_MASK_B2 : 0) // Generic: K2, Switch: A, Xbox: B
          | (psx->buttonPressed(PSB_SQUARE)   ? GAMEPAD_MASK_B3 : 0) // Generic: P1, Switch: Y, Xbox: X
          | (psx->buttonPressed(PSB_TRIANGLE) ? GAMEPAD_MASK_B4 : 0) // Generic: P2, Switch: X, Xbox: Y
          | (psx->buttonPressed(PSB_L1)       ? GAMEPAD_MASK_L1 : 0) // Generic: P4, Switch: L, Xbox: LB
          | (psx->buttonPressed(PSB_R1)       ? GAMEPAD_MASK_R1 : 0) // Generic: P3, Switch: R, Xbox: RB
          | (psx->buttonPressed(PSB_L2)       ? GAMEPAD_MASK_L2 : 0) // Generic: K4, Switch: ZL, Xbox: LT (Digital)
          | (psx->buttonPressed(PSB_R2)       ? GAMEPAD_MASK_R2 : 0) // Generic: K3, Switch: ZR, Xbox: RT (Digital)
          | (psx->buttonPressed(PSB_SELECT)   ? GAMEPAD_MASK_S1 : 0) // Generic: Select, Switch: -, Xbox: View
          | (psx->buttonPressed(PSB_START)    ? GAMEPAD_MASK_S2 : 0) // Generic: Start, Switch: +, Xbox: Menu
          | (psx->buttonPressed(PSB_L3)       ? GAMEPAD_MASK_L3 : 0) // All: Left Stick Click
          | (psx->buttonPressed(PSB_R3)       ? GAMEPAD_MASK_R3 : 0) // All: Right Stick Click
        ;
    
    
    //if(proto != PSPROTO_DIGITAL)
    
        //analog sticks
        if (psx->getLeftAnalog(analogX, analogY) && proto != PSPROTO_NEGCON) {
          state[outputIndex].lx = convertAnalog(analogX);
          state[outputIndex].ly = convertAnalog(analogY);
        } else {
          analogX = ANALOG_IDLE_VALUE;
          analogY = ANALOG_IDLE_VALUE;
          state[outputIndex].lx = convertAnalog(analogX);
          state[outputIndex].ly = convertAnalog(analogY);
        }
        
        if (lastLX[outputIndex] != analogX || lastLY[outputIndex] != analogY)
          stateChanged = true;
          
        lastLX[outputIndex] = analogX;
        lastLY[outputIndex] = analogY;
    
        if (psx->getRightAnalog(analogX, analogY) && proto != PSPROTO_NEGCON) {
          if (specialDpadMask == SPECIALMASK_JET)
            analogX = ANALOG_IDLE_VALUE;
          state[outputIndex].rx = convertAnalog(analogX);
          state[outputIndex].ry = convertAnalog(analogY);
        } else {
          analogX = ANALOG_IDLE_VALUE;
          analogY = ANALOG_IDLE_VALUE;
          state[outputIndex].rx = convertAnalog(analogX);
          state[outputIndex].ry = convertAnalog(analogY);
        }
        
        if (lastRX[outputIndex] != analogX || lastRY[outputIndex] != analogY)
          stateChanged = true;
    
        lastRX[outputIndex] = analogX;
        lastRY[outputIndex] = analogY;
    
        if(stateChanged) {
          #ifdef ENABLE_REFLEX_PAD
            if (inputPort < 2) {
              loopPadDisplayCharsPsx(inputPort, proto, psx, false);
            }
          #endif
        }
    
        break;
      }
      default:
        break;
      }
    
    
      #ifdef ENABLE_REFLEX_PAD
        /*for (uint8_t i = 0; i < 2; i++) {
          if(lastPadType[i] != currentPadType[i] && currentPadType[i] == PSPROTO_UNKNOWN) {
            ShowDefaultPadPsx(i, currentPadType[i]);
          }
          lastPadType[i] = currentPadType[i];
        }*/
        lastPadType[inputPort] = currentPadType[inputPort];
      #endif
    
      return digitalStateChanged;
    }


  public:
    ReflexInputPsx() : RZInputModule() { }

    const char* getUsbId() override {
      static const char* usbId_ds1       { "RZMPsDS1" };
      static const char* usbId_guncon    { "ReflexPSGun" }; //{ "RZordPsGun" };
      static const char* usbId_negcon    { "RZMPsNeGcon" };
      //static const char* usbId_negwheel { "RZordPsWheel" }; //{ "RZMPsWheel" };
      static const char* usbId_negwheel  { "ReflexPSWheel" }; //{ "RZMPsWheel" };
      static const char* usbId_misterjog { "MiSTer-A1 JogCon" };
      static const char* usbId_mousejog  { "RZMPSJogCon" };
      
      if (isGuncon)
        return usbId_guncon;
      else if (isNeGcon)
        return isNeGconMiSTer ? usbId_negwheel : usbId_negcon;
      else if (isJogcon)
        return enableMouseMove ? usbId_mousejog : usbId_misterjog;
      else
        return usbId_ds1;
    }

    const uint16_t getUsbVersion() override {
      return MODE_ID_PSX;
    }

//    #ifdef ENABLE_REFLEX_PAD
//      void displayInputName() override {
//        display.setCol(5*6);
//        display.print(F("PLAYSTATION"));
//      }
//    #endif

    void setup() override {
      //initialize the "attention" pins as OUTPUT HIGH.
      fastPinMode(PIN_PS1_ATT, OUTPUT);
      fastPinMode(PIN_PS2_ATT, OUTPUT);
    
      fastDigitalWrite(PIN_PS1_ATT, HIGH);
      fastDigitalWrite(PIN_PS2_ATT, HIGH);
      
      psx = psxlist[0];

      sleepTime = PS_INTERVAL_DEFAULT;
    
      //if forcing specific mode
    #ifdef ENABLE_REFLEX_PSX_JOG
      PsxControllerProtocol proto = (deviceMode == RZORD_PSX_JOG ? PSPROTO_JOGCON : PSPROTO_UNKNOWN);
    #else
      PsxControllerProtocol proto = PSPROTO_UNKNOWN;
    #endif
    
      if (psx->begin ()) {
        //delay(150);//200
        //haveController = true;
        haveController[0] = true;
        //const PsxControllerProtocol proto = psx->getProtocol();
    
    
        //if not forced a mode, then read from currenct connected controller
        if(proto == PSPROTO_UNKNOWN)
          proto = psx->getProtocol();

        lastProto[0] = proto;
    
        if (proto == PSPROTO_GUNCON) {
          isGuncon = true;
        } else if (proto == PSPROTO_NEGCON) {
          isNeGcon = true;
    
          //Configure NeGcon mode
          #if defined(NEGCON_FORCE_MODE) && NEGCON_FORCE_MODE >= 0 && NEGCON_FORCE_MODE < 2
            #if NEGCON_FORCE_MODE == 1
              isNeGconMiSTer = true;
            #endif
          #else //NEGCON_FORCE_MODE
            if (psx->buttonPressed(PSB_CIRCLE)) //NeGcon A / Volume B
              isNeGconMiSTer = !isNeGconMiSTer;
          #endif //NEGCON_FORCE_MODE
        } else { //jogcon can't be detected during boot as it needs to be in analog mode
    
    #ifdef JOGCON_SUPPORT
        //Try to detect by it's id
        if(proto == PSPROTO_DIGITAL) {
          if (psx->enterConfigMode ()) {
            if (psx->getControllerType () == PSCTRL_JOGCON) {
              isJogcon = true;
              if (psx->buttonPressed(PSB_L2))
                enableMouseMove = true;
            }
            psx->exitConfigMode ();
          }
        }
    #endif

          if (psx->buttonPressed(PSB_SELECT)) { //dualshock used in guncon mode to help map axis on emulators.
            isGuncon = true;
          }
          /*else if (proto == PSPROTO_JOGCON || psx->buttonPressed(PSB_L1)) {
            isJogcon = true;
          } else if (psx->buttonPressed(PSB_L2)) {
            isJogcon = true;
            enableMouseMove = true;
          }*/
        }

        tryEnableRumble();


      } else { //no controller connected
        if (proto == PSPROTO_JOGCON)
          isJogcon = true;
      }
    
      if (isNeGcon) {
        #ifdef NEGCON_SUPPORT
          negconSetup();
        #endif
      } else if (isJogcon) {
        #ifdef JOGCON_SUPPORT
          jogconSetup();
        #endif
      } else {
        if (isGuncon) {
          #ifdef GUNCON_SUPPORT
            gunconSetup();
          #endif
        } else { //dualshock [default]
          
          if (proto == PSPROTO_DIGITAL
          && psx->buttonPressed(PSB_PAD_DOWN)
          && psx->buttonPressed(PSB_PAD_LEFT)
          && psx->buttonPressed(PSB_PAD_RIGHT))
            specialDpadMask = SPECIALMASK_POPN;

          if (proto == PSPROTO_DUALSHOCK
          && psx->buttonPressed(PSB_PAD_LEFT)
          && psx->buttonPressed(PSB_PAD_RIGHT)) {
            byte analogX;
            byte analogY;
            if (psx->getRightAnalog(analogX, analogY) && analogX == 0xFF) {
              sleepTime = PS_INTERVAL_JET;
              specialDpadMask = SPECIALMASK_JET;
            }
          }
            
          totalUsb = 2;//MAX_USB_STICKS;
          for (uint8_t i = 0; i < totalUsb; i++) {
            hasLeftAnalogStick[i] = true;
            hasRightAnalogStick[i] = true;
          }
        }
      }
    }//end setup

    void setup2() override {
      if (isNeGcon) {
        #ifdef NEGCON_SUPPORT
          negconSetup2();
        #endif
      } else if (isJogcon) {
        #ifdef JOGCON_SUPPORT
          jogconSetup2();
        #endif
      } else if (isGuncon) {
        #ifdef GUNCON_SUPPORT
          gunconSetup2();
        #endif
      }
    }

    bool read() override {
      static bool isReadSuccess[] = {false,false};
      static bool isEnabled[] = {false,false};
      bool stateChanged = false;
    
      outputIndex = 0;
    
      if(isJogcon) {
        #ifdef JOGCON_SUPPORT
          if (!haveController[0]) {
            init_jogcon();
          } else {
            if(!psx->read()){
              //haveController = false;
              haveController[0] = false;
            } else {
              stateChanged = handleJogconData();
            }
          }
        #endif
        return stateChanged;//haveController[0];
      }
    
      //if (millis() - last >= POLLING_INTERVAL) {
      //  last = millis();
    
    
        //nothing detected yet
        if (!isEnabled[0] && !isEnabled[1]) {
          for (uint8_t i = 0; i < 2; i++) {
            //isEnabled[i] = haveController[i] || psxlist[i]->begin();
            isEnabled[i] = haveController[i] || (haveController[i] = psxlist[i]->begin());
            //haveController[i] = haveController[i] || psxlist[i]->begin();
            //isEnabled[i] = haveController[i];
          }
          #if defined REFLEX_USE_OLED_DISPLAY && defined ENABLE_PSX_GENERAL_OLED
          //display.setCursor(0, 7);
          //display.clearToEOL();
          //display.print(F("Ports "));
          setOledDisplay(true);
          //clearOledLineAndPrint(0, 7 - oledDisplayFirstRow, F("Ports "));
          //clearOledLineAndPrint(0, 6 - oledDisplayFirstRow, F("Connect a single pad."));
          //clearOledLineAndPrint(0, 7 - oledDisplayFirstRow, F("Connect two pads and reset device."));
    
          //clearOledLineAndPrint(0, 5 - oledDisplayFirstRow, F("To begin, connect:"));
          //clearOledLineAndPrint(0, 6 - oledDisplayFirstRow, F("- A single pad."));
          //clearOledLineAndPrint(0, 7 - oledDisplayFirstRow, F("- Two pads, then reset device."));
    
          //clearOledLineAndPrint(6, 6 - oledDisplayFirstRow, F("CONNECT A CONTROLLER"));
          //clearOledLineAndPrint(4*6, 7 - oledDisplayFirstRow, F("TO INITIALIZE"));
    
          display.clear(0, 127, 7, 7);
          display.setRow(7);
    
    
          for (uint8_t i = 0; i < 2; i++) {
            //const uint8_t firstCol = i == 0 ? 0 : 12*6;
            display.setCol(padDivision[i].firstCol);
            if (!isEnabled[0] && !isEnabled[1])
              display.print(PSTR_TO_F(PSTR_NONE));  
            else if (!isEnabled[i])
              display.print(PSTR_TO_F(PSTR_NA));
          }
    /*      
          if(!isEnabled[0] && !isEnabled[1]) {
            
            for (uint8_t i = 0; i < 2; i++) {
              const uint8_t firstCol = i == 0 ? 0 : 12*6;
              display.setCol(firstCol);
              display.print(PSTR_TO_F(PSTR_NONE));
            }
          }
    
          if(isEnabled[0] || isEnabled[1]) {
            for (uint8_t i = 0; i < 2; i++) {
              //display.setCol(36 + (i*12)); //each char is 6 cols
              const uint8_t firstCol = i == 0 ? 0 : 12*6;
              display.setCol(firstCol); //each char is 6 cols
              if(isEnabled[i])
                display.print(PSTR_TO_F(PSTR_NONE));
              else
                display.print(PSTR_TO_F(PSTR_NA));
            }
          }
          */
    
          
          //for (uint8_t i = 0; i < 2; i++) {
          //  if(isEnabled[i]){
          //    display.setCol(36 + (i*12)); //each char is 6 cols
          //    display.print(i+1);
          //  }
          //}
          #endif
        }
    
    
        //read all ports
    
        for (uint8_t i = 0; i < 2; i++) {
          psx = psxlist[i];
          isReadSuccess[i] = false;
    
          if (!isEnabled[i])
            continue;
          
          if (!haveController[i]) {
            if (psx->begin()) {
              haveController[i] = true;
              tryEnableRumble();
              ShowDefaultPadPsx(i, psx->getProtocol());
            }
          } else {
            const PsxControllerProtocol proto = psx->getProtocol();
            if(lastProto[i] != proto)
              tryEnableRumble();
            lastProto[i] = proto;
            #ifdef PSX_COMBINE_RUMBLE
              psx->setRumble ((rumble[i].left_power | rumble[i].right_power) != 0x0, (rumble[i].left_power | rumble[i].right_power));
            #else
              psx->setRumble (rumble[i].right_power != 0x0, rumble[i].left_power);
            #endif
            isReadSuccess[i] = psx->read();
            if (!isReadSuccess[i]){ //debug (F("Controller lost.")); debug (F(" last values: x = ")); debug (lastX); debug (F(", y = ")); debugln (lastY);
              haveController[i] = false;
              ShowDefaultPadPsx(i, PSPROTO_UNKNOWN);
            }
          }
          if(isGuncon)//only use first port for guncon
            break;
        }
    
    
        for (uint8_t i = 0; i < totalUsb; i++) {
          if (haveController[i] && isReadSuccess[i]) {
            psx = psxlist[i];
            if (isNeGcon) {
              #ifdef NEGCON_SUPPORT
                stateChanged |= loopNeGcon();
              #endif
            } else if (isGuncon) {
              #ifdef GUNCON_SUPPORT
                loopGuncon();
              #endif
            } else {
              stateChanged |= loopDualShock();
            }
          }
          outputIndex++;
        }
    
      //} end if (millis() - last >= POLLING_INTERVAL)
      return stateChanged;//haveController[0] || haveController[1];
    }//end read

};
