/*******************************************************************************
 * Reflex adapder
 * by MisterAddons (PorkchopExpress)
 * 
 * Powered by RetroZord
 * by Matheus Fraguas (sonik-br)
 * 
 * https://github.com/sonik-br/RetroZordAdapter
*/

//#pragma GCC push_options
//#pragma GCC optimize ("Os")

/*******************************************************************************
 * Optional settings
 *******************************************************************************/

#define REFLEX_USE_OLED_DISPLAY //enable use of the oled display. at minimum it will show the current mode
#define ENABLE_REFLEX_LOGO //reflex logo on oled display
#define ENABLE_REFLEX_PAD //pad on oled display

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

// Sega MegaDrive/Saturn config
#define SATLIB_ENABLE_8BITDO_HOME_BTN // support for HOME button on 8bidto M30 2.4G.
// #define SATLIB_ENABLE_MEGATAP //suport for 4p megatap
// #define SATLIB_ENABLE_SATTAP //support for 6p multitap

// SNES config
//#define SNES_ENABLE_VBOY
//#define SNES_ENABLE_MULTITAP

// PS1 Guncon config
// 0=Mouse, 1=Joy, 2=Joy OffScreenEdge (MiSTer), 3=Guncon Raw (MiSTer)
//#define GUNCON_FORCE_MODE 3

// PS1 NeGcon config
// 0=Default, 1=MiSTer Wheel format with paddle
//#define NEGCON_FORCE_MODE 1

// It's possible to remove support of some hardware to free program storage space
// Just comment the desired lines below
//#define GUNCON_SUPPORT
//#define JOGCON_SUPPORT
//#define NEGCON_SUPPORT

// Mouse output is used on guncon and jogcon modes.
// Can be disabled if only using on MiSTer
// #define ENABLE_PSX_GUNCON_MOUSE
// #define ENABLE_PSX_JOGCON_MOUSE

// Oled display can be used for detailed info
//#define ENABLE_PSX_GENERAL_OLED
//#define ENABLE_PSX_GUNCON_OLED
//#define ENABLE_PSX_JOGCON_OLED

//PCEngine config
// #define PCE_ENABLE_MULTITAP

//NeoGeo config (REQUIRED!)
//#define NEOGEO_DEBOUNCE 2 //debounce time in milliseconds

//Jaguar config
//#define JAG_DEBOUNCE 8 //debounce time in milliseconds. Optional

//N64 config
//Analog range
//Comment out (undefine) to use raw values.
//Set as zero to use automatic map. Will calibrate and scale to full range.
//Set a positive value to use it as the maximum value. It will be scaled to full range.
//#define N64_ANALOG_MAX 80

//GameCube config
//Analog range
//Comment out (undefine) to use raw values.
//Set a positive value to use it as the maximum value. It will be scaled to full range.
#define GC_ANALOG_MAX 225


//Wii config
//#define WII_ANALOG_MAX 0 //still need to implement...
//#define ENABLE_WII_GUITAR //Enable support for Guitar Hero device


#ifndef REFLEX_NO_DEFAULTS
#define ENABLE_REFLEX_SATURN
#define ENABLE_REFLEX_GAMECUBE
#endif // REFLEX_NO_DEFAULTS

/******************************************************************************/

#if defined(ENABLE_REFLEX_LOGO) && !defined(REFLEX_USE_OLED_DISPLAY)
  #error REFLEX_USE_OLED_DISPLAY must be enabled if using ENABLE_REFLEX_LOGO
#endif

#if defined(ENABLE_REFLEX_PAD) && !defined(REFLEX_USE_OLED_DISPLAY)
  #error REFLEX_USE_OLED_DISPLAY must be enabled if using ENABLE_REFLEX_PAD
#endif

#if defined(ENABLE_REFLEX_PSX_JOG) && !defined(ENABLE_REFLEX_PSX)
  #error ENABLE_REFLEX_PSX must be enabled if using ENABLE_REFLEX_PSX_JOG
#endif

#if defined(ENABLE_REFLEX_PSX_JOG) && !defined(JOGCON_SUPPORT)
  #error JOGCON_SUPPORT must be enabled if using ENABLE_REFLEX_PSX_JOG
#endif

#include "Shared.h"

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
#ifdef ENABLE_REFLEX_SMS
  #include "Input_Sms.h"
#endif

#include "src/DigitalIO/DigitalIO.h"
#include <avr/wdt.h>
#include <EEPROM.h>

//eerpom index for storage values
#define RZORD_EEPROM_MODE 0

#define pinBtn 12 //mode button

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
        #endif
        #ifndef SNES_ENABLE_VBOY
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
        display.setCol(5*6);
        display.print(F("PLAYSTATION"));
#ifdef ENABLE_REFLEX_PSX_JOG
        if(deviceMode == RZORD_PSX_JOG)
          display.print(F("-JOGCON"));
#endif
        break;
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
        display.setCol(9*6);
        display.print(F("N64"));
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
        display.setCol(9*6);
        display.print(F("SMS"));
        break;
#endif

      default:
        break;
    }
  }
#endif

void setNextMode() {
  deviceMode = (DeviceEnum)(deviceMode + 1);
  if(deviceMode >= RZORD_LAST)
    deviceMode = (DeviceEnum)1;
}

