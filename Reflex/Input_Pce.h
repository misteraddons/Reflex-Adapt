/*******************************************************************************
 * NEC PC Engine controllers to USB using an Arduino Leonardo.
 *
 * Works with 2btn and 6btn digital pad, multitap.
 *
 * For details on Joystick Library, see
 * https://github.com/MHeironimus/ArduinoJoystickLibrary
 *
*/

#include "src/PceLib/PceLib.h"
#include "src/ArduinoJoystickLibrary/Joy1.h"

//PCE pins - Port 1
/*#define PCE1_SEL A2
#define PCE1_CLR 7
#define PCE1_D0 A4 //U 1 3
#define PCE1_D1 A5 //R 2 4
#define PCE1_D2 A3 //D S 5
#define PCE1_D3 A0 //L R 6*/

//PCE pins - Port 1
#define PCE1_SEL 5
#define PCE1_CLR 4
#define PCE1_D0  9 //U 1 3
#define PCE1_D1  8 //R 2 4
#define PCE1_D2  7 //D S 5
#define PCE1_D3  6 //L R 6

//PCE pins - Port 2
#define PCE2_SEL 14
#define PCE2_CLR 16
#define PCE2_D0  20 //U 1 3
#define PCE2_D1  19 //R 2 4
#define PCE2_D2  18 //D S 5
#define PCE2_D3  15 //L R 6

PcePort<PCE1_SEL, PCE1_CLR, PCE1_D0, PCE1_D1, PCE1_D2, PCE1_D3> pce1;
PcePort<PCE2_SEL, PCE2_CLR, PCE2_D0, PCE2_D1, PCE2_D2, PCE2_D3> pce2;

