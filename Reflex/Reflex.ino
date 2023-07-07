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

#define REFLEX_VERSION -2 // V2 - mode button. oled display.
//#define REFLEX_VERSION -1 // V1 - mode button. single rgb led.
//#define REFLEX_VERSION 0  // V1 - rotary encoder. single rgb led.
//#define REFLEX_VERSION 1  // V2 - mode button. non-functional rgb leds. uses TX/RX leds
//#define REFLEX_VERSION 2  // V2 - fixed rgb leds

#define ENABLE_REFLEX_LOGO //reflex logo on oled display
#define ENABLE_REFLEX_PAD //pad on oled display

//#define ENABLE_REFLEX_SATURN
//#define ENABLE_REFLEX_SNES
//#define ENABLE_REFLEX_PSX
//#define ENABLE_REFLEX_PSX_JOG //this is for jogcon forced specific mode. jogcon can still be used with ENABLE_REFLEX_PSX
//define ENABLE_REFLEX_PCE
//#define ENABLE_REFLEX_NEOGEO
//#define ENABLE_REFLEX_3DO
//#define ENABLE_REFLEX_JAGUAR
//#define ENABLE_REFLEX_N64
//#define ENABLE_REFLEX_GAMECUBE
//#define ENABLE_REFLEX_WII

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
#define NEOGEO_DEBOUNCE 2 //debounce time in milliseconds

//Jaguar config
//#define JAG_DEBOUNCE 8 //debounce time in milliseconds. Optional

//N64 config
//Analog range
//Comment out (undefine) to use raw values.
//Set as zero to use automatic map. Will calibrate and scale to full range.
//Set a positive value to use it as the maximum value. It will be scaled to full range.
#define N64_ANALOG_MAX 65

//GameCube config
//Analog range
//Comment out (undefine) to use raw values.
//Set a positive value to use it as the maximum value. It will be scaled to full range.
#define GC_ANALOG_MAX 225


//Wii config
//#define WII_ANALOG_MAX 0 //still need to implement...
//#define ENABLE_WII_GUITAR //Enable support for Guitar Hero device


#ifndef REFLEX_NO_DEFAULTS
#define ENABLE_REFLEX_GAMECUBE
#endif // REFLEX_NO_DEFAULTS

/******************************************************************************/

#ifndef REFLEX_VERSION
  #error REFLEX_VERSION must be defined
#endif

#if defined(ENABLE_REFLEX_PSX_JOG) && !defined(ENABLE_REFLEX_PSX)
  #error ENABLE_REFLEX_PSX must be enabled if using ENABLE_REFLEX_PSX_JOG
#endif

#if defined(ENABLE_REFLEX_PSX_JOG) && !defined(JOGCON_SUPPORT)
  #error JOGCON_SUPPORT must be enabled if using ENABLE_REFLEX_PSX_JOG
#endif

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

#include "Shared.h"


#ifdef ENABLE_REFLEX_SATURN_SIMPLE
  #define ENABLE_REFLEX_SATURN
//#define SATLIB_ENABLE_MEGATAP //suport for 4p megatap
//#define SATLIB_ENABLE_SATTAP //support for 6p multitap
  #include "Input_Saturn.h"
#endif
#ifdef ENABLE_REFLEX_SATURN
  #define SATLIB_ENABLE_MEGATAP //suport for 4p megatap
  #define SATLIB_ENABLE_SATTAP //support for 6p multitap
  #include "Input_Saturn.h"
#endif
#ifdef ENABLE_REFLEX_SNES_SIMPLE
  #define ENABLE_REFLEX_SNES
  //#define SNES_ENABLE_VBOY
  //#define SNES_ENABLE_MULTITAP
  #include "Input_Snes.h"
#endif
#ifdef ENABLE_REFLEX_SNES
  #define SNES_ENABLE_VBOY
  #define SNES_ENABLE_MULTITAP
  #include "Input_Snes.h"
