void loopNeGcon() {
  byte lx, ly;
  psx->getLeftAnalog (lx, ly);

  ((Joy1_*)usbStick[outputIndex])->setAnalog0(lx); //steering
  
  //uses inversed values
  ((Joy1_*)usbStick[outputIndex])->setAnalog1((uint8_t) ~ psx->getAnalogButton(PSAB_L1)); //z
  ((Joy1_*)usbStick[outputIndex])->setAnalog2((uint8_t) ~ psx->getAnalogButton(PSAB_CROSS)); //I throttle
  ((Joy1_*)usbStick[outputIndex])->setAnalog3((uint8_t) ~ psx->getAnalogButton(PSAB_SQUARE)); //II brake

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
  
  usbStick[outputIndex]->sendState();
}

void negconSetup() {
  totalUsb = 2;//MAX_USB_STICKS;
  for (uint8_t i = 0; i < totalUsb; i++) {
    usbStick[i] = new Joy1_(isNeGconMiSTer ? "RZordPsWheel" : "RZordPsNeGcon", JOYSTICK_DEFAULT_REPORT_ID + i, JOYSTICK_TYPE_GAMEPAD, totalUsb,
      true,//includeXAxis,
      false,//includeYAxis,
      true,//includeZAxis,
      false,//includeRxAxis,
      false,//includeRyAxis,
      true,//includeThrottle,
      true,//includeBrake,
      isNeGconMiSTer);//includeSteering
  }

  //initial values
  for (uint8_t i = 0; i < totalUsb; i++) {
    usbStick[i]->resetState();
    ((Joy1_*)usbStick[i])->setAnalog0(ANALOG_IDLE_VALUE); //x
    ((Joy1_*)usbStick[i])->setAnalog1(ANALOG_MAX_VALUE); //z
    ((Joy1_*)usbStick[i])->setAnalog2(ANALOG_MAX_VALUE); //I throttle
    ((Joy1_*)usbStick[i])->setAnalog3(ANALOG_MAX_VALUE); //II brake
    ((Joy1_*)usbStick[i])->setAnalog4(ANALOG_IDLE_VALUE); //paddle
    usbStick[i]->sendState();
  }
}
