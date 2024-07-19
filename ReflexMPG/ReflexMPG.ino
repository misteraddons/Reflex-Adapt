/*******************************************************************************
 * LUFA based output for RetroZord / Reflex-Adapt.
 * By Matheus Fraguas (sonik-br)
 *
 * Uses a modified version of MPG
 * https://github.com/sonik-br/MPG
 *
*/

#define DEBOUNCE_MILLIS 0

#define REFLEX_VERSION -2 // V2 - mode button. oled display.

#define ENABLE_REFLEX_LOGO //reflex logo on oled display
#define ENABLE_REFLEX_PAD //pad on oled display

// DONT USE THIS. ENABLE MODES DOWN BELOW AT REFLEX_NO_DEFAULTS
//#define ENABLE_REFLEX_SATURN
//#define ENABLE_REFLEX_SNES
//#define ENABLE_REFLEX_PSX
//#define ENABLE_REFLEX_PSX_JOG //this is for jogcon forced specific mode. jogcon can still be used with ENABLE_REFLEX_PSX
//#define ENABLE_REFLEX_PCE
//#define ENABLE_REFLEX_NEOGEO
//#define ENABLE_REFLEX_3DO
//#define ENABLE_REFLEX_JAGUAR
//#define ENABLE_REFLEX_N64
//#define ENABLE_REFLEX_GAMECUBE
//#define ENABLE_REFLEX_WII
//#define ENABLE_REFLEX_SMS
//#define ENABLE_REFLEX_JPC

// Sega MegaDrive/Saturn config
#define SATLIB_ENABLE_8BITDO_HOME_BTN // support for HOME button on 8bidto M30 2.4G.
// #define SATLIB_ENABLE_MEGATAP //suport for 4p megatap
// #define SATLIB_ENABLE_SATTAP //support for 6p multitap

// SNES config
//#define SNES_ENABLE_VBOY
//#define SNES_ENABLE_MULTITAP

// PS1 Guncon config. ONLY MODE 3 IS IMPLEMENTED
// 0=Mouse, 1=Joy, 2=Joy OffScreenEdge (MiSTer), 3=Guncon Raw (MiSTer)
//#define GUNCON_FORCE_MODE 3

// PS1 NeGcon HID config
// 0=Default, 1=MiSTer Wheel format with paddle
//#define NEGCON_FORCE_MODE 1

// PS1 Rumble config
//#define PSX_COMBINE_RUMBLE

// It's possible to remove support of some hardware to free program storage space
// Just comment the desired lines below
//#define GUNCON_SUPPORT
//#define JOGCON_SUPPORT
//#define NEGCON_SUPPORT

// Mouse output is used on guncon and jogcon modes.
// Can be disabled if only using on MiSTer
//#define ENABLE_PSX_GUNCON_MOUSE
//#define ENABLE_PSX_JOGCON_MOUSE


// Oled display can be used for detailed info
//#define ENABLE_PSX_GENERAL_OLED
//#define ENABLE_PSX_GUNCON_OLED
//#define ENABLE_PSX_JOGCON_OLED

//PCEngine config
//#define PCE_ENABLE_MULTITAP

//NeoGeo config (REQUIRED!)
#define NEOGEO_DEBOUNCE 2 //debounce time in milliseconds

//Jaguar config
#define JAG_DEBOUNCE 8 //debounce time in milliseconds. Optional

//N64 config
//Analog stick range
//Comment out (undefine) to use raw values.
//Set as zero to use automatic map. Will calibrate and scale to full range.
//Set a positive value to use it as the maximum value. It will be scaled to full range.
#define N64_ANALOG_MAX 65

//GameCube config
//Analog sticks range
//Comment out (undefine) to use raw values.
//Set a positive value to use it as the maximum value. It will be scaled to full range.
#define GC_ANALOG_MAX 225

//Wii config
//#define WII_ANALOG_RAW //report analog as raw values? leave undefined to use full range analog.
//#define ENABLE_WII_GUITAR //Enable support for Guitar Hero device