#ifdef ENABLE_REFLEX_PAD
  const Pad padPce[] = {
    { PCE_PAD_UP,    1, 1*6, UP_ON, UP_OFF },
    { PCE_PAD_RIGHT, 2, 2*6, RIGHT_ON, RIGHT_OFF },
    { PCE_PAD_DOWN,  3, 1*6, DOWN_ON, DOWN_OFF },
    { PCE_PAD_LEFT,  2, 0,   LEFT_ON, LEFT_OFF },
    { PCE_1,         3, 9*6, FACEBTN_ON, FACEBTN_OFF },
    { PCE_2,         3, 8*6, FACEBTN_ON, FACEBTN_OFF },
    { PCE_SELECT,    2, 4*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
    { PCE_RUN,       2, 5*6, RECTANGLEBTN_ON, RECTANGLEBTN_OFF },
    { PCE_3,         3, 7*6, FACEBTN_ON, FACEBTN_OFF },
    { PCE_4,         2, 7*6, FACEBTN_ON, FACEBTN_OFF },
    { PCE_5,         2, 8*6, FACEBTN_ON, FACEBTN_OFF },
    { PCE_6,         2, 9*6, FACEBTN_ON, FACEBTN_OFF }
  };
  
  void ShowDefaultPadPce(const uint8_t index, PceDeviceType_Enum padType) {
    //If multitap support is enabled, force display as 2btn pad
    #ifdef PCE_ENABLE_MULTITAP
      if(padType == PCE_DEVICE_NONE)
        padType = PCE_DEVICE_PAD2;
    #endif
    //print default joystick state to oled screen
  
    //const uint8_t firstCol = index == 0 ? 0 : 11*6;
    //const uint8_t lastCol = index == 0 ? 11*6 : 127;
  
    display.clear(padDivision[index].firstCol, padDivision[index].lastCol, oledDisplayFirstRow + 1, 7);
    display.setCursor(padDivision[index].firstCol, 7);
    
    switch(padType) {
      case PCE_DEVICE_PAD2:
        display.print(F("PCE-2"));
        break;
      case PCE_DEVICE_PAD6:
        display.print(F("PCE-6"));
        break;
      default:
        display.print(PSTR_TO_F(PSTR_NONE));
        return;
    }
  
    if (index < 2) {
      //const uint8_t startCol = index == 0 ? 0 : 11*6;
      for(uint8_t x = 0; x < 12; x++){
        if(padType == PCE_DEVICE_PAD2 && x > 7)
          continue;
        const Pad pad = padPce[x];
        PrintPadChar(index, padDivision[index].firstCol, pad.col, pad.row, pad.padvalue, true, pad.on, pad.off, true);
      }
    }
  }
#endif


void pceResetJoyValues(const uint8_t i) {
  if (i >= totalUsb)
    return;
  usbStick[i]->resetState();
}

void pceSetup() {
  //Init onboard led pin
  //pinMode(LED_BUILTIN, OUTPUT);
  
  //Init the pce class
  pce1.begin();
  pce2.begin();
  
  delayMicroseconds(10);
  
  //Detect multitap on first port
  pce1.detectMultitap();
  
  const uint8_t tap = pce1.getMultitapPorts();//pce1.getMultitapPorts() + pce2.getMultitapPorts();
 
  if (tap == 0) //No multitap connected during boot
    totalUsb = 2;
  else //Multitap connected.
    totalUsb = min(tap + 1, MAX_USB_STICKS);

  /*#ifdef PCE_ENABLE_MULTITAP
    totalUsb = min(TAP_PCE_PORTS + 1, MAX_USB_STICKS);
  #else
    totalUsb = 2;
  #endif*/
  
  //Create usb controllers
  for (uint8_t i = 0; i < totalUsb; i++) {
    usbStick[i] = new Joy1_("RZordPce", JOYSTICK_DEFAULT_REPORT_ID + i, JOYSTICK_TYPE_GAMEPAD, totalUsb);
  }
  
  //Set usb parameters and reset to default values
  for (uint8_t i = 0; i < totalUsb; i++) {
    pceResetJoyValues(i);
    usbStick[i]->sendState();
  }

  sleepTime = 50;
  
  delay(50);
  
  dstart(115200);
  debugln (F("Powered on!"));
}

inline bool __attribute__((always_inline))
pceLoop() {
  static uint8_t lastControllerCount = 0;
  #ifdef ENABLE_REFLEX_PAD
    static PceDeviceType_Enum lastPadType[] = { PCE_DEVICE_NOTSUPPORTED, PCE_DEVICE_NOTSUPPORTED, PCE_DEVICE_NOTSUPPORTED, PCE_DEVICE_NOTSUPPORTED, PCE_DEVICE_NOTSUPPORTED, PCE_DEVICE_NOTSUPPORTED };
    PceDeviceType_Enum currentPadType[] = { PCE_DEVICE_NONE, PCE_DEVICE_NONE, PCE_DEVICE_NONE, PCE_DEVICE_NONE, PCE_DEVICE_NONE, PCE_DEVICE_NONE };

    static bool firstTime = true;
    if(firstTime) {
      firstTime = false;
      //Multitap mode!
      if(totalUsb > 2) {
        display.setCursor(5*6, oledDisplayFirstRow + 3);
        display.print(F("MULTI-TAP"));
      }
    }
      
  #endif
  bool stateChanged = false;
  
  //Read each pce port
  pce1.update();
  pce2.update();

  //Get the number of connected controllers on each port
  const uint8_t joyCount1 = pce1.getControllerCount();
  const uint8_t joyCount2 = pce2.getControllerCount();
  const uint8_t joyCount = joyCount1 + joyCount2;
  
  for (uint8_t i = 0; i < joyCount; i++) {
    if (i == totalUsb)
      break;
    
    //Get the data for the specific controller from a port
    const uint8_t inputPort = (i < joyCount1) ? 0 : 1;
    const PceController& sc = inputPort == 0 ? pce1.getPceController(i) : pce2.getPceController(i - joyCount1);

    #ifdef ENABLE_REFLEX_PAD
      //Only used if not in multitap mode
      if(totalUsb == 2) {
        if(inputPort == 0 && joyCount1 != 0)
          currentPadType[inputPort] = sc.deviceType();
        else if(inputPort == 1 && joyCount2 != 0)
          currentPadType[inputPort] = sc.deviceType();
      }
    #endif
    
    //Only process data if state changed from previous read
    if (sc.stateChanged()) {
      stateChanged = true;
      
      //Controller just connected. Also can happen when changing mode on 6btn pad
      if (sc.deviceJustChanged()) {
        pceResetJoyValues(i);
        #ifdef ENABLE_REFLEX_PAD
          //Only used if not in multitap mode
          if (totalUsb == 2)
            ShowDefaultPadPce(inputPort, sc.deviceType());
        #endif
      }
      
      const uint8_t hatData = sc.hat();
      uint16_t buttonData = 0;

      bitWrite(buttonData, 2, sc.digitalPressed(PCE_2));
      bitWrite(buttonData, 5, sc.digitalPressed(PCE_1));
      bitWrite(buttonData, 8, sc.digitalPressed(PCE_SELECT));
      bitWrite(buttonData, 9, sc.digitalPressed(PCE_RUN));
      
      if (sc.deviceType() == PCE_DEVICE_PAD6) {
        bitWrite(buttonData, 1, sc.digitalPressed(PCE_3));
        bitWrite(buttonData, 0, sc.digitalPressed(PCE_4));
        bitWrite(buttonData, 3, sc.digitalPressed(PCE_5));
        bitWrite(buttonData, 4, sc.digitalPressed(PCE_6));
      }

      ((Joy1_*)usbStick[i])->setButtons(buttonData);
      
      //remap hat data
      uint8_t newHatData = B0;
      bitWrite(newHatData, 0, bitRead(hatData, 0)); //U
      bitWrite(newHatData, 1, bitRead(hatData, 2)); //D
      bitWrite(newHatData, 2, bitRead(hatData, 3)); //L
      bitWrite(newHatData, 3, bitRead(hatData, 1)); //R
      
      //Get angle from hatTable and pass to joystick class
      ((Joy1_*)usbStick[i])->setHatSwitch(hatTable[newHatData]);
      
      usbStick[i]->sendState();
      
      #ifdef ENABLE_REFLEX_PAD
        //Only used if not in multitap mode
        if (totalUsb == 2 && inputPort < 2) {
          //const uint8_t startCol = inputPort == 0 ? 0 : 11*6;
          const PceDeviceType_Enum padType = sc.deviceType();
          for(uint8_t x = 0; x < 12; x++){
            if(padType == PCE_DEVICE_PAD2 && x > 7)
                continue;
            const Pad pad = padPce[x];
            PrintPadChar(inputPort, padDivision[inputPort].firstCol, pad.col, pad.row, pad.padvalue, sc.digitalPressed((PceDigital_Enum)pad.padvalue), pad.on, pad.off);
          }
        }
      #endif
    }
  }
  
  #ifdef ENABLE_REFLEX_PAD
    for (uint8_t i = 0; i < 2; i++) {
      //Only used if not in multitap mode
      if(totalUsb == 2 && lastPadType[i] != currentPadType[i] && currentPadType[i] == PCE_DEVICE_NONE)
        ShowDefaultPadPce(i, PCE_DEVICE_NONE);
      lastPadType[i] = currentPadType[i];
    }
  #endif
  
  //Controller has been disconnected? Reset it's values!
  if (lastControllerCount > joyCount) {
    for (uint8_t i = joyCount; i < lastControllerCount; i++) {
      if (i == totalUsb)
        break;
      pceResetJoyValues(i);
      usbStick[i]->sendState();
    }
  }
  
  //Keep count for next read
  lastControllerCount = joyCount;

  /*
  const uint8_t multitapPorts = 0;//pce1.getMultitapPorts() + pce2.getMultitapPorts();
  if (multitapPorts != 0) //todo implement multitap support
    sleepTime = (joyCount + 1) * 500;
  else
    sleepTime = 50;
  */
  
  return stateChanged; //joyCount != 0;
}
