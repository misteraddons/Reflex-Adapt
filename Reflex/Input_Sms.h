/*******************************************************************************
 * Sega Master System controllers to USB using an Arduino Leonardo.
 *
 * Works with digital pad.
*/

#include "src/DigitalIO/DigitalIO.h"
#include "src/ArduinoJoystickLibrary/Joy1.h"

//Saturn pins - Port 1
#define SMS1_TH 4
#if REFLEX_PIN_VERSION == 1
  #define SMS1_TR 3
#else
  #define SMS1_TR 13
#endif
#define SMS1_TL 5
#define SMS1_U 9
#define SMS1_D 8
#define SMS1_L 7
#define SMS1_R 6

//Saturn pins - Port 2
#define SMS2_TH 16
#define SMS2_TR 10
#define SMS2_TL 14
#define SMS2_U 20
#define SMS2_D 19
#define SMS2_L 18
#define SMS2_R 15

DigitalPin<SMS1_U> sms1_U;
DigitalPin<SMS1_D> sms1_D;
DigitalPin<SMS1_L> sms1_L;
DigitalPin<SMS1_R> sms1_R;
DigitalPin<SMS1_TL> sms1_TL;
DigitalPin<SMS1_TR> sms1_TR;
//DigitalPin<SMS1_TH> sms1_TH;

DigitalPin<SMS2_U> sms2_U;
DigitalPin<SMS2_D> sms2_D;
DigitalPin<SMS2_L> sms2_L;
DigitalPin<SMS2_R> sms2_R;
DigitalPin<SMS2_TL> sms2_TL;
DigitalPin<SMS2_TR> sms2_TR;
//DigitalPin<SMS2_TH> sms2_TH;

uint8_t getSmsData(const uint8_t inputPort) {
  return inputPort
    ? ((sms2_TR << 5) | (sms2_TL << 4) | (sms2_R << 3) | (sms2_L << 2) | (sms2_D << 1) | sms2_U)
    : ((sms1_TR << 5) | (sms1_TL << 4) | (sms1_R << 3) | (sms1_L << 2) | (sms1_D << 1) | sms1_U)
  ;
}

#ifdef ENABLE_REFLEX_PAD
  const Pad padSms[] = {
    { 0x0001, 1, 2*6, UP_ON, UP_OFF },        //Up
    { 0x0002, 3, 2*6, DOWN_ON, DOWN_OFF },    //Down
    { 0x0004, 2, 1*6, LEFT_ON, LEFT_OFF },    //Left
    { 0x0008, 2, 3*6, RIGHT_ON, RIGHT_OFF },  //Right
    { 0x0010, 3, 6*6, FACEBTN_ON, FACEBTN_OFF },  //A
    { 0x0020, 3, 7*6, FACEBTN_ON, FACEBTN_OFF }   //B
  };
  
  void ShowDefaultPadSms(const uint8_t index) {
    //print default joystick state to oled screen

    const uint8_t firstCol = padDivision[index].firstCol;  
    display.clear(firstCol, padDivision[index].lastCol, oledDisplayFirstRow + 1, 7);
    display.setCursor(firstCol, 7);

    for(uint8_t x = 0; x < 6; ++x){
      const Pad pad = padSms[x];
      PrintPadChar(index, firstCol, pad.col, pad.row, pad.padvalue, false, pad.on, pad.off, true);
    }
  }
#endif

void smsSetup() {
  //init input pins with pull-up
  sms1_U.config(INPUT, HIGH);
  sms1_D.config(INPUT, HIGH);
  sms1_L.config(INPUT, HIGH);
  sms1_R.config(INPUT, HIGH);
  sms1_TL.config(INPUT, HIGH);
  sms1_TR.config(INPUT, HIGH);
  //sms1_TH.config(INPUT, HIGH);

  sms2_U.config(INPUT, HIGH);
  sms2_D.config(INPUT, HIGH);
  sms2_L.config(INPUT, HIGH);
  sms2_R.config(INPUT, HIGH);
  sms2_TL.config(INPUT, HIGH);
  sms2_TR.config(INPUT, HIGH);
  //sms2_TH.config(INPUT, HIGH);

  //Create usb controllers
  totalUsb = 2;
  for (uint8_t i = 0; i < totalUsb; ++i) {
    usbStick[i] = new Joy1_("RZordSms", JOYSTICK_DEFAULT_REPORT_ID + i, JOYSTICK_TYPE_GAMEPAD, totalUsb);
  }

  //Set usb parameters and reset to default values
  for (uint8_t i = 0; i < totalUsb; ++i) {
    usbStick[i]->resetState();
    usbStick[i]->sendState();
  }

  sleepTime = 50;

  delay(5);
}

inline bool __attribute__((always_inline))
smsLoop() {
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
    const uint8_t portState = getSmsData(inputPort);

    //Only process data if state changed from previous read
    if (lastState[inputPort] != portState) {
      stateChanged = true;
      lastState[inputPort] = portState;

      uint8_t buttonData = 0;
      bitWrite(buttonData, 1, ~portState & 0x10); //A
      bitWrite(buttonData, 2, ~portState & 0x20); //B

      ((Joy1_*)usbStick[inputPort])->setButtons(buttonData);

      //Get angle from hatTable and pass to joystick class
      ((Joy1_*)usbStick[inputPort])->setHatSwitch(hatTable[portState & 0xF]);

      usbStick[inputPort]->sendState();

      #ifdef ENABLE_REFLEX_PAD
        const uint8_t firstCol = padDivision[inputPort].firstCol;
        for(uint8_t i = 0; i < 6; ++i) {
          const bool isPressed = !(portState & (1 << i));
          const Pad pad = padSms[i];
          PrintPadChar(inputPort, firstCol, pad.col, pad.row, pad.padvalue, isPressed, pad.on, pad.off);
        }
      #endif
    }
  }

  return stateChanged;
}