void setup() {
  //configure the mode button  
  fastPinMode(pinBtn, INPUT_PULLUP);
    

  //use eeprom
  deviceMode = (DeviceEnum)EEPROM.read(RZORD_EEPROM_MODE);
  if (deviceMode >= RZORD_LAST)
    deviceMode = (DeviceEnum)1;
    
  switch(deviceMode) {
#ifdef ENABLE_REFLEX_SATURN
    case RZORD_SATURN:
      saturnSetup();
      break;
#endif
#ifdef ENABLE_REFLEX_SNES
    case RZORD_SNES:
      snesSetup();
      break;
#endif
#ifdef ENABLE_REFLEX_PSX
    case RZORD_PSX:
      psxSetup();
      break;
#endif
#ifdef ENABLE_REFLEX_PSX_JOG
    case RZORD_PSX_JOG:
      psxSetup();
      break;
#endif
#ifdef ENABLE_REFLEX_PCE
    case RZORD_PCE:
      pceSetup();
      break;
#endif
#ifdef ENABLE_REFLEX_NEOGEO
    case RZORD_NEOGEO:
      neogeoSetup();
      break;
#endif
#ifdef ENABLE_REFLEX_3DO
    case RZORD_3DO:
      threedoSetup();
      break;
#endif
#ifdef ENABLE_REFLEX_JAGUAR
    case RZORD_JAGUAR:
      jaguarSetup();
      break;
#endif
#ifdef ENABLE_REFLEX_N64
    case RZORD_N64:
      n64Setup();
      break;
#endif
#ifdef ENABLE_REFLEX_GAMECUBE
    case RZORD_GAMECUBE:
      gameCubeSetup();
      break;
#endif
#ifdef ENABLE_REFLEX_WII
    case RZORD_WII:
      wiiSetup();
      break;
#endif
#ifdef ENABLE_REFLEX_SMS
    case RZORD_SMS:
      smsSetup();
      break;
#endif

    default:
      break;
  } //end switch deviceMode

  //initialize the oled display
  #ifdef REFLEX_USE_OLED_DISPLAY
    display.begin(&Adafruit128x64, I2C_ADDRESS);
    display.setContrast(1);
    //display.setFont(System5x7);
    display.setFont(ReflexPad5x7);
    display.clear();

    #ifdef ENABLE_REFLEX_LOGO
      display.set1X();
      display.setCol(5*6);
      display.set2X();
      display.print(F("REFLEX"));
      display.set1X();
    #endif
    
    setPixelMode();
  #endif //REFLEX_USE_OLED_DISPLAY

  //Serial.println("Device initialized");
}

void loop() {
  static uint32_t last = 0;
  bool stateChanged = false;

  if (micros() - last >= sleepTime) {
    switch(deviceMode){
#ifdef ENABLE_REFLEX_SATURN
      case RZORD_SATURN:
        stateChanged = saturnLoop();
        break;
#endif
#ifdef ENABLE_REFLEX_SNES
      case RZORD_SNES:
        stateChanged = snesLoop();
        break;
#endif
#ifdef ENABLE_REFLEX_PSX
      case RZORD_PSX:
#ifdef ENABLE_REFLEX_PSX_JOG
      case RZORD_PSX_JOG:
#endif
        stateChanged = psxLoop();
        break;
#endif
#ifdef ENABLE_REFLEX_PCE
      case RZORD_PCE:
        stateChanged = pceLoop();
        break;
#endif
#ifdef ENABLE_REFLEX_NEOGEO
        case RZORD_NEOGEO:
        stateChanged = neogeoLoop();
        break;
#endif
#ifdef ENABLE_REFLEX_3DO
        case RZORD_3DO:
        stateChanged = threedoLoop();
        break;
#endif
#ifdef ENABLE_REFLEX_JAGUAR
        case RZORD_JAGUAR:
        stateChanged = jaguarLoop();
        break;
#endif
#ifdef ENABLE_REFLEX_N64
        case RZORD_N64:
        stateChanged = n64Loop();
        break;
#endif
#ifdef ENABLE_REFLEX_GAMECUBE
        case RZORD_GAMECUBE:
        stateChanged = gameCubeLoop();
        break;
#endif
#ifdef ENABLE_REFLEX_WII
        case RZORD_WII:
        stateChanged = wiiLoop();
        break;
#endif
#ifdef ENABLE_REFLEX_SMS
        case RZORD_SMS:
        stateChanged = smsLoop();
        break;
#endif

      default:
        stateChanged = false;
        break;
    }
    last = micros();
  }

  //Disable oled display after 300 seconds (5 minutes) of no state changed
  //Any input state change will wake up the display
  #ifdef REFLEX_USE_OLED_DISPLAY
    if(stateChanged) {
      if (!oledOn)
        setOledDisplay(true);
      else
        oledDisplayTimer = millis();
    }
    if (oledOn && millis() - oledDisplayTimer >= 300000) {
      setOledDisplay(false);
    }
  #endif
  
  if(fastDigitalRead(pinBtn) == LOW) {
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

    for(uint16_t timeOut = 0; timeOut < 2000; timeOut++) { //timeout to save the next mode and reset the device
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
        setPixelMode();
      #endif
      return;
    }

    //save mode
    EEPROM.write(RZORD_EEPROM_MODE, (byte)deviceMode);
    //EEPROM.update(RZORD_EEPROM_MODE, (byte)deviceMode);

    #ifdef REFLEX_USE_OLED_DISPLAY
      display.clear();
      display.print(F("REBOOTING..."));
    #endif

    //initialize watchdog
    wdt_enable(WDTO_1S);
    while(1) { }

  }//end if(fastDigitalRead(pinBtn) == LOW) 
  
}//end loop