#ifndef REFLEX_NO_DEFAULTS
//  #define ENABLE_REFLEX_SATURN
//  #define ENABLE_REFLEX_SNES
//  #define ENABLE_REFLEX_PSX
//  #define ENABLE_REFLEX_PSX_JOG //this is for jogcon forced specific mode. jogcon can still be used with ENABLE_REFLEX_PSX
//  #define ENABLE_REFLEX_PCE
//  #define ENABLE_REFLEX_NEOGEO
//  #define ENABLE_REFLEX_3DO
//  #define ENABLE_REFLEX_JAGUAR
//  #define ENABLE_REFLEX_N64
//  #define ENABLE_REFLEX_GAMECUBE
//  #define ENABLE_REFLEX_WII
//  #define ENABLE_REFLEX_SMS
//  #define ENABLE_REFLEX_JPC
#endif // REFLEX_NO_DEFAULTS


//eerpom index for storage values
#define REFLEX_EEPROM_MODULE    0
#define REFLEX_EEPROM_INPUTMODE 1

#define pinBtn 12 //mode button


#if REFLEX_VERSION == 0 || REFLEX_VERSION == -1
  #define REFLEX_PIN_VERSION 1
#else
  #define REFLEX_PIN_VERSION 2
#endif

#if REFLEX_VERSION == -2
  #define REFLEX_USE_OLED_DISPLAY
#elif REFLEX_VERSION == -1 || REFLEX_VERSION == 0
  #define REFLEX_USE_SINGLE_OLED
#endif

#include "src/DigitalIO/DigitalIO.h"
#include <avr/wdt.h>
#include <EEPROM.h>
#include "Shared.h"

#include <LUFA.h>
#include "LUFADriver.h"

// Define time function for gamepad debouncer
#include "src/MPG/GamepadDebouncer.h"
uint32_t getMillis() { return millis(); }

#include "RZInputModule.h"

#ifdef ENABLE_REFLEX_SATURN
  #include "Input_Saturn.h"
#endif
#ifdef ENABLE_REFLEX_SNES
  #include "Input_Snes.h"
#endif
#ifdef ENABLE_REFLEX_PSX
  #include "Input_Psx.h"
#endif
#ifdef ENABLE_REFLEX_PCE
  #include "Input_Pce.h"
#endif
#ifdef ENABLE_REFLEX_NEOGEO
  #include "Input_NeoGeo.h"
#endif
#ifdef ENABLE_REFLEX_3DO
  #include "Input_3do.h"
#endif
#ifdef ENABLE_REFLEX_JAGUAR
  #include "Input_Jaguar.h"
#endif
#ifdef ENABLE_REFLEX_N64
  #include "Input_N64.h"
#endif
#ifdef ENABLE_REFLEX_GAMECUBE
  #include "Input_GameCube.h"
#endif
#ifdef ENABLE_REFLEX_WII
  #include "Input_Wii.h"
#endif
#if defined(ENABLE_REFLEX_SMS) || defined(ENABLE_REFLEX_JPC)
  #include "Input_Sms.h"
#endif

// validation
static_assert( (RZORD_LAST > 1), "Error: must enable at least one input module");

RZInputModule* gamepadModule; // gamepad(DEBOUNCE_MILLIS); // The gamepad instance

char USB_STRING_MANUFACTURER[] = "MiSTerAddons";
char USB_STRING_PRODUCT[] = "Reflex Adapt";
//char USB_STRING_VERSION[] = "1.0";
//char USB_STRING_PS3_VERSION[] = "DUALSHOCK3";
char USB_SWITCH_STRING_MANUFACTURER[] = "HORI CO.,LTD.";
char USB_SWITCH_STRING_PRODUCT[] = "POKKEN CONTROLLER";

bool ps3enabled = false;

void printModeByIndex(const InputMode i) {
  if(i == INPUT_MODE_XINPUT) {
    display.print(F("XINPUT"));
  } else if (i == INPUT_MODE_SWITCH) {
    display.print(F("SWITCH"));
//  } else if (i == INPUT_MODE_PS3) {
//    display.print(F("PS3"));
  } else if (i >= INPUT_MODE_HID && i < INPUT_MODE_LAST) {
    display.print(F("HID"));
  } else {
      display.print(F("ERROR"));
  }
}

