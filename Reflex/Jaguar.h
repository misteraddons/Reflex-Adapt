/*******************************************************************************
 * Jaguar controllers to USB using an Arduino Leonardo.
 *
 * Works with digital pad.
 *
 * For details on Joystick Library, see
 * https://github.com/MHeironimus/ArduinoJoystickLibrary
 *
*/

#include "src/JaguarLib/JaguarLib.h"
#include "src/ArduinoJoystickLibrary/Joy1.h"

//Jaguar pins - Port 1
#define JAGPIN_J3_J4   13 //h1  // ORIGINALLY PIN 9
#define JAGPIN_J2_J5   10 //h2   // ORIGINALLY PIN 8
#define JAGPIN_J1_J6   7 //h3
#define JAGPIN_J0_J7   6 //h4
#define JAGPIN_B0_B2   5 //h5
#define JAGPIN_B1_B3   4 //h6
#define JAGPIN_J11_J15 9 //h7  // ORIGINALLY PIN 13
#define JAGPIN_J10_J14 8 //h8 PS1ACK  // ORIGINALLY PIN 10
#define JAGPIN_J9_J13  22 //h9 PS1GUN
#define JAGPIN_J8_J12  14 //12 PS1DATA

JagPort<JAGPIN_J3_J4, JAGPIN_J2_J5, JAGPIN_J1_J6, JAGPIN_J0_J7, JAGPIN_B0_B2, JAGPIN_B1_B3, JAGPIN_J11_J15, JAGPIN_J10_J14, JAGPIN_J9_J13, JAGPIN_J8_J12> jag1;

#ifdef ENABLE_REFLEX_PAD
  const Pad padJaguar[] = {
    { JAG_PAD_UP,    1, 1*6, UP_ON, UP_OFF },
    { JAG_PAD_DOWN,  3, 1*6, DOWN_ON, DOWN_OFF },
    { JAG_PAD_LEFT,  2, 0,   LEFT_ON, LEFT_OFF },
    { JAG_PAD_RIGHT, 2, 2*6, RIGHT_ON, RIGHT_OFF },
    { JAG_B,         3, 8*6, FACEBTN_ON, FACEBTN_OFF },
    { JAG_A,         3, 9*6, FACEBTN_ON, FACEBTN_OFF },
    { JAG_C,         3, 7*6, FACEBTN_ON, FACEBTN_OFF },
    { JAG_PAUSE,     2, 4*6, DASHBTN_ON, DASHBTN_OFF },
    { JAG_OPTION,    2, 5*6, DASHBTN_ON, DASHBTN_OFF },
    //{ JAG_9,         2, 9*6, FACEBTN_ON, FACEBTN_OFF }, //X 9
    //{ JAG_8,         2, 8*6, FACEBTN_ON, FACEBTN_OFF }, //Y 8
    //{ JAG_7,         2, 7*6, FACEBTN_ON, FACEBTN_OFF }, //Z 7
    //{ JAG_6,         0, 8*6, SHOULDERBTN_ON, SHOULDERBTN_OFF }, //6 R
    //{ JAG_4,         0, 1*6, SHOULDERBTN_ON, SHOULDERBTN_OFF }, //4 L
    { JAG_1,         0, 12*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
    { JAG_2,         0, 13*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
    { JAG_3,         0, 14*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
    { JAG_4,         1, 12*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
    { JAG_5,         1, 13*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
    { JAG_6,         1, 14*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
    { JAG_7,         2, 12*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
    { JAG_8,         2, 13*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
    { JAG_9,         2, 14*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
    { JAG_STAR,      3, 12*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
    { JAG_0,         3, 13*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
    { JAG_HASH,      3, 14*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF }
  };

  void ShowDefaultPadJaguar(const uint8_t index, const JagDeviceType_Enum padType) {
    //print default joystick state to oled screen
  
    //const uint8_t firstCol = index == 0 ? 0 : 11*6;
    //const uint8_t lastCol = index == 0 ? 11*6 : 127;
  
    display.clear(padDivision[index].firstCol, padDivision[index].lastCol, oledDisplayFirstRow + 1, 7);
    display.setCursor(padDivision[index].firstCol, 7);
    
    switch(padType) {
      case JAG_DEVICE_PAD:
        display.print(F("DIGITAL"));
        break;
      default:
        display.print(F("NONE"));
        return;
    }
  
    if (index < 2) {
      //const uint8_t startCol = index == 0 ? 0 : 11*6;
      for(uint8_t x = 0; x < 21; x++){
        const Pad pad = padJaguar[x];
        PrintPadChar(index, padDivision[index].firstCol, pad.col, pad.row, (JagDigital_Enum)pad.padvalue, true, pad.on, pad.off, true);
      }
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
          //const uint8_t startCol = inputPort == 0 ? 0 : 11*6;
          //const JagDeviceType_Enum padType = sc.deviceType();
          for(uint8_t x = 0; x < 21; x++){
            const Pad pad = padJaguar[x];
            PrintPadChar(inputPort, padDivision[inputPort].firstCol, pad.col, pad.row, pad.padvalue, sc.digitalPressed((JagDigital_Enum)pad.padvalue), pad.on, pad.off);
          }
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

  return stateChanged; //joyCount != 0;
}
