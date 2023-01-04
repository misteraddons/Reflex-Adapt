/*******************************************************************************
 * Wii controllers to USB using an Arduino Leonardo.
 *
 * Works with digital pad.
 * 
 * Uses NintendoExtensionCtrl Lib
 * https://github.com/dmadison/NintendoExtensionCtrl/
 * 
 * Uses SoftWire Lib
 * https://github.com/felias-fogg/SoftI2CMaster
 *
 * For details on Joystick Library, see
 * https://github.com/MHeironimus/ArduinoJoystickLibrary
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

#define I2C_FASTMODE 1
#define I2C_TIMEOUT 1000
#define I2C_PULLUP 1

#include "src/SoftWire/SoftWire.h"
#include "src/NintendoExtensionCtrl/NintendoExtensionCtrl.h"
#include "src/ArduinoJoystickLibrary/Joy1.h"

ExtensionPort* wii;
ClassicController::Shared* wii_classic;
Nunchuk::Shared* wii_nchuk;

#define WII_ANALOG_DEFAULT_CENTER 127U

#ifdef ENABLE_REFLEX_PAD
  const Pad padWii[] = {
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
    //print default joystick state to oled screen
  
    //const uint8_t firstCol = index == 0 ? 0 : 11*6;
    //const uint8_t lastCol = index == 0 ? 11*6 : 127;
  
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
        display.print(F("NONE"));
        return;
    }

    if (index < 2) {
      //const uint8_t startCol = index == 0 ? 0 : 11*6;
      if(padType == ExtensionType::Nunchuk) {
        for(uint8_t x = 0; x < 2; x++){
          const Pad pad = padWii[x];
          PrintPadChar(index, padDivision[index].firstCol, pad.col, pad.row, pad.padvalue, true, pad.on, pad.off, true);
        }
      } else { //classic
        for(uint8_t x = 0; x < 15; x++){
          const Pad pad = padWii[x];
          PrintPadChar(index, padDivision[index].firstCol, pad.col, pad.row, pad.padvalue, true, pad.on, pad.off, true);
        }
      }
    }
  }
#endif

void wiiResetAnalogMinMax() {
  //n64_x_min = -50;
  //n64_x_max = 50;
  //n64_y_min = -50;
  //n64_y_max = 50;
}

void wiiResetJoyValues(const uint8_t i) {
  if (i >= totalUsb)
    return;
  usbStick[i]->resetState();
  ((Joy1_*)usbStick[0])->setAnalog0(WII_ANALOG_DEFAULT_CENTER); //x
  ((Joy1_*)usbStick[0])->setAnalog1(WII_ANALOG_DEFAULT_CENTER); //y
  ((Joy1_*)usbStick[0])->setAnalog2(WII_ANALOG_DEFAULT_CENTER); //rx
  ((Joy1_*)usbStick[0])->setAnalog3(WII_ANALOG_DEFAULT_CENTER); //ry
  wiiResetAnalogMinMax();
}

void wiiSetup() {
  //Init onboard led pin
  //pinMode(LED_BUILTIN, OUTPUT);
  
  delay(5);

  totalUsb = 1;
  sleepTime = 50;
  
  //Create usb controllers
  for (uint8_t i = 0; i < totalUsb; i++) {
    usbStick[i] = new Joy1_("RZordWii", JOYSTICK_DEFAULT_REPORT_ID + i, JOYSTICK_TYPE_GAMEPAD, totalUsb,
          true,//includeXAxis,
          true,//includeYAxis,
          false,//includeZAxis,
          true,//includeRxAxis,
          true,//includeRyAxis,
          false,//includeThrottle,
          false,//includeBrake,
          false);//includeSteering
  }
  
  //Set usb parameters and reset to default values
  for (uint8_t i = 0; i < totalUsb; i++) {
    wiiResetJoyValues(i);
    usbStick[i]->sendState();
  }


  //SoftWire initialization
  Wire.begin();

  //Will use 400khz if I2C_FASTMODE 1. No need to set the clock here.
  //Wire.setClock(400000L);

  wii = new ExtensionPort(Wire);
  wii_classic = new ClassicController::Shared(*wii);
  wii_nchuk = new Nunchuk::Shared(*wii);

  wii->begin();

  delay(50);
  
  dstart(115200);
  debugln (F("Powered on!"));
}

inline bool __attribute__((always_inline))
wiiLoop() {
  static uint8_t lastControllerCount = 0;
  static boolean haveController = false;
  static uint16_t oldButtons = 0;
  static uint8_t oldHat = 0;
  static uint8_t oldLX = WII_ANALOG_DEFAULT_CENTER, oldLY = WII_ANALOG_DEFAULT_CENTER;
  static uint8_t oldRX = WII_ANALOG_DEFAULT_CENTER, oldRY = WII_ANALOG_DEFAULT_CENTER;
  
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
      //controller removed
      haveController = true;
      wiiResetJoyValues(0);
      #ifdef ENABLE_REFLEX_PAD
        ShowDefaultPadWii(0, wii->getControllerType());
      #endif
      sleepTime = 50;
    } else {
      sleepTime = 50000;
    }
  } else {
    if (!wii->update()) {
      //controller removed
      haveController = false;
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

      const ExtensionType conType = wii->getControllerType();

      //only handle data from Classic or Nunchuk
      if(conType == ExtensionType::ClassicController || conType == ExtensionType::Nunchuk) {

        if(conType == ExtensionType::ClassicController) {
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

          bitWrite(hatData, 0, !wii_classic->dpadUp());
          bitWrite(hatData, 1, !wii_classic->dpadDown());
          bitWrite(hatData, 2, !wii_classic->dpadLeft());
          bitWrite(hatData, 3, !wii_classic->dpadRight());

          leftX = wii_classic->leftJoyX();
          leftY = wii_classic->leftJoyY();
          rightX = wii_classic->rightJoyX();
          rightY = wii_classic->rightJoyY();
          
        } else { //nunchuk
          bitWrite(buttonData, 0, wii_nchuk->buttonC());
          bitWrite(buttonData, 1, wii_nchuk->buttonZ());

          leftX = wii_nchuk->joyX();
          leftY = wii_nchuk->joyY();
        }
        
      }//end if classiccontroller / nunckuk

      bool buttonsChanged = buttonData != oldButtons || hatData != oldHat;
      bool analogChanged = leftX != oldLX || leftY != oldLY || rightX != oldRX || rightY != oldRY;

      if (buttonsChanged || analogChanged) { //state changed?
        stateChanged = true;
        
        ((Joy1_*)usbStick[0])->setButtons(buttonData);

        //Get angle from hatTable and pass to joystick class
        ((Joy1_*)usbStick[0])->setHatSwitch(hatTable[hatData]);
        
        ((Joy1_*)usbStick[0])->setAnalog0(leftX); //x
        ((Joy1_*)usbStick[0])->setAnalog1(-leftY); //y
        ((Joy1_*)usbStick[0])->setAnalog2(rightX); //rx
        ((Joy1_*)usbStick[0])->setAnalog3(-rightY); //ry

        usbStick[0]->sendState();

        #ifdef ENABLE_REFLEX_PAD
          //const uint8_t startCol = inputPort == 0 ? 0 : 11*6;
          const uint8_t nbuttons = (conType == ExtensionType::ClassicController) ? 15 : 2;
          for(uint8_t x = 0; x < nbuttons; x++){
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
      }//end if statechanged

    }//end else pad read
  }//end havecontroller


  return stateChanged; //joyCount != 0;
}