void configureOutput(InputMode currentMode) {
  uint8_t btnState = fastDigitalRead(pinBtn);

  if (btnState == LOW) { // Enter config mode
    bool firstRun = true;
    uint32_t btnPressTime = 0;
    uint8_t btnStateLast = HIGH;

    display.clear();
    display.println("SETUP");
    delay(1500);

    display.clear();
    display.println(F("OUTPUT MODE\n"));
    display.println(F("PRESS: CHANGE"));
    display.println(F("HOLD:  SAVE\n"));
    delay(500);

    while(1) {
      btnState = fastDigitalRead(pinBtn);

      bool isNewPress = (btnState == LOW) && (btnStateLast == HIGH);
      bool isNewRelease = (btnState == HIGH) && (btnStateLast == LOW);

      if(isNewPress) {
        btnPressTime = millis();
        delay(16);
      }

      if(isNewRelease) {
        delay(16);
      }
      
      btnStateLast = btnState;

      if(btnState == LOW && millis() - btnPressTime > 1000) { //save!
        display.clear();
        display.println(F("SAVED!\n"));
        display.print(F("MODE: "));
        printModeByIndex(currentMode);
        display.println();
        display.println();
        display.print(F("DEVICE WILL RESTART"));
        delay(2000);
        //while (1) {}
        
        //save mode
        EEPROM.write(REFLEX_EEPROM_INPUTMODE, (byte)currentMode);
        
        //initialize watchdog
        wdt_enable(WDTO_1S);
        while (1) {}
      }

      if(firstRun || isNewRelease) {
        if(!firstRun) {
          currentMode = (InputMode)(currentMode + 1);

          if(currentMode > INPUT_MODE_HID) //LAST
            currentMode = (InputMode)0;
        }
        display.clear(0, 127, 5, 7);

        printModeByIndex(currentMode);

        firstRun = false;
      }
    }
  }
}

#ifdef REFLEX_USE_OLED_DISPLAY
  void setPixelMode() {
    clearOledDisplay();
    switch(deviceMode) {
#ifdef ENABLE_REFLEX_SATURN
      case RZORD_SATURN:
        display.setCol(2*6);
        display.print(F("GENESIS + SATURN"));
        break;
#endif
#ifdef ENABLE_REFLEX_SNES
      case RZORD_SNES:
        #ifdef SNES_ENABLE_VBOY
          display.setCol(2*6);
          display.print(F("NES + SNES + VBOY"));
        #else
          display.setCol(6*6);
          display.print(F("NES + SNES"));
        #endif
        break;
#endif
#ifdef ENABLE_REFLEX_PSX
      case RZORD_PSX:
#ifdef ENABLE_REFLEX_PSX_JOG
      case RZORD_PSX_JOG:
#endif
      {
#ifdef ENABLE_REFLEX_PSX_JOG //dedicated jogcon mode
        if(deviceMode == RZORD_PSX_JOG) {
          display.setCol(6*6);
          display.println("PSX JOGCON");
        } else
#else //general psx mode
        {
          uint8_t psxchars = 9;
          #ifdef GUNCON_SUPPORT
            psxchars -= 2;
          #endif
          #ifdef JOGCON_SUPPORT
            psxchars -= 2;
          #endif
          #ifdef NEGCON_SUPPORT
            psxchars -= 2;
          #endif

          display.setCol(psxchars*6);
          display.print(F("PSX"));

          #ifdef GUNCON_SUPPORT
            display.print(F("+GUN"));
          #endif
          #ifdef JOGCON_SUPPORT
            display.print(F("+JOG"));
          #endif
          #ifdef NEGCON_SUPPORT
            display.print(F("+NEG"));
          #endif
        }
#endif //end general psx mode

        break;
      }
#endif
#ifdef ENABLE_REFLEX_PCE
      case RZORD_PCE:
        display.setCol(6*6);
        display.print(F("PC-ENGINE"));
        break;
#endif
#ifdef ENABLE_REFLEX_NEOGEO
      case RZORD_NEOGEO:
        display.setCol(7*6);
        display.print(F("NEO-GEO"));
        break;
#endif
#ifdef ENABLE_REFLEX_3DO
      case RZORD_3DO:
        display.setCol(9*6);
        display.print(F("3DO"));
        break;
#endif
#ifdef ENABLE_REFLEX_JAGUAR
      case RZORD_JAGUAR:
        display.setCol(7*6);
        display.print(F("JAGUAR"));
        break;
#endif
#ifdef ENABLE_REFLEX_N64
      case RZORD_N64:
        #ifdef N64_ANALOG_MAX
          display.setCol(1*6);
          display.print(F("N64 EXTENDED RANGE"));
        #else
          display.setCol(1*6);
          display.print(F("N64 ORIGINAL RANGE"));
        #endif
        break;
#endif
#ifdef ENABLE_REFLEX_GAMECUBE
      case RZORD_GAMECUBE:
        display.setCol(6*6);
        display.print(F("GAMECUBE"));
        break;
#endif
#ifdef ENABLE_REFLEX_WII
      case RZORD_WII:
        display.setCol(9*6);
        display.print(F("WII"));
        break;
#endif
#ifdef ENABLE_REFLEX_SMS
      case RZORD_SMS:
        display.setCol(1*6);
        display.print(F("SMS+ATARI+C64+AMIGA"));
        break;
#endif
#ifdef ENABLE_REFLEX_JPC
      case RZORD_JPC:
        display.setCol(2*6);
        display.print(F("FMT+MSX+X68K+PC-X8"));
        break;
#endif

      default:
        break;
    }
  }
