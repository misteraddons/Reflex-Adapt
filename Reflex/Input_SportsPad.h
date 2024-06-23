/*******************************************************************************
   Reflex Adapt USB
   Master System Sports Pad (Sports Mode) input module

   Uses a modified version of Joystick Library
   https://github.com/MHeironimus/ArduinoJoystickLibrary
*/

#include "src/DigitalIO/DigitalIO.h"
#ifdef ENABLE_SMS_SPORTSPAD_MOUSE // TODO: Add Analog Joystick Mode
  #include <Mouse.h>
  #define SCALING 4
#endif

#ifndef ENABLE_REFLEX_SMS
  //SMS pins - Port 1
  #define SMS1_TH 4
  #define SMS1_TR 13
  #define SMS1_TL 5
  #define SMS1_U  9
  #define SMS1_D  8
  #define SMS1_L  7
  #define SMS1_R  6
  
  /* port 2 not yet implemented
    //SMS pins - Port 2
    #define SMS2_TH 16
    #define SMS2_TR 10
    #define SMS2_TL 14
    #define SMS2_U  20
    #define SMS2_D  19
    #define SMS2_L  18
    #define SMS2_R  15
  */
  DigitalPin<SMS1_U> sms1_U;
  DigitalPin<SMS1_D> sms1_D;
  DigitalPin<SMS1_L> sms1_L;
  DigitalPin<SMS1_R> sms1_R;
  DigitalPin<SMS1_TL> sms1_TL;
  DigitalPin<SMS1_TR> sms1_TR;
  DigitalPin<SMS1_TH> sms1_TH;
  
  /*
    DigitalPin<SMS2_U> sms2_U;
    DigitalPin<SMS2_D> sms2_D;
    DigitalPin<SMS2_L> sms2_L;
    DigitalPin<SMS2_R> sms2_R;
    DigitalPin<SMS2_TL> sms2_TL;
    DigitalPin<SMS2_TR> sms2_TR;
    DigitalPin<SMS2_TH> sms2_TH;
  */
#endif

struct SportsPadData {
  int8_t dx, dy;
  uint8_t portState;
};

struct SportsPadData getSportsPadData(const uint8_t inputPort) {
  struct SportsPadData sp;

  // https://www.smspower.org/Development/SMSOfficialDocs#HOWTHETRACKBALLWORKS
  // every time we strobe TH, we read four bits from the controller.
  // this will give us two signed 8-bit values for the change in X and Y
  // since the last time we read it.

  sms1_TH.write(LOW);
  delayMicroseconds(80);
  sp.dx = sms1_R << 7 | sms1_L << 6 | sms1_D << 5 | sms1_U << 4; // first 4 bits of X
  sms1_TH.write(HIGH);
  delayMicroseconds(40);
  sp.dx = -(sp.dx | (sms1_R << 3 | sms1_L << 2 | sms1_D << 1 | sms1_U)); // last 4 bits of X
  sms1_TH.write(LOW);
  delayMicroseconds(40);
  sp.dy = sms1_R << 7 | sms1_L << 6 | sms1_D << 5 | sms1_U << 4; // first 4 bits of Y
  sms1_TH.write(HIGH);
  delayMicroseconds(40);
  sp.dy = -(sp.dy | (sms1_R << 3 | sms1_L << 2 | sms1_D << 1 | sms1_U)); // last 4 bits of Y
  sp.portState = sms1_TR << 5 | sms1_TL << 4 | !(sp.dx > 0) << 3 | !(sp.dx < 0) << 2 | !(sp.dy > 0) << 1 | !(sp.dy < 0);
  
  return sp;
}

#ifdef ENABLE_REFLEX_PAD

const Pad padSportsPad[] = {
  { 0x0001, 1, 1 * 6, UP_ON, UP_OFF },         //Up
  { 0x0002, 3, 1 * 6, DOWN_ON, DOWN_OFF },     //Down
  { 0x0004, 2, 0,   LEFT_ON, LEFT_OFF },       //Left
  { 0x0008, 2, 2 * 6, RIGHT_ON, RIGHT_OFF },   //Right
  { 0x0010, 3, 7 * 6, FACEBTN_ON, FACEBTN_OFF }, //A
  { 0x0020, 3, 8 * 6, FACEBTN_ON, FACEBTN_OFF }, //B
};

void ShowDefaultPadSportsPad(const uint8_t index) {
  //print default joystick state to oled screen

  const uint8_t firstCol = padDivision[index].firstCol;
  display.clear(firstCol, padDivision[index].lastCol, oledDisplayFirstRow + 1, 7);
  display.setCursor(firstCol, 7);

  const uint8_t total = 6;
  for (uint8_t x = 0; x < total; ++x) {
    const Pad pad = padSportsPad[x];
    PrintPadChar(index, firstCol, pad.col, pad.row, pad.padvalue, false, pad.on, pad.off, true);
  }
}
#endif

void sportsPadSetup() {
  //init input pins with pull-up
  sms1_U.config(INPUT, HIGH);
  sms1_D.config(INPUT, HIGH);
  sms1_L.config(INPUT, HIGH);
  sms1_R.config(INPUT, HIGH);
  sms1_TL.config(INPUT, HIGH);
  sms1_TR.config(INPUT, HIGH);
  //set TH as output
  sms1_TH.config(OUTPUT, HIGH);
  
  /* we can only have one mouse so we only have one possible device (TODO: option to act as analog sticks)
      sms2_U.config(INPUT, HIGH);
      sms2_D.config(INPUT, HIGH);
      sms2_L.config(INPUT, HIGH);
      sms2_R.config(INPUT, HIGH);
      sms2_TL.config(INPUT, HIGH);
      sms2_TR.config(INPUT, HIGH);
      sms2_TH.config(OUTPUT, HIGH);
  */
  sleepTime = 50;
  delay(5);
}

inline bool __attribute__((always_inline))
sportsPadLoop() {
  bool stateChanged = false;
#ifdef ENABLE_REFLEX_PAD
  static bool firstTime = true;
  if (firstTime) {
    firstTime = false;
    for (uint8_t i = 0; i < 1; ++i) {
      ShowDefaultPadSportsPad(i);
    }
  }
#endif

  int inputPort = 0;

  bool oldtr = sms1_TR;
  bool oldtl = sms1_TL;

  struct SportsPadData sp = getSportsPadData(inputPort);

  if (sp.dx || sp.dy || sms1_TL != oldtl || sms1_TR != oldtr) {
    stateChanged = true;
    #ifdef ENABLE_SMS_SPORTSPAD_MOUSE
      Mouse.move(sp.dx * SCALING, sp.dy * SCALING);
      if (!sms1_TL && oldtl)
        Mouse.press(MOUSE_LEFT);
      if (sms1_TL && !oldtl)
        Mouse.release(MOUSE_LEFT);
      if (!sms1_TR && oldtr)
        Mouse.press(MOUSE_RIGHT);
      if (sms1_TR && !oldtr)
        Mouse.release(MOUSE_RIGHT);
    #endif
  } else {
    stateChanged = false;
  }

#ifdef ENABLE_REFLEX_PAD
  
  uint8_t portState = sp.portState;
  const uint8_t firstCol = padDivision[inputPort].firstCol;
  const uint8_t total = 6;
  for (uint8_t i = 0; i < total; ++i) {
    const bool isPressed = !(portState & (1 << i));
    const Pad pad = padSportsPad[i];
    PrintPadChar(inputPort, firstCol, pad.col, pad.row, pad.padvalue, isPressed, pad.on, pad.off);
  }
#endif
  return stateChanged;
}