#endif
#ifdef ENABLE_REFLEX_PSX_PADS
#define ENABLE_REFLEX_PSX
// PS1 Guncon config
// 0=Mouse, 1=Joy, 2=Joy OffScreenEdge (MiSTer), 3=Guncon Raw (MiSTer)
//#define GUNCON_FORCE_MODE 3
// PS1 NeGcon config
// 0=Default, 1=MiSTer Wheel format with paddle
//#define NEGCON_FORCE_MODE 1
//#define GUNCON_SUPPORT
//#define JOGCON_SUPPORT
//#define NEGCON_SUPPORT
//#define ENABLE_PSX_GUNCON_MOUSE
//#define ENABLE_PSX_JOGCON_MOUSE
  #define ENABLE_PSX_GENERAL_OLED
//#define ENABLE_PSX_GUNCON_OLED
//#define ENABLE_PSX_JOGCON_OLED
  #include "Input_Psx.h"
#endif
#ifdef ENABLE_REFLEX_PSX_MISTER
#define ENABLE_REFLEX_PSX
// PS1 Guncon config
// 0=Mouse, 1=Joy, 2=Joy OffScreenEdge (MiSTer), 3=Guncon Raw (MiSTer)
  #define GUNCON_FORCE_MODE 3
// PS1 NeGcon config
// 0=Default, 1=MiSTer Wheel format with paddle
  #define NEGCON_FORCE_MODE 1
  #define GUNCON_SUPPORT
  #define JOGCON_SUPPORT
  #define NEGCON_SUPPORT
  #define ENABLE_PSX_GUNCON_MOUSE
  #define ENABLE_PSX_JOGCON_MOUSE
  #define ENABLE_PSX_GENERAL_OLED
  #define ENABLE_PSX_GUNCON_OLED
  #define ENABLE_PSX_JOGCON_OLED
  #include "Input_Psx.h"
#endif
#ifdef ENABLE_REFLEX_PSX_PC
#define ENABLE_REFLEX_PSX
// PS1 Guncon config
// 0=Mouse, 1=Joy, 2=Joy OffScreenEdge (MiSTer), 3=Guncon Raw (MiSTer)
  #define GUNCON_FORCE_MODE 0
// PS1 NeGcon config
// 0=Default, 1=MiSTer Wheel format with paddle
  #define NEGCON_FORCE_MODE 0
  #define GUNCON_SUPPORT
  #define JOGCON_SUPPORT
  #define NEGCON_SUPPORT
  #define ENABLE_PSX_GUNCON_MOUSE
  #define ENABLE_PSX_JOGCON_MOUSE
  #define ENABLE_PSX_GENERAL_OLED
  #define ENABLE_PSX_GUNCON_OLED
  #define ENABLE_PSX_JOGCON_OLED
  #include "Input_Psx.h"
#endif
#ifdef ENABLE_REFLEX_PCE_SIMPLE
  #define ENABLE_REFLEX_PCE
  //#define PCE_ENABLE_MULTITAP
  #include "Input_Pce.h"
#endif
#ifdef ENABLE_REFLEX_PCE
  #define PCE_ENABLE_MULTITAP
  #include "Input_Pce.h"
#endif
#ifdef ENABLE_REFLEX_NEOGEO_NO_DEBOUNCE
  #define ENABLE_REFLEX_NEOGEO
  //#define NEOGEO_DEBOUNCE 2 //debounce time in milliseconds
  #include "Input_NeoGeo.h"
#endif
#ifdef ENABLE_REFLEX_NEOGEO_0MS_DEBOUNCE
  #define ENABLE_REFLEX_NEOGEO
  #define NEOGEO_DEBOUNCE 0 //debounce time in milliseconds
  #include "Input_NeoGeo.h"
#endif
#ifdef ENABLE_REFLEX_NEOGEO_DEBOUNCE
  #define ENABLE_REFLEX_NEOGEO
  #define NEOGEO_DEBOUNCE 2 //debounce time in milliseconds
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

#include "src/DigitalIO/DigitalIO.h"
#include <avr/wdt.h>

#if REFLEX_VERSION != 0 //for version zero I'm using the rotary encoder. no saving/loading from eeprom
  #include <EEPROM.h>
#endif

//Oled Display
#if REFLEX_VERSION == -2
  //included in shared.h
  //SSD1306AsciiAvrI2c display;
#else
  #include "src/Adafruit_NeoPixel/Adafruit_NeoPixel.h"
#endif


//eerpom index for storage values
#define RZORD_EEPROM_MODE 0