#endif

void createModuleInstance() {
  switch(deviceMode) {
    #ifdef ENABLE_REFLEX_SATURN
      case RZORD_SATURN:
        gamepadModule = new ReflexInputSaturn();
        break;
    #endif
    #ifdef ENABLE_REFLEX_SNES
      case RZORD_SNES:
        gamepadModule = new ReflexInputSnes();
        break;
    #endif
    #ifdef ENABLE_REFLEX_PSX
      case RZORD_PSX:
        gamepadModule = new ReflexInputPsx();
        break;
    #endif
    #ifdef ENABLE_REFLEX_PCE
      case RZORD_PCE:
        gamepadModule = new ReflexInputPce();
        break;
    #endif
    #ifdef ENABLE_REFLEX_NEOGEO
      case RZORD_NEOGEO:
        gamepadModule = new ReflexInputNeoGeo();
        break;
    #endif
    #ifdef ENABLE_REFLEX_3DO
      case RZORD_3DO:
        gamepadModule = new ReflexInput3do();
        break;
    #endif
    #ifdef ENABLE_REFLEX_JAGUAR
      case RZORD_JAGUAR:
        gamepadModule = new ReflexInputJaguar();
        break;
    #endif
    #ifdef ENABLE_REFLEX_N64
      case RZORD_N64:
        gamepadModule = new ReflexInputN64();
        break;
    #endif
    #ifdef ENABLE_REFLEX_GAMECUBE
      case RZORD_GAMECUBE:
        gamepadModule = new ReflexInputGameCube();
        break;
    #endif
    #ifdef ENABLE_REFLEX_WII
      case RZORD_WII:
        gamepadModule = new ReflexInputWii();
        break;
    #endif
    #ifdef ENABLE_REFLEX_SMS
      case RZORD_SMS:
        gamepadModule = new ReflexInputSms();
        break;
    #endif
    #ifdef ENABLE_REFLEX_JPC
      case RZORD_JPC:
        gamepadModule = new ReflexInputSms();
        break;
    #endif
  }
}

void setNextMode() {
  deviceMode = (DeviceEnum)(deviceMode + 1);
  if(deviceMode >= RZORD_LAST)
    deviceMode = (DeviceEnum)1;
}

