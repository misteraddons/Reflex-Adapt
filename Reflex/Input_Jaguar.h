/*******************************************************************************
 * Reflex Adapt USB
 * Jaguar input module
 *
 * Uses JaguarLib
 * https://github.com/sonik-br/JaguarLib
 *
 * Uses a modified version of Joystick Library
 * https://github.com/MHeironimus/ArduinoJoystickLibrary
 *
*/

#include "src/JaguarLib/JaguarLib.h"
#include "src/ArduinoJoystickLibrary/Joy1.h"

//Jaguar pins
#define JAGPIN_J3_J4   13
#define JAGPIN_J2_J5   10
#define JAGPIN_J1_J6   7
#define JAGPIN_J0_J7   6
#define JAGPIN_B0_B2   5
#define JAGPIN_B1_B3   4
#define JAGPIN_J11_J15 9
#define JAGPIN_J10_J14 8
#define JAGPIN_J9_J13  22
#define JAGPIN_J8_J12  14

JagPort<JAGPIN_J3_J4, JAGPIN_J2_J5, JAGPIN_J1_J6, JAGPIN_J0_J7, JAGPIN_B0_B2, JAGPIN_B1_B3, JAGPIN_J11_J15, JAGPIN_J10_J14, JAGPIN_J9_J13, JAGPIN_J8_J12> jag1;