#define pinBtn 12
//int mode = 0; // 0 = Off, 1 = Sega, 2 = Nintendo, 3 = Sony
//int deviceInit;

// NeoPixel
#if REFLEX_VERSION != -2
  #define LED_PIN        11 // On Trinket or Gemma, suggest changing this to 1
  #define NUMPIXELS 2 // Popular NeoPixel ring size
  Adafruit_NeoPixel pixels(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);
  #define DELAYVAL 500 // Time (in milliseconds) to pause between pixels
#endif

/*#if defined(REFLEX_USE_OLED_DISPLAY) && defined(ENABLE_REFLEX_LOGO)
  const char reflexLogo[] PROGMEM = "  _   _  _     _\n |_) |_ |_ |  |_ \\/\n | \\ |_ |  |_ |_ /\\";
#endif*/

//uint8_t colors[] = {0,0,0};
uint32_t colors = 0;
#if REFLEX_VERSION != -2
  void setLedColors(const uint8_t r, const uint8_t g, const uint8_t b) {
    colors = pixels.Color(r, g, b);
    //colors[0] = r;
    //colors[1] = g;
    //colors[2] = b;
  }
#else
  #define setLedColors(...)
#endif

#if REFLEX_VERSION == -2
  void setPixelMode() {
    //display.clear();
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
        display.setCol(2*6);
        display.print(F("NES + SNES + VBOY"));
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

      default:
        break;
    }
  }
#else
  void setPixelMode() {
    uint8_t controllerCount = 0;
    const uint8_t pw = 20; //led power (brightness)
    // NeoPixels
    pixels.clear(); // Set all pixel colors to 'off'
    
    //Serial.println(deviceMode);
    switch(deviceMode) {
      case RZORD_SATURN:
        //Serial.println("LED BLUE");
        controllerCount = 2;
        setLedColors(0, 0, pw);
        break;
      case RZORD_SNES:
        //Serial.println("LED RED");
        controllerCount = 2;
        setLedColors(pw, 0, 0);
        break;
      case RZORD_PSX:
        //Serial.println("LED GREEN");
        controllerCount = 2;
        setLedColors(0, pw, 0);
        break;
      case RZORD_PSX_JOG:
        //Serial.println("LED LIGHT GREEN");
        controllerCount = 2;
        setLedColors(0, pw/2, 0);
        break;
      case RZORD_PCE:
        //Serial.println("LED YELLOW");
        controllerCount = 2;
        setLedColors(pw, pw, 0);
        break;
      case RZORD_NEOGEO:
        //Serial.println("LED AQUA");
        controllerCount = 1;
        setLedColors(0, pw, pw);
        break;
      case RZORD_3DO:
        controllerCount = 2;
        setLedColors(0, pw, pw);
        break;
      case RZORD_JAGUAR:
        controllerCount = 1;
        setLedColors(0, pw, pw);
        break;
      case RZORD_N64:
        controllerCount = 1;
        setLedColors(0, pw, pw);
        break;
      default:
        break;
    }
    
    for(uint8_t i=0; i<controllerCount; i++) {
      pixels.setPixelColor(i, colors);
      //delay(DELAYVAL);
    }
    pixels.show();
  }
#endif

void setNextMode() {
    deviceMode = (DeviceEnum)(deviceMode + 1);
    if(deviceMode >= RZORD_LAST)
      deviceMode = (DeviceEnum)1;
  /*switch(deviceMode) {
    case RZORD_SATURN:
      //Serial.println("CURRENT MODE SATURN");
      //Serial.println("CHANGE TO SNES");
      deviceMode = RZORD_SNES;
      break;
    case RZORD_SNES:
      //Serial.println("CURRENT MODE SNES");
      //Serial.println("CHANGE TO PSX");
      deviceMode = RZORD_PSX;
      break;
    case RZORD_PSX:
      //Serial.println("CURRENT MODE PSX");
      //Serial.println("CHANGE TO PSX_JOG");
      deviceMode = RZORD_PSX_JOG;
      break;
    case RZORD_PSX_JOG:
      //Serial.println("CURRENT MODE PSX_JOG");
      //Serial.println("CHANGE TO PCE");
      deviceMode = RZORD_PCE;
      break;
    case RZORD_PCE:
      //Serial.println("CURRENT MODE PCE");
      //Serial.println("CHANGE TO NEOGEO");
      deviceMode = RZORD_NEOGEO;
      break;
    default://loop to first value
      deviceMode = RZORD_SATURN;
      //Serial.println("CURRENT MODE NEOGEO");
      //Serial.println("CHANGE TO SATURN");
      break;
  }*/
}