void setup() {
  //configure the mode button  
  fastPinMode(pinBtn, INPUT_PULLUP);
  ps3enabled = false;

  //load saved module
  deviceMode = (DeviceEnum)EEPROM.read(REFLEX_EEPROM_MODULE);
  if (deviceMode >= RZORD_LAST)
    deviceMode = (DeviceEnum)1;

  createModuleInstance();

  #if REFLEX_VERSION == -2
    display.begin(&Adafruit128x64, I2C_ADDRESS);
    display.setContrast(1);
    //display.setFont(System5x7);
    display.setFont(ReflexPad5x7);
    display.clear();
    display.set1X();
    display.setCol(5*6);
    display.set2X();
    display.print(F("REFLEX"));
    display.set1X();
  #endif

  #ifdef ENABLE_REFLEX_PAD
    display.setRow(oledDisplayFirstRow);
    //gamepadModule->displayInputName();
    setPixelMode();
  #endif

  //display.setRow(2);
  //display.setCol(2*6);


//  InputMode inputMode = gamepadModule->options.inputMode;
  InputMode loadedInputMode = (InputMode)EEPROM.read(REFLEX_EEPROM_INPUTMODE);
  if (loadedInputMode >= INPUT_MODE_LAST)
    loadedInputMode = (InputMode)0;
  
  gamepadModule->options.inputMode = loadedInputMode;

  gamepadModule->setup(); // Runs your custom setup logic
  //gamepadModule->load();  // Get saved options if enabled
  delay(20);
  gamepadModule->read();  // Perform an initial button read so we can set input mode
  delay(20);
  gamepadModule->read();



  if (gamepadModule->pressedB1(0)) {
    gamepadModule->options.inputMode = INPUT_MODE_SWITCH;
  } else if (gamepadModule->pressedB2(0)) {
    gamepadModule->options.inputMode = INPUT_MODE_XINPUT;
  }
//  } else if (gamepadModule->pressedB2(0)) {
//    if(static_cast<uint8_t>(moduleInputMode) < static_cast<uint8_t>(INPUT_MODE_HID))
//      gamepadModule->options.inputMode = INPUT_MODE_HID;
//  } else if (gamepadModule->pressedB3(0)) {
//    gamepadModule->options.inputMode = INPUT_MODE_PS3;
//  }

// forcing output mode
//gamepadModule->options.inputMode = INPUT_MODE_XINPUT;
//gamepadModule->options.inputMode = INPUT_MODE_HID;
//gamepadModule->options.inputMode = INPUT_MODE_SWITCH;

  configureOutput(gamepadModule->options.inputMode);

  gamepadModule->setup2(); //Second step of setup. Module can change the hid mode

  const InputMode moduleInputMode = gamepadModule->options.inputMode;
//    if (loadedInputMode != moduleInputMode) { //module changed the mode
//      
//    }
    


  // Use the inlined `pressed` convenience methods
//  if (gamepadModule->pressedB1(0)) {
//    inputMode = INPUT_MODE_SWITCH;
//    display.print('S');
//  } else if (gamepadModule->pressedB2(0)) {
//    inputMode = INPUT_MODE_XINPUT;
//    display.print('X');
//  } else {
//    inputMode = INPUT_MODE_HID;
//    display.print('D');
//  }
//  InputMode inputMode = gamepadModule->options.inputMode;
//  if (gamepadModule->pressedB1(0)) {
//    inputMode = INPUT_MODE_XINPUT;
//    display.print('X');
//  } else {
//    inputMode = INPUT_MODE_SWITCH;
//    display.print('S');
//  }

//  if (gamepadModule->pressedB1(0)) {
//    inputMode = INPUT_MODE_SWITCH;
//    display.print('S');
//  } else if (gamepadModule->pressedB2(0)) {
//    inputMode = INPUT_MODE_XINPUT;
//    display.print('X');
//  } else if (gamepadModule->pressedB3(0)) {
//    inputMode = INPUT_MODE_HID_NEGCON;
//    display.print('M');
//  } else {
//    //inputMode = INPUT_MODE_HID;
//    inputMode = INPUT_MODE_XINPUT;
//    display.print('X');
//  }
  //inputMode = INPUT_MODE_XINPUT;

  display.setRow(1);
  display.setCol(0);

  if (gamepadModule->options.inputMode == INPUT_MODE_XINPUT) {
    display.print('X');
  } else if (gamepadModule->options.inputMode == INPUT_MODE_SWITCH) {
    display.print('S');
//  } else if (gamepadModule->options.inputMode == INPUT_MODE_PS3) {
//    display.print('P');
  } else if (gamepadModule->options.inputMode == INPUT_MODE_HID) {
    display.print('H');
  } else if (gamepadModule->options.inputMode == INPUT_MODE_HID_GUNCON
    || gamepadModule->options.inputMode == INPUT_MODE_HID_NEGCON
    || gamepadModule->options.inputMode == INPUT_MODE_HID_JOGCON
    || gamepadModule->options.inputMode == INPUT_MODE_HID_JOGCON_MOUSE) {
    display.print('M');
  } else {
    display.print('E');
  }

  gamepadModule->f1Mask = (GAMEPAD_MASK_B1 | GAMEPAD_MASK_S2); //(A+START)

//  if (inputMode != gamepadModule->options.inputMode) {
//    gamepadModule->options.inputMode = inputMode;
//    //gamepadModule->save();
//  }

  // Initialize USB device driver
  setupHardware(gamepadModule->options.inputMode, gamepadModule->totalUsb, gamepadModule->getUsbVersion(), gamepadModule->getUsbId());
}

