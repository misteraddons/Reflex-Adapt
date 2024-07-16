/*******************************************************************************
 * PlayStation (NEGCON) input module for RetroZord / Reflex-Adapt.
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
    
    void negconSetup() {
      totalUsb = 2;//MAX_USB_STICKS;

//      for (uint8_t i = 0; i < totalUsb; i++) {
//        usbStick[i] = new Joy1_(isNeGconMiSTer ? "RZordPsWheel" : "RZordPsNeGcon", JOYSTICK_DEFAULT_REPORT_ID + i, JOYSTICK_TYPE_GAMEPAD, totalUsb,
//          true,//includeXAxis,
//          false,//includeYAxis,
//          true,//includeZAxis,joy
//          false,//includeRzAxis,
//          true,//includeThrottle,
//          true,//includeBrake,
//          isNeGconMiSTer);//includeSteering
//      }
    
      //initial values
//      for (uint8_t i = 0; i < totalUsb; i++) {
//        usbStick[i]->resetState();
//        ((Joy1_*)usbStick[i])->setAnalog0(ANALOG_IDLE_VALUE); //x
//        ((Joy1_*)usbStick[i])->setAnalog1(ANALOG_MAX_VALUE); //z (rx)
//        ((Joy1_*)usbStick[i])->setAnalog2(ANALOG_MAX_VALUE); //I throttle
//        ((Joy1_*)usbStick[i])->setAnalog3(ANALOG_MAX_VALUE); //II brake
//        ((Joy1_*)usbStick[i])->setAnalog4(ANALOG_IDLE_VALUE); //paddle
//        usbStick[i]->sendState();
//      }
    }
    void negconSetup2() {
      //change hid mode
      if (isNeGconMiSTer && canChangeMode()) {
        options.inputMode = INPUT_MODE_HID_NEGCON;
      }
    }

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
      
      bool stateChanged = psx->buttonsChanged();//check if any digital value changed (dpad and buttons)
    
      byte lx, ly;
      psx->getLeftAnalog (lx, ly);

      const bool isXinput = options.inputMode == INPUT_MODE_XINPUT || options.inputMode == INPUT_MODE_HID || (isPS3 && options.inputMode == INPUT_MODE_SWITCH);
      const bool analogTriggers = canUseAnalogTrigger();
    
      //uses inversed values
      const uint8_t z = (uint8_t)~psx->getAnalogButton(PSAB_L1);
      const uint8_t cross =  isXinput ? psx->getAnalogButton(PSAB_CROSS)  : (uint8_t)~psx->getAnalogButton(PSAB_CROSS);
      const uint8_t square = isXinput ? psx->getAnalogButton(PSAB_SQUARE) : (uint8_t)~psx->getAnalogButton(PSAB_SQUARE);
    
      state[outputIndex].lx = convertAnalog(lx, false); //steering

      hasLeftAnalogStick[outputIndex] = true;

      if (options.inputMode == INPUT_MODE_HID_NEGCON) {
        hasAnalogTriggers[outputIndex] = true;
        hasRightAnalogStick[outputIndex] = true;
        
        state[outputIndex].ly = convertAnalog(z, false);//z

        state[outputIndex].rx = convertAnalog(cross, false);  //I throttle
        state[outputIndex].ry = convertAnalog(square, false); //II brake

        state[outputIndex].buttons = 0
          | (psx->buttonPressed(PSB_TRIANGLE) ? GAMEPAD_MASK_B1 : 0) // Generic: K1, Switch: B, Xbox: A
          | (psx->buttonPressed(PSB_R1)       ? GAMEPAD_MASK_B2 : 0) // Generic: K2, Switch: A, Xbox: B
          | (psx->buttonPressed(PSB_CIRCLE)   ? GAMEPAD_MASK_B3 : 0) // Generic: P1, Switch: Y, Xbox: X
          | (psx->buttonPressed(PSB_START)    ? GAMEPAD_MASK_B4 : 0) // Generic: P2, Switch: X, Xbox: Y
          //| (psx->buttonPressed(PSB_L1)       ? GAMEPAD_MASK_L1 : 0) // Generic: P4, Switch: L, Xbox: LB
          //| (psx->buttonPressed(PSB_R1)       ? GAMEPAD_MASK_R1 : 0) // Generic: P3, Switch: R, Xbox: RB
          //| (psx->buttonPressed(PSB_L2)       ? GAMEPAD_MASK_L2 : 0) // Generic: K4, Switch: ZL, Xbox: LT (Digital)
          //| (psx->buttonPressed(PSB_R2)       ? GAMEPAD_MASK_R2 : 0) // Generic: K3, Switch: ZR, Xbox: RT (Digital)
          //| (psx->buttonPressed(PSB_SELECT)   ? GAMEPAD_MASK_S1 : 0) // Generic: Select, Switch: -, Xbox: View
          //| (psx->buttonPressed(PSB_START)    ? GAMEPAD_MASK_S2 : 0) // Generic: Start, Switch: +, Xbox: Menu
          //| (psx->buttonPressed(PSB_L3)       ? GAMEPAD_MASK_L3 : 0) // All: Left Stick Click
          //| (psx->buttonPressed(PSB_R3)       ? GAMEPAD_MASK_R3 : 0) // All: Right Stick Click
        ;
        
      } else { //xinput,switch,ps3...
        hasRightAnalogStick[outputIndex] = false;
        
        if (analogTriggers) {//xinput,ps3
          hasAnalogTriggers[outputIndex] = true;
          state[outputIndex].rt = cross;  //I throttle
          state[outputIndex].lt = square; //II brake

          state[outputIndex].buttons = 0
            | (psx->buttonPressed(PSB_CIRCLE)   ? GAMEPAD_MASK_B1 : 0) // Generic: K1, Switch: B, Xbox: A
            | (psx->buttonPressed(PSB_TRIANGLE)   ? GAMEPAD_MASK_B2 : 0) // Generic: K2, Switch: A, Xbox: B
            //| (psx->buttonPressed(PSB_CIRCLE)   ? GAMEPAD_MASK_B3 : 0) // Generic: P1, Switch: Y, Xbox: X
            //| (psx->buttonPressed(PSB_START)    ? GAMEPAD_MASK_B4 : 0) // Generic: P2, Switch: X, Xbox: Y
            | (psx->buttonPressed(PSB_L1)       ? GAMEPAD_MASK_L1 : 0) // Generic: P4, Switch: L, Xbox: LB
            | (psx->buttonPressed(PSB_R1)       ? GAMEPAD_MASK_R1 : 0) // Generic: P3, Switch: R, Xbox: RB
            //| (psx->buttonPressed(PSB_L2)       ? GAMEPAD_MASK_L2 : 0) // Generic: K4, Switch: ZL, Xbox: LT (Digital)
            //| (psx->buttonPressed(PSB_R2)       ? GAMEPAD_MASK_R2 : 0) // Generic: K3, Switch: ZR, Xbox: RT (Digital)
            //| (psx->buttonPressed(PSB_SELECT)   ? GAMEPAD_MASK_S1 : 0) // Generic: Select, Switch: -, Xbox: View
            | (psx->buttonPressed(PSB_START)    ? GAMEPAD_MASK_S2 : 0) // Generic: Start, Switch: +, Xbox: Menu
            //| (psx->buttonPressed(PSB_L3)       ? GAMEPAD_MASK_L3 : 0) // All: Left Stick Click
            //| (psx->buttonPressed(PSB_R3)       ? GAMEPAD_MASK_R3 : 0) // All: Right Stick Click
          ;

        } else { //swtich
          hasAnalogTriggers[outputIndex] = false;

          state[outputIndex].buttons = 0
            | (psx->buttonPressed(PSB_TRIANGLE)    ? GAMEPAD_MASK_B1 : 0) // Generic: K1, Switch: B, Xbox: A
            | (psx->buttonPressed(PSB_CIRCLE)   ? GAMEPAD_MASK_B2 : 0) // Generic: K2, Switch: A, Xbox: B
            //| (psx->buttonPressed(PSB_SQUARE)   ? GAMEPAD_MASK_B3 : 0) // Generic: P1, Switch: Y, Xbox: X
            //| (psx->buttonPressed(PSB_CROSS) ? GAMEPAD_MASK_B4 : 0) // Generic: P2, Switch: X, Xbox: Y
            | (psx->buttonPressed(PSB_L1)       ? GAMEPAD_MASK_L1 : 0) // Generic: P4, Switch: L, Xbox: LB
            | (psx->buttonPressed(PSB_R1)       ? GAMEPAD_MASK_R1 : 0) // Generic: P3, Switch: R, Xbox: RB
            | (psx->buttonPressed(PSB_SQUARE)   ? GAMEPAD_MASK_L2 : 0) // Generic: K4, Switch: ZL, Xbox: LT (Digital)
            | (psx->buttonPressed(PSB_CROSS)    ? GAMEPAD_MASK_R2 : 0) // Generic: K3, Switch: ZR, Xbox: RT (Digital)
            //| (psx->buttonPressed(PSB_SELECT)   ? GAMEPAD_MASK_S1 : 0) // Generic: Select, Switch: -, Xbox: View
            | (psx->buttonPressed(PSB_START)    ? GAMEPAD_MASK_S2 : 0) // Generic: Start, Switch: +, Xbox: Menu
            //| (psx->buttonPressed(PSB_L3)       ? GAMEPAD_MASK_L3 : 0) // All: Left Stick Click
            //| (psx->buttonPressed(PSB_R3)       ? GAMEPAD_MASK_R3 : 0) // All: Right Stick Click
          ;
        }
        
        state[outputIndex].ly = convertAnalog(0x80);
      }



//todo use this:
//      ((Joy1_*)usbStick[outputIndex])->setAnalog0(lx); //steering
//      ((Joy1_*)usbStick[outputIndex])->setAnalog1(z); //z
//      ((Joy1_*)usbStick[outputIndex])->setAnalog2(cross); //I throttle
//      ((Joy1_*)usbStick[outputIndex])->setAnalog3(square); //II brake
//      if(isNeGconMiSTer)
//        ((Joy1_*)usbStick[0])->setAnalog4(lx); //paddle

      
      
      //state[outputIndex].rx = convertAnalog(cross);//I throttle
      //state[outputIndex].ry = convertAnalog(square);//II brake

    
      //combine the two axes and use only half precision for each
      //const int btnI_II = psx.getAnalogButton(PSAB_SQUARE) - psx.getAnalogButton(PSAB_CROSS);
      //const uint8_t btnI_II = ((psx.getAnalogButton(PSAB_SQUARE) - psx.getAnalogButton(PSAB_CROSS)) >> 1) + ANALOG_IDLE_VALUE;
    
//      uint8_t buttonData = 0;
//      bitWrite(buttonData, 0, psx->buttonPressed(PSB_CIRCLE)); //A
//      bitWrite(buttonData, 1, psx->buttonPressed(PSB_TRIANGLE)); //B
//      bitWrite(buttonData, 2, psx->buttonPressed(PSB_R1));
//      bitWrite(buttonData, 3, psx->buttonPressed(PSB_START)); //Start


      handleDpad();
    
      if (lastX[outputIndex] != lx || lastZ[outputIndex] != z || lastCross[outputIndex] != cross || lastSquare[outputIndex] != square)
        stateChanged = true;
    
      lastX[outputIndex] = lx;
      lastZ[outputIndex] = z;
      lastCross[outputIndex] = cross;
      lastSquare[outputIndex] = square;
    
      if(stateChanged) {
    
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
      return stateChanged;
    }