#if REFLEX_VERSION != -2
  static bool LEDMODE = true;
#endif

#if REFLEX_VERSION == 1
  void showModeUsingTXLed() {
    const uint32_t maxNoBlinkCounter = 1300000;
    const uint32_t maxTimeCounter =     200000;
    static uint8_t timesBlinked = 0;
    static uint32_t timeCounter = micros();
    static uint32_t noBlinkTimer = micros();
    static bool isSleeping = false;
    const uint8_t timesToBlink = deviceMode * 2; // OFF/ON sequences
    if (isSleeping) {
      if (micros() - noBlinkTimer > maxNoBlinkCounter) {
        //reset variables
        isSleeping = false;
        timesBlinked = 0;
        noBlinkTimer = micros();
      }
    } else {
      if (micros() - timeCounter > maxTimeCounter) {
        if(timesBlinked < timesToBlink) {
          LEDMODE = !LEDMODE;
          timesBlinked++;
        } else {
          isSleeping = true;
          LEDMODE = true;
          noBlinkTimer = micros();
        }
        timeCounter = micros();
      }
    }
    if(LEDMODE)
      TXLED0;//ON
    else
      TXLED1;//OFF
  }
#endif


//DeviceEnum deviceMode = RZORD_NONE;

// 315deg = 0.5V = 186
// 0deg   = 1.2V = 254
// 45deg  = 2.1V = 430
// 90deg  = 3.1V = 635
// 135deg = 4.0V = 819
// 180deg = 4.5V = 922
// 225deg = 4.7V = 956
// 270deg = 5.0V = 1024
/*
90
250
409
611
783
891
930
1023
1023
*/

void setup() {
  //#if REFLEX_VERSION == 0
    //
  //#elif REFLEX_VERSION == 1
    //
  //#else
    //pinMode(LED_BUILTIN, OUTPUT);
  //#endif


  //Initialize oled display

  
  fastPinMode(pinBtn, INPUT_PULLUP);
  
  //Serial.begin(115200);
  //while (!Serial){};
  //Serial.println("Hello");
  //Serial.println(mode);
  
  //deviceInit = 0;

  //initialize oled display
  #if REFLEX_VERSION != -2
    // NeoPixel
    pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  #endif
    
  #if REFLEX_VERSION == 0
    //use rotary encoder
    const int sensorValue = analogRead(A3);
    //Serial.println(sensorValue);
    if (sensorValue < 150) {
      deviceMode = RZORD_SATURN; //90
      saturnSetup();
    } else if (sensorValue < 320) {
      deviceMode = RZORD_SNES; //250
      snesSetup();
    } else if (sensorValue < 500) {
      deviceMode = RZORD_PSX; //409
      psxSetup();
    } else if (sensorValue < 680) {
      deviceMode = RZORD_PCE; //611
      pceSetup();
    } else {
      deviceMode = RZORD_NEOGEO; //783
      neogeoSetup();
    }

    //leds go from 0 to 80, then to 0. feedback during boot
    pixels.clear();
    uint8_t loopCounter = 0;
    uint8_t i = 1;
    while (true) {
      if (i == 80)
        loopCounter++;
      if (i == 0)
        break;
      else if (loopCounter == 0)
        i++;
      else
        i--;
      pixels.setPixelColor(0, pixels.Color(i, i, i));
      pixels.show();
      delay(10);
    }
    pixels.clear();
    pixels.show();
    delay(100);

  #else
  
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
      default:
        break;
    }
  #endif

  #if REFLEX_VERSION == -2
    display.begin(&Adafruit128x64, I2C_ADDRESS);
    display.setContrast(1);
    //display.setFont(System5x7);
    display.setFont(ReflexPad5x7);
    display.clear();

    #ifdef ENABLE_REFLEX_LOGO
      //art
      //display.println(F("  _   _  _     _\n |_) |_ |_ |  |_ \\/\n | \\ |_ |  |_ |_ /\\"));

      //simple font 2x
      display.set1X();
      display.setCol(5*6);
      display.set2X();
      //display.println(F("  REFLEX"));
      display.print(F("REFLEX"));
      display.set1X();
    
    #endif
  #endif

  setPixelMode();

  //Serial.println("Device initialized");
}