void loop() {
  //static const uint8_t reportSize = gamepadModule->getReportSize();  // Get report size from Gamepad instance
  //static GamepadHotkey hotkey;                            // The last hotkey pressed
  static uint32_t lastRead = 0;

  bool stateChanged = false;

  if (micros() - lastRead >= gamepadModule->sleepTime) {
    stateChanged = gamepadModule->read();                              // Read raw inputs
    lastRead = micros();
  }
  
  //gamepadModule->debounce();                                         // Run debouncing if enabled
  //hotkey = gamepadModule->hotkey(i);                                  // Check hotkey presses (D-pad mode, SOCD mode, etc.), hotkey enum returned
 
  for (uint8_t i = 0; i < gamepadModule->totalUsb; ++i) {
    gamepadModule->process(i);                                          // Perform final input processing (SOCD cleaning, LS/RS emulation, etc.)
    sendReport(gamepadModule->getReport(i), gamepadModule->getReportSize(i), &gamepadModule->rumble[i], i);             // Convert and send it!
    
    if (gamepadModule->options.inputMode == INPUT_MODE_XINPUT)         // Limit xinput to a single device
      break;
  }

//  display.clear();
//  display.println( gamepadModule->pressedL2(0) );
//  display.println( gamepadModule->state[0].lt );
//  display.println( );
//  display.println( gamepadModule->pressedR2(0) );
//  display.println( gamepadModule->state[0].rt );
//  delay(2);

  if (ps3enabled) {
    ps3enabled = false;
    gamepadModule->isPS3 = true;
    display.setRow(1);
    display.setCol(0);
    display.print('P');
  }

  //Disable oled display after 2 minutes of no state changed
  //Any input state change will wake up the display
  #ifdef REFLEX_USE_OLED_DISPLAY
    const uint32_t millisNow = millis();
    if (stateChanged) {
      if (!oledOn)
        setOledDisplay(true);
      else
        oledDisplayTimer = millisNow;
    }
    if (oledOn && millisNow - oledDisplayTimer >= 120000) { //two minutes
      setOledDisplay(false);
    }
  #endif
  
  if (fastDigitalRead(pinBtn) == LOW) {
    delay(300); //todo debounce?

    #ifdef REFLEX_USE_OLED_DISPLAY
      if(!oledOn) {
        setOledDisplay(true);
        return;
      }
    #endif

    DeviceEnum previousDeviceMode = deviceMode;
    setNextMode();
    #ifdef REFLEX_USE_OLED_DISPLAY
      setPixelMode();
    #endif

    for (uint16_t timeOut = 0; timeOut < 2000; timeOut++) { //timeout to save the next mode and reset the device
      delay(2);

      //check for button press
      if(fastDigitalRead(pinBtn) == LOW) {
        delay(300); //todo debounce?
        setNextMode();
        #ifdef REFLEX_USE_OLED_DISPLAY
          setPixelMode();
        #endif
        timeOut = 0;
      }
    }

    if (previousDeviceMode == deviceMode) { //mode was not changed
      #ifdef REFLEX_USE_OLED_DISPLAY
        setPixelMode(); //todo reset?
      #endif
      return;
    }

    //save mode
    EEPROM.write(REFLEX_EEPROM_MODULE, (byte)deviceMode);
    //EEPROM.update(REFLEX_EEPROM_MODULE, (byte)deviceMode);

    #ifdef REFLEX_USE_OLED_DISPLAY
      display.clear();
      display.print(F("REBOOTING..."));
    #endif

    //initialize watchdog
    wdt_enable(WDTO_1S);
    while(1) { }

  }//end if(fastDigitalRead(pinBtn) == LOW) 

  //delayMicroseconds(100);
}