#ifdef ENABLE_REFLEX_PAD
  const Pad padJaguar[] = {
    { JAG_PAD_UP,    0, (1+4)*6, UP_ON, UP_OFF },
    { JAG_PAD_DOWN,  2, (1+4)*6, DOWN_ON, DOWN_OFF },
    { JAG_PAD_LEFT,  1, (0+4)*6, LEFT_ON, LEFT_OFF },
    { JAG_PAD_RIGHT, 1, (2+4)*6, RIGHT_ON, RIGHT_OFF },
    { JAG_B,         (3-2), (8+7)*6, FACEBTN_ON, FACEBTN_OFF },
    { JAG_A,         (3-3), (9+7)*6, FACEBTN_ON, FACEBTN_OFF },
    { JAG_C,         (3-1), (7+7)*6, FACEBTN_ON, FACEBTN_OFF },
    { JAG_PAUSE,     2-2, (4.5+5)*6, DASHBTN_ON, DASHBTN_OFF },
    { JAG_OPTION,    2-2, (5.5+5)*6, DASHBTN_ON, DASHBTN_OFF },
    //{ JAG_9,         2, 9*6, FACEBTN_ON, FACEBTN_OFF }, //X 9
    //{ JAG_8,         2, 8*6, FACEBTN_ON, FACEBTN_OFF }, //Y 8
    //{ JAG_7,         2, 7*6, FACEBTN_ON, FACEBTN_OFF }, //Z 7
    //{ JAG_6,         0, 8*6, SHOULDERBTN_ON, SHOULDERBTN_OFF }, //6 R
    //{ JAG_4,         0, 1*6, SHOULDERBTN_ON, SHOULDERBTN_OFF }, //4 L
    { JAG_1,         1, (12-3)*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
    { JAG_2,         1, (13-3)*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
    { JAG_3,         1, (14-3)*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
    { JAG_4,         2, (12-3)*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
    { JAG_5,         2, (13-3)*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
    { JAG_6,         2, (14-3)*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
    { JAG_7,         3, (12-3)*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
    { JAG_8,         3, (13-3)*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
    { JAG_9,         3, (14-3)*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
    { JAG_STAR,      4, (12-3)*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
    { JAG_0,         4, (13-3)*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
    { JAG_HASH,      4, (14-3)*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF }
  };

  void loopPadDisplayCharsJaguar(const uint8_t index, const JagDeviceType_Enum padType, const void* sc, const bool force) {
    for(uint8_t i = 0; i < (sizeof(padJaguar) / sizeof(Pad)); ++i){
      const Pad pad = padJaguar[i];
      PrintPadChar(index, padDivision[index].firstCol, pad.col, pad.row, pad.padvalue, !sc || static_cast<const JagController*>(sc)->digitalPressed(static_cast<JagDigital_Enum>(pad.padvalue)), pad.on, pad.off, force);
    }
  }

  void ShowDefaultPadJaguar(const uint8_t index, const JagDeviceType_Enum padType) {
    //print default joystick state to oled screen
  
    //const uint8_t firstCol = index == 0 ? 0 : 11*6;
    //const uint8_t lastCol = index == 0 ? 11*6 : 127;
  
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
      loopPadDisplayCharsJaguar(index, padType, NULL , true);
    }
  }
#endif


void jaguarResetJoyValues(const uint8_t i) {
  if (i >= totalUsb)
    return;
  usbStick[i]->resetState();
}

void jaguarSetup() {
  //Init onboard led pin
  //pinMode(LED_BUILTIN, OUTPUT);
  
  //Init the jaguar class
  jag1.begin();
  
  delay(5);

  totalUsb = 1;
  sleepTime = 100;
  
  //Create usb controllers
  for (uint8_t i = 0; i < totalUsb; i++) {
    usbStick[i] = new Joy1_("RZordJag", JOYSTICK_DEFAULT_REPORT_ID + i, JOYSTICK_TYPE_GAMEPAD, totalUsb);
  }
  
  //Set usb parameters and reset to default values
  for (uint8_t i = 0; i < totalUsb; i++) {
    jaguarResetJoyValues(i);
    usbStick[i]->sendState();
  }
  
  delay(50);
  
  dstart(115200);
  debugln (F("Powered on!"));
}

inline bool __attribute__((always_inline))
jaguarLoop() {
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
  
  for (uint8_t i = 0; i < joyCount; i++) {
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
        jaguarResetJoyValues(i);
        #ifdef ENABLE_REFLEX_PAD
          ShowDefaultPadJaguar(inputPort, sc.deviceType());
        #endif
      }
      
      const uint8_t hatData = sc.hat();
      uint32_t buttonData = 0;

      bitWrite(buttonData, 0, sc.digitalPressed(JAG_C));
      bitWrite(buttonData, 1, sc.digitalPressed(JAG_B));
      bitWrite(buttonData, 2, sc.digitalPressed(JAG_A));
      bitWrite(buttonData, 8, sc.digitalPressed(JAG_OPTION));
      bitWrite(buttonData, 9, sc.digitalPressed(JAG_PAUSE));
      bitWrite(buttonData, 10, sc.digitalPressed(JAG_1));
      bitWrite(buttonData, 11, sc.digitalPressed(JAG_2));
      bitWrite(buttonData, 12, sc.digitalPressed(JAG_3));
      bitWrite(buttonData, 4, sc.digitalPressed(JAG_4)); //Pro L
      bitWrite(buttonData, 13, sc.digitalPressed(JAG_5));
      bitWrite(buttonData, 5, sc.digitalPressed(JAG_6)); //Pro R
      bitWrite(buttonData, 3, sc.digitalPressed(JAG_7)); //Pro Z
      bitWrite(buttonData, 6, sc.digitalPressed(JAG_8)); //Pro Y
      bitWrite(buttonData, 7, sc.digitalPressed(JAG_9)); //Pro X
      bitWrite(buttonData, 14, sc.digitalPressed(JAG_STAR));
      bitWrite(buttonData, 15, sc.digitalPressed(JAG_0));
      bitWrite(buttonData, 16, sc.digitalPressed(JAG_HASH));
      
      ((Joy1_*)usbStick[i])->setButtons(buttonData);
      
      //remap hat data
      uint8_t newHatData = B0;
      bitWrite(newHatData, 0, bitRead(hatData, 3)); //U
      bitWrite(newHatData, 1, bitRead(hatData, 2)); //D
      bitWrite(newHatData, 2, bitRead(hatData, 1)); //L
      bitWrite(newHatData, 3, bitRead(hatData, 0)); //R
      
      //Get angle from hatTable and pass to joystick class
      ((Joy1_*)usbStick[i])->setHatSwitch(hatTable[newHatData]);
      
      usbStick[i]->sendState();
      
      #ifdef ENABLE_REFLEX_PAD
        if (inputPort < 2) {
          const JagDeviceType_Enum padType = sc.deviceType();
          loopPadDisplayCharsJaguar(inputPort, padType, &sc, false);
        }
      #endif
    }
  }
  
  #ifdef ENABLE_REFLEX_PAD
    for (uint8_t i = 0; i < 1; i++) {
      if(lastPadType[i] != currentPadType[i] && currentPadType[i] == JAG_DEVICE_NONE)
        ShowDefaultPadJaguar(i, JAG_DEVICE_NONE);
      lastPadType[i] = currentPadType[i];
    }
  #endif
  
  //Controller has been disconnected? Reset it's values!
  if (lastControllerCount > joyCount) {
    for (uint8_t i = joyCount; i < lastControllerCount; i++) {
      if (i == totalUsb)
        break;
      jaguarResetJoyValues(i);
      usbStick[i]->sendState();
    }
  }
  
  //Keep count for next read
  lastControllerCount = joyCount;

  return stateChanged;
}