void loop() {
  static unsigned long last = 0;
  static bool stateChanged = false;
  
  #if REFLEX_VERSION == 1
    static bool firstBoot = true;
    if(firstBoot) {
      firstBoot = false;
      delay(500);
      //TX_RX_LED_INIT;
      for(uint16_t timeOut = 0; timeOut < 3500; timeOut++) { //2500
        showModeUsingTXLed();
        delay(2);
      }
      LEDMODE = true;
      TXLED0;
    }
  #endif

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

      default:
        stateChanged = false;
        break;
    }
    last = micros();
  }

  //Disable oled display after 300 seconds (5 minutes) of no state changed
  if(stateChanged) {
    if (!oledOn)
      setOledDisplay(true);
    else
      oledDisplayTimer = millis();
  }
  if (oledOn && millis() - oledDisplayTimer >= 300000) {
    //oledDisplayTimer = millis();
    //display.ssd1306WriteCmd(SSD1306_DISPLAYOFF);
    //oledOn = false;
    setOledDisplay(false);
  }
  
  if(fastDigitalRead(pinBtn) == LOW) { //todo debounce?
    delay(300);

    if(!oledOn) {
      //display.ssd1306WriteCmd(SSD1306_DISPLAYON);
      //setPixelMode();
      //oledOn = true;
      //delay(300);
      //oledDisplayTimer = millis();
      setOledDisplay(true);
      return;
    }
      
      //Only handle mode button press when no controller is connected
      if(true) { // !haveController
  
      DeviceEnum previousDeviceMode = deviceMode;
      setNextMode();
      setPixelMode();
  
      //uint16_t timeOut = 0;
      for(uint16_t timeOut = 0; timeOut < 2000; timeOut++) { //2500
          //Serial.println(timeOut);
          
          delay(2);
  
          //blink led
          #if REFLEX_VERSION == 1
            showModeUsingTXLed();
          #elif REFLEX_VERSION != -2
            if (timeOut % 100 == 0){
              if(LEDMODE)
                pixels.setPixelColor(0, colors);
              else
                pixels.clear();
              pixels.show();
              LEDMODE = !LEDMODE;
            }
          #endif
          /*if (timeOut % 100 == 0){
            digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
          }*/
  
          //check for button press
          if(fastDigitalRead(pinBtn) == LOW) {//todo debounce?
            #if REFLEX_VERSION == 1
              LEDMODE = true;
            #endif
            delay(300);
            setNextMode();
            setPixelMode();
            timeOut = 0;
          }
      }
  
      if(previousDeviceMode == deviceMode)//mode was not changed
      {
        setPixelMode();
        //todo also set txled?
        return;
      }
  
      //save mode
      #if REFLEX_VERSION != 0
        EEPROM.write(RZORD_EEPROM_MODE, (byte)deviceMode);
        //EEPROM.update(RZORD_EEPROM_MODE, (byte)deviceMode);
      #endif
      
      //Serial.println("Rebooting...");
      #if REFLEX_VERSION == -2
        display.clear();
        display.print(F("REBOOTING..."));
      #endif
  
      //initialize watchdog and blink led faster
      wdt_enable(WDTO_1S);
      while(1) {
        //blink led fast to indicate that mode will change
        //static bool LEDMODE = true;
        #if REFLEX_VERSION == 1
          if(LEDMODE)
            TXLED0;//ON
          else
            TXLED1;//OFF
          LEDMODE = !LEDMODE;
        #elif REFLEX_VERSION != -2
          if(LEDMODE)
            pixels.setPixelColor(0, colors);
          else
            pixels.clear();
          pixels.show();
          LEDMODE = !LEDMODE;
        #endif
        //digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        delay(120);
      }
    }//end if(!haveController)

  }//end if(fastDigitalRead(pinBtn) == LOW) 
  
}
