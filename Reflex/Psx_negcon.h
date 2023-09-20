/*******************************************************************************
 * Reflex Adapt USB
 * PSX input module - NeGcon support
 * 
*/

bool loopNeGcon() {
  static byte lastX[] = { ANALOG_IDLE_VALUE, ANALOG_IDLE_VALUE };
  static uint8_t lastZ[] = { ANALOG_MAX_VALUE, ANALOG_MAX_VALUE };
  static uint8_t lastCross[] = { ANALOG_MAX_VALUE, ANALOG_MAX_VALUE };
  static uint8_t lastSquare[] = { ANALOG_MAX_VALUE, ANALOG_MAX_VALUE };

  #ifdef ENABLE_REFLEX_PAD
    static bool firstTime[] = {true, true };
    const uint8_t inputPort = outputIndex; //todo fix
    if(firstTime[inputPort]) {
      firstTime[inputPort] = false;
      ShowDefaultPadPsx(inputPort, PSPROTO_NEGCON);
    }
  #endif
  
  const bool digitalStateChanged = psx->buttonsChanged();//check if any digital value changed (dpad and buttons)
  bool stateChanged = digitalStateChanged;

  byte lx, ly;
  psx->getLeftAnalog (lx, ly);

  //uses inversed values
  const uint8_t z = (uint8_t)~psx->getAnalogButton(PSAB_L1);
  const uint8_t cross = (uint8_t)~psx->getAnalogButton(PSAB_CROSS);
  const uint8_t square = (uint8_t)~psx->getAnalogButton(PSAB_SQUARE);


  ((Joy1_*)usbStick[outputIndex])->setAnalog0(lx); //steering
  
  ((Joy1_*)usbStick[outputIndex])->setAnalog1(z); //z
  ((Joy1_*)usbStick[outputIndex])->setAnalog2(cross); //I throttle
  ((Joy1_*)usbStick[outputIndex])->setAnalog3(square); //II brake

  if(isNeGconMiSTer)
    ((Joy1_*)usbStick[0])->setAnalog4(lx); //paddle

  //combine the two axes and use only half precision for each
  //const int btnI_II = psx.getAnalogButton(PSAB_SQUARE) - psx.getAnalogButton(PSAB_CROSS);
  //const uint8_t btnI_II = ((psx.getAnalogButton(PSAB_SQUARE) - psx.getAnalogButton(PSAB_CROSS)) >> 1) + ANALOG_IDLE_VALUE;

  uint8_t buttonData = 0;
  bitWrite(buttonData, 0, psx->buttonPressed(PSB_CIRCLE)); //A
  bitWrite(buttonData, 1, psx->buttonPressed(PSB_TRIANGLE)); //B
  bitWrite(buttonData, 2, psx->buttonPressed(PSB_R1));
  bitWrite(buttonData, 3, psx->buttonPressed(PSB_START)); //Start

  ((Joy1_*)usbStick[outputIndex])->setButtons(buttonData);
  
  //uint8_t hat = (psx.getButtonWord() >> 4) & B00001111;
  //usbStick[0]->setHatSwitch(0, hatTable[hat]);
  handleDpad();

  if (lastX[outputIndex] != lx || lastZ[outputIndex] != z || lastCross[outputIndex] != cross || lastSquare[outputIndex] != square)
    stateChanged = true;

  lastX[outputIndex] = lx;
  lastZ[outputIndex] = z;
  lastCross[outputIndex] = cross;
  lastSquare[outputIndex] = square;

  if(stateChanged) {
    usbStick[outputIndex]->sendState();

    #ifdef ENABLE_REFLEX_PAD
      if (inputPort < 2) {
        //const uint8_t startCol = inputPort == 0 ? 0 : 11*6;
        for(uint8_t x = 3; x < 16; x++){
          const Pad pad = padPsx[x];
          if(x == 8 || x == 9)
            continue;
          PrintPadChar(inputPort, padDivision[inputPort].firstCol, pad.col, pad.row, pad.padvalue, psx->buttonPressed((PsxButton)pad.padvalue), pad.on, pad.off);
        }
      }
    #endif
  }
  return digitalStateChanged;
}

void negconSetup() {
  totalUsb = 2;//MAX_USB_STICKS;
  for (uint8_t i = 0; i < totalUsb; i++) {
    usbStick[i] = new Joy1_(isNeGconMiSTer ? "ReflexPSWheel" : "ReflexPSneGcon", JOYSTICK_DEFAULT_REPORT_ID + i, JOYSTICK_TYPE_GAMEPAD, totalUsb,
      true,//includeXAxis,
      false,//includeYAxis,
      true,//includeZAxis,joy
      false,//includeRzAxis,
      true,//includeThrottle,
      true,//includeBrake,
      isNeGconMiSTer);//includeSteering
  }

  //initial values
  for (uint8_t i = 0; i < totalUsb; i++) {
    usbStick[i]->resetState();
    ((Joy1_*)usbStick[i])->setAnalog0(ANALOG_IDLE_VALUE); //x
    ((Joy1_*)usbStick[i])->setAnalog1(ANALOG_MAX_VALUE); //z (rx)
    ((Joy1_*)usbStick[i])->setAnalog2(ANALOG_MAX_VALUE); //I throttle
    ((Joy1_*)usbStick[i])->setAnalog3(ANALOG_MAX_VALUE); //II brake
    ((Joy1_*)usbStick[i])->setAnalog4(ANALOG_IDLE_VALUE); //paddle
    usbStick[i]->sendState();
  }
}
