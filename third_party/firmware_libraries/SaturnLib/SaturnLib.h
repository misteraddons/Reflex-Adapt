/*******************************************************************************
 * Sega Saturn controller input library.
 * https://github.com/sonik-br/SaturnLib
 * 
 * The library depends on greiman's DigitalIO library.
 * https://github.com/greiman/DigitalIO
 * 
 * I recommend the usage of SukkoPera's fork of DigitalIO as it supports a few more platforms.
 * https://github.com/SukkoPera/DigitalIO
 * 
 * 
 * It works with saturn controllers (digital or analog) and multitaps.
 * Also works with MegaDrive controllers and mulltitaps.

 Notes on Mission Stick (3 or 6-axis)
    Stick #1 (right side)
    SAT_ANALOG_X,
    SAT_ANALOG_Y,
    SAT_ANALOG_R,

  Stick #2 (left side)
    SAT_ANALOG_X2,
    SAT_ANALOG_Y2,
    SAT_ANALOG_L,
*/

#ifndef SATURNLIB_H_
#define SATURNLIB_H_

#if defined(ARDUINO_ARCH_RP2040)
#include <hardware/gpio.h>
#include <pico/platform.h>
#endif

//Enable usage of HOME button on 8bidto M30 2.4G.
//It will report as saturn's L button.
//#define SATLIB_ENABLE_8BITDO_HOME_BTN

//Dual Mission Stick (6-axis) support
//#define SATLIB_ENABLE_MISSION6

//Multitap support
// #define SATLIB_ENABLE_MEGATAP //suport for 4p megatap
// #define SATLIB_ENABLE_SATTAP //support for 6p multitap

//Max of 6 controllers per port (with a multitap)
#ifndef SATLIB_MAX_CTRL
  #define SATLIB_MAX_CTRL 6
#endif

#define TAP_MEGA_PORTS 4
#define TAP_SAT_PORTS 6

#define SAT_ID_NONE 0b11111111
#define SAT_ID_DIGITALPAD 0b01001111
#define SAT_ID_3DDIGITAL 0b00000010
#define SAT_ID_3DANALOG 0b00010110
#define SAT_ID_WHEEL 0b00010011
#define SAT_ID_MISSION3 0b00010101
#define SAT_ID_MISSION6 0b00011001
//#define SAT_ID_MEGA 0b00001100
#define SAT_ID_MEGA3 0b11100000
#define SAT_ID_MEGA6 0b11100001
#define SAT_ID_MOUSE 0b11100010

// enum SatLibConfig_Enum {
//   SATLIB_ENABLE_SATURN    = (1 << 0), // saturn devices
//   SATLIB_ENABLE_MEGADRIVE = (1 << 1), // megadrive devices
//   SATLIB_ENABLE_MEGATAP   = (1 << 2), // support for 4p megatap (megadrive)
//   SATLIB_ENABLE_SATTAP    = (1 << 3), // support for 6p multitap (saturn)
// };

typedef struct __attribute((packed, aligned(1))) {
  uint8_t enable_saturn    : 1;
  uint8_t enable_megadrive : 1;
  uint8_t enable_sattap    : 1;
  uint8_t enable_megatap   : 1;
  uint8_t                  : 4;
} SatLibConfig_t;

constexpr SatLibConfig_t SatLibConfig_default         { .enable_saturn = 1, .enable_megadrive = 1, .enable_sattap = 1, .enable_megatap = 1 };
constexpr SatLibConfig_t SatLibConfig_saturn_only     { .enable_saturn=1, .enable_sattap = 1 };
constexpr SatLibConfig_t SatLibConfig_megadrive_only  { .enable_megadrive=1, .enable_megatap = 1 };

enum DB9_TR_Enum {
  DB9_TR_INPUT = 0,
  DB9_TR_OUTPUT
};

enum SatDeviceType_Enum {
  SAT_DEVICE_NONE = 0,
  SAT_DEVICE_NOTSUPPORTED,
  SAT_DEVICE_MEGA3,
  SAT_DEVICE_MEGA6,
  SAT_DEVICE_MOUSE,
  SAT_DEVICE_PAD,
  SAT_DEVICE_3DPAD,
  SAT_DEVICE_WHEEL,
  SAT_DEVICE_MISSION3,
#ifdef SATLIB_ENABLE_MISSION6
  SAT_DEVICE_MISSION6,
#endif
};

enum SatDigital_Enum {
  SAT_PAD_UP    = 0x0001,
  SAT_PAD_DOWN  = 0x0002,
  SAT_PAD_LEFT  = 0x0004,
  SAT_PAD_RIGHT = 0x0008,
  SAT_B         = 0x0010,
  SAT_C         = 0x0020,
  SAT_A         = 0x0040,
  SAT_START     = 0x0080,
  SAT_Z         = 0x0100,
  SAT_Y         = 0x0200,
  SAT_X         = 0x0400,
  SAT_R         = 0x0800,
  SAT_L         = 0x8000
};

enum SatAnalog_Enum {
  SAT_ANALOG_X = 0,
  SAT_ANALOG_Y,
  SAT_ANALOG_L,
  SAT_ANALOG_R,
#ifdef SATLIB_ENABLE_MISSION6
  SAT_ANALOG_X2,
  SAT_ANALOG_Y2,
#endif
};


struct SaturnControllerState {
  uint8_t id = SAT_ID_NONE;
  uint16_t digital = 0xFFFF; //Dpad and buttons
  uint8_t analogX = 0x80; //Wheel center is 0x7F ? I don't have one to test.
  uint8_t analogY = 0x80;
  uint8_t analogR = 0x00;
  uint8_t analogL = 0x00;

#ifdef SATLIB_ENABLE_MISSION6
  uint8_t analogX2 = 0x80;
  uint8_t analogY2 = 0x80;
#endif

  // Mouse data (relative movement)
  int8_t mouseX = 0;
  int8_t mouseY = 0;
  uint8_t mouseFlags = 0;
  uint8_t mouseRawX = 0;
  uint8_t mouseRawY = 0;
  uint8_t mouseButtons = 0; // bit 0=Left, bit 1=Right, bit 2=Middle, bit 3=Start/C
  bool mouseOverflow = false;
  uint8_t debugControllerType = 0;
  uint8_t debugDataSize = 0;
  uint8_t debugNibbleCount = 0;
  uint8_t debugNibbles[20] = {};
  /*bool operator==(const SaturnControllerState& b) const {
    return id == b.id &&
         digital == b.digital &&
         analogX == b.analogX &&
         analogY == b.analogY &&
         analogR == b.analogR &&
         analogL == b.analogL;
  }*/
  bool operator!=(const SaturnControllerState& b) const {
    return id != b.id ||
         digital != b.digital ||
         analogX != b.analogX ||
         analogY != b.analogY ||
#ifdef SATLIB_ENABLE_MISSION6
         analogX2 != b.analogX2 ||
         analogY2 != b.analogY2 ||
#endif
         analogR != b.analogR ||
         analogL != b.analogL ||
         mouseX != b.mouseX ||
         mouseY != b.mouseY ||
         mouseFlags != b.mouseFlags ||
         mouseRawX != b.mouseRawX ||
         mouseRawY != b.mouseRawY ||
         mouseButtons != b.mouseButtons;
  }
};

class SaturnController {
  public:
    static constexpr uint8_t MEGA6_SIGNATURE_CONFIRM_HITS = 4;
    static constexpr uint8_t MEGA6_SIGNATURE_WINDOW_MISSES = 128;
    static constexpr uint8_t MEGA6_EXTRA_PAGE_GRACE_POLLS = 8;
    static constexpr uint16_t MEGA6_TYPE_GRACE_POLLS = 2048;

        
    SaturnControllerState currentState;
    SaturnControllerState lastState;

    uint16_t sixbtn_missed_polls = 0;
    uint8_t sixbtn_signature_hits = 0;
    uint8_t sixbtn_signature_window_misses = 0;
    bool sixbtn_confirmed = false;

    void reset(const bool resetId = false, bool resetPrevious = false) {
      if (resetId) {
        currentState.id = 0xFF;
        sixbtn_missed_polls = 0;
        sixbtn_signature_hits = 0;
        sixbtn_signature_window_misses = 0;
        sixbtn_confirmed = false;
      }

      currentState.digital = 0xFFFF;
      currentState.analogX = 0x80;
      currentState.analogY = 0x80;
      currentState.analogR = 0x00;
      currentState.analogL = 0x00;
#ifdef SATLIB_ENABLE_MISSION6
      currentState.analogX2 = 0x80;
      currentState.analogY2 = 0x80;
#endif
      // Reset mouse data
      currentState.mouseX = 0;
      currentState.mouseY = 0;
      currentState.mouseFlags = 0;
      currentState.mouseRawX = 0;
      currentState.mouseRawY = 0;
      currentState.mouseButtons = 0;
      currentState.mouseOverflow = false;
      currentState.debugControllerType = 0;
      currentState.debugDataSize = 0;
      currentState.debugNibbleCount = 0;
      for (uint8_t i = 0; i < sizeof(currentState.debugNibbles); ++i) {
        currentState.debugNibbles[i] = 0;
      }

      if (resetPrevious) {
        copyCurrentToLast();
      }
    }

    void copyCurrentToLast() {
      lastState.id = currentState.id;
      lastState.digital = currentState.digital;
      lastState.analogX = currentState.analogX;
      lastState.analogY = currentState.analogY;
      lastState.analogR = currentState.analogR;
      lastState.analogL = currentState.analogL;
#ifdef SATLIB_ENABLE_MISSION6
      lastState.analogX2 = currentState.analogX2;
      lastState.analogY2 = currentState.analogY2;
#endif
      // Copy state fields used by stateChanged(); debug nibble metadata is not
      // part of the input report and stays out of the hot-path copy.
      lastState.mouseX = currentState.mouseX;
      lastState.mouseY = currentState.mouseY;
      lastState.mouseFlags = currentState.mouseFlags;
      lastState.mouseRawX = currentState.mouseRawX;
      lastState.mouseRawY = currentState.mouseRawY;
      lastState.mouseButtons = currentState.mouseButtons;
      lastState.mouseOverflow = currentState.mouseOverflow;
    }


  bool deviceJustChanged() const { return currentState.id != lastState.id; }
  bool stateChanged() const { return currentState != lastState; }
  bool isAnalog() const { return (currentState.id & 0b11110000) == 0b00010000; }
  uint16_t digitalRaw() const { return currentState.digital; }  
  uint8_t hat() const { return currentState.digital & 0xF; }
  
  bool digitalPressed(const SatDigital_Enum s) const { return (currentState.digital & s) == 0; }
  bool digitalChanged (const SatDigital_Enum s) const { return ((lastState.digital ^ currentState.digital) & s) > 0; }
  bool digitalJustPressed (const SatDigital_Enum s) const { return digitalChanged(s) & digitalPressed(s); }
  bool digitalJustReleased (const SatDigital_Enum s) const { return digitalChanged(s) & !digitalPressed(s); }

  bool analogChanged (const SatAnalog_Enum s) const {
      switch (s) {
        case SAT_ANALOG_X:
          return currentState.analogX != lastState.analogX;
        case SAT_ANALOG_Y:
          return currentState.analogY != lastState.analogY;
        case SAT_ANALOG_R:
          return currentState.analogR != lastState.analogR;
        case SAT_ANALOG_L:
          return currentState.analogL != lastState.analogL;
#ifdef SATLIB_ENABLE_MISSION6
        case SAT_ANALOG_X2:
          return currentState.analogX2 != lastState.analogX2;
        case SAT_ANALOG_Y2:
          return currentState.analogY2 != lastState.analogY2;
#endif
        default:
          return false;
    }
  }

  uint8_t analog(const SatAnalog_Enum s) const {
    switch (s) {
      case SAT_ANALOG_X:
        return currentState.analogX;
      case SAT_ANALOG_Y:
        return currentState.analogY;
      case SAT_ANALOG_R:
        return currentState.analogR;
      case SAT_ANALOG_L:
        return currentState.analogL;
#ifdef SATLIB_ENABLE_MISSION6
      case SAT_ANALOG_X2:
        return currentState.analogX2;
      case SAT_ANALOG_Y2:
        return currentState.analogY2;
#endif
      default:
        return 0;
    }
  }


  SatDeviceType_Enum deviceType() const {
    if (currentState.id == SAT_ID_DIGITALPAD) { //L1001111 digital pad
      return SAT_DEVICE_PAD;
    } else if (currentState.id == SAT_ID_3DDIGITAL) {//3d pad in digital mode
      return SAT_DEVICE_PAD;
    } else if (currentState.id == SAT_ID_3DANALOG) {//3d pad in analog mode
      return SAT_DEVICE_3DPAD;
    } else if (currentState.id == SAT_ID_WHEEL) {//arcade racer wheel
      return SAT_DEVICE_WHEEL;
    } else if (currentState.id == SAT_ID_MISSION3) {//mission stick (single / 3 axis)
      return SAT_DEVICE_MISSION3;
#ifdef SATLIB_ENABLE_MISSION6
    } else if (currentState.id == SAT_ID_MISSION6) {//mission stick (dual / 6 axis)
      return SAT_DEVICE_MISSION6;
#endif
    } else if (currentState.id == SAT_ID_MEGA3) { //3btn (using saturn id)
      return SAT_DEVICE_MEGA3;
    } else if (currentState.id == SAT_ID_MEGA6) { //6btn (using saturn id)
      return SAT_DEVICE_MEGA6;
    } else if (currentState.id == SAT_ID_MOUSE) { // mouse
      #ifdef ENABLE_EXPERIMENTAL_CONSOLE_MOUSE
      return SAT_DEVICE_MOUSE;
      #else
      return SAT_DEVICE_NOTSUPPORTED;
      #endif
    } else if (currentState.id == SAT_ID_NONE) {
      return SAT_DEVICE_NONE;
    } else {
      return SAT_DEVICE_NOTSUPPORTED;
    }
  }

};

//template <uint8_t PIN_D0, uint8_t PIN_D1, uint8_t PIN_D2, uint8_t PIN_D3, uint8_t PIN_TH, uint8_t PIN_TR, uint8_t PIN_TL>
class SaturnPort {
  private:
    const uint8_t sat_D0;// DigitalPin<PIN_D0> sat_D0; //U B Z .
    const uint8_t sat_D1;// DigitalPin<PIN_D1> sat_D1; //D C Y .
    const uint8_t sat_D2;// DigitalPin<PIN_D2> sat_D2; //L A X .
    const uint8_t sat_D3;// DigitalPin<PIN_D3> sat_D3; //R S R L
    const uint8_t sat_TH;// DigitalPin<PIN_TH> sat_TH;
    const uint8_t sat_TR;// DigitalPin<PIN_TR> sat_TR;
    const uint8_t sat_TL;// DigitalPin<PIN_TL> sat_TL;
    const SatLibConfig_t config;

    uint8_t joyCount = 0;
    uint8_t multitapPorts = 0;
    DB9_TR_Enum portState = DB9_TR_INPUT;
    SaturnController controllers[SATLIB_MAX_CTRL];

    inline void __attribute__((always_inline))
    setTR(const uint8_t value) {
#if defined(ARDUINO_ARCH_RP2040)
      gpio_put(sat_TR, value);
#else
      digitalWrite(sat_TR, value);
#endif
      /*sat_TR.write(value);*/
    }
    
    inline void __attribute__((always_inline))
    setTH(const uint8_t value) {
#if defined(ARDUINO_ARCH_RP2040)
      gpio_put(sat_TH, value);
#else
      digitalWrite(sat_TH, value);
#endif
      /*sat_TH.write(value);*/
    }
    
    inline bool __attribute__((always_inline))
    readTL() const {
#if defined(ARDUINO_ARCH_RP2040)
      return gpio_get(sat_TL);
#else
      return digitalRead(sat_TL);
#endif
      /*sat_TL;*/
    }
    
    inline bool __attribute__((always_inline))
    readTR() const {
#if defined(ARDUINO_ARCH_RP2040)
      return gpio_get(sat_TR);
#else
      return digitalRead(sat_TR);
#endif
      /*sat_TR;*/
    }
    
    inline uint8_t __attribute__((always_inline))
    waitTL(const uint8_t state) const { //returns 1 when reach timeout
      uint16_t t_out = 5000;  // Increased timeout for Saturn 6-player multitap
      const uint8_t compare = !state;
      while (readTL() == compare) {
        delayMicroseconds(4);
        if (t_out-- == 1)
          return 1;
      }
      return 0;
    }

    inline uint8_t __attribute__((always_inline))
    setTRAndWaitTL(const  uint8_t trValue) {
      setTR(trValue);
      return waitTL(trValue);
    }
    
    inline void __attribute__((always_inline))
    setTR_Mode(const DB9_TR_Enum mode) {
      if(portState != mode) {
        portState = mode;
        if(mode == DB9_TR_OUTPUT) {
          pinMode(sat_TR, OUTPUT);digitalWrite(sat_TR, HIGH);//sat_TR.config(OUTPUT, HIGH);
        } else {
          pinMode(sat_TR, INPUT_PULLUP);//sat_TR.config(INPUT, HIGH);
        }
      }
    }

    uint8_t __not_in_flash_func(readMegadriveBits)() { //6 bits
      uint8_t nibble = readNibble();
      if (portState != DB9_TR_OUTPUT) {//only used on megadrive. not saturn
        bitWrite(nibble, 4, readTL());
        bitWrite(nibble, 5, readTR());
      }
      return nibble;
    }

    inline uint8_t __attribute__((always_inline))
    __not_in_flash_func(readNibble)() const {
#if defined(ARDUINO_ARCH_RP2040)
      const uint32_t pins = gpio_get_all();
      return (((pins >> sat_D3) & 1) << 3) |
             (((pins >> sat_D2) & 1) << 2) |
             (((pins >> sat_D1) & 1) << 1) |
             ((pins >> sat_D0) & 1);
#else
      return (digitalRead(sat_D3) << 3) + (digitalRead(sat_D2) << 2) + (digitalRead(sat_D1) << 1) + digitalRead(sat_D0);
#endif
    }
    //readNibble() const { return (sat_D3 << 3) + (sat_D2 << 2) + (sat_D1 << 1) + sat_D0; }

    static inline bool isMegadriveSubtypeId(const uint8_t id) {
      return id == SAT_ID_MEGA3 || id == SAT_ID_MEGA6;
    }

    static int8_t decodeMouseMagnitude(uint8_t magnitude, bool sign) {
      if (magnitude == 0) {
        return 0;
      }
      if (magnitude > 127) {
        magnitude = 127;
      }
      const int16_t value = sign ? -static_cast<int16_t>(magnitude)
                                 : static_cast<int16_t>(magnitude);
      return static_cast<int8_t>(value);
    }

    void __not_in_flash_func(decodeMouseNibbles)(SaturnController& sc, const uint8_t mouseNibbles[6]) {
      // Nibble 0: Yo, Xo, Ys, Xs. Nibble 1: C, M, R, L.
      const uint8_t flags = mouseNibbles[0];
      const bool xOverflow = (flags >> 2) & 1;
      const bool yOverflow = (flags >> 3) & 1;
      const bool xSign = (flags >> 0) & 1;
      const bool ySign = (flags >> 1) & 1;
      const uint8_t xVal = (mouseNibbles[2] << 4) | mouseNibbles[3];
      const uint8_t yVal = (mouseNibbles[4] << 4) | mouseNibbles[5];

      sc.currentState.mouseFlags = flags;
      sc.currentState.mouseButtons = mouseNibbles[1];
      sc.currentState.mouseRawX = xVal;
      sc.currentState.mouseRawY = yVal;
      sc.currentState.mouseX = decodeMouseMagnitude(xVal, xSign);
      sc.currentState.mouseY = decodeMouseMagnitude(yVal, ySign);
      sc.currentState.mouseOverflow = xOverflow || yOverflow;
    }

    void __not_in_flash_func(readSatPort)() {
      uint8_t nibble_0;
      uint8_t nibble_1;

      if (config.enable_saturn) {
        // Precharge data lines to reduce false-low reads on marginal Saturn connectors.
        // Genesis-only mode keeps these as inputs to avoid pinMode churn in the hot path.
        pinMode(sat_D0, OUTPUT); digitalWrite(sat_D0, HIGH);
        pinMode(sat_D1, OUTPUT); digitalWrite(sat_D1, HIGH);
        pinMode(sat_D2, OUTPUT); digitalWrite(sat_D2, HIGH);
        pinMode(sat_D3, OUTPUT); digitalWrite(sat_D3, HIGH);
        delayMicroseconds(2);
        pinMode(sat_D0, INPUT_PULLUP);
        pinMode(sat_D1, INPUT_PULLUP);
        pinMode(sat_D2, INPUT_PULLUP);
        pinMode(sat_D3, INPUT_PULLUP);
        delayMicroseconds(2);
      }

      setTH(HIGH);
      delayMicroseconds(4);
      nibble_0 = readMegadriveBits();

      setTH(LOW);
      delayMicroseconds(4);
      nibble_1 = readMegadriveBits();

      #if defined(AUTODETECT_DEBUG_CDC)
        // Keep serial diagnostics out of normal polling; printing here can
        // stretch the Megadrive 6-button TH sequence past the pad timeout.
        static uint32_t last_nibble_debug = 0;
        static uint8_t last_n0 = 0xFF, last_n1 = 0xFF;
        if ((nibble_0 != last_n0 || nibble_1 != last_n1) || (millis() - last_nibble_debug > 5000)) {
          last_nibble_debug = millis();
          last_n0 = nibble_0;
          last_n1 = nibble_1;
          printf("SaturnLib: nibble_0=0x%02X nibble_1=0x%02X\n", nibble_0, nibble_1);
        }
      #endif

      // If Saturn multitap was detected at power-on, force Saturn 3-wire protocol
      // This prevents misdetection as Megadrive due to timing variations
      if (multitapPorts == TAP_SAT_PORTS) {
        setTR_Mode(DB9_TR_OUTPUT);
        readThreeWire();
      } else if (config.enable_saturn && config.enable_sattap &&
                 (nibble_0 & 0b00001111) == 0b00000100 && (nibble_1 & 0b00001111) == 0b00000001) {
        // Saturn multitap: ID = 0b0100 (device type 4), handshake = 0b0001
        // Must check BEFORE digital pad since both have nibble_0 & 0b0111 == 0b0100
        setTR_Mode(DB9_TR_OUTPUT);
        readThreeWire();
      } else if (config.enable_saturn && (nibble_0 & 0b00000111) == 0b00000100) { //Saturn ID
        //debugln (F("DIGITAL-PAD"));
        setTR_Mode(DB9_TR_OUTPUT);
        readDigitalPad(nibble_0 & 0b00001111, nibble_1 & 0b00001111);
      } else if (config.enable_saturn && (nibble_0 & 0b00001111) == 0b00000001 && (nibble_1 & 0b00001111) == 0b00000001) { //Saturn ID
        //debugln (F("3-WIRE-HANDSHAKE"));
        setTR_Mode(DB9_TR_OUTPUT);
        readThreeWire();
      } else if (config.enable_saturn &&
                 (((~nibble_0) & 0b00000111) == 0b00000100 ||
                  (((~nibble_0) & 0b00001111) == 0b00000001 && ((~nibble_1) & 0b00001111) == 0b00000001))) {
        // Inverted-ID fallback for noisy Saturn wiring on Adapt 2.
        // Keep data-path handling simple and robust: parse as digital pad.
        setTR_Mode(DB9_TR_OUTPUT);
        uint8_t n0 = nibble_0 & 0x0F;
        uint8_t n1 = nibble_1 & 0x0F;

        // Common flaky signature is D0 stuck low (0xE/0xF or 0xE/0xE), which
        // otherwise latches phantom Up/L. Normalize D0 to released here.
        if (n0 == 0x0E && (n1 == 0x0F || n1 == 0x0E)) {
          n0 |= 0x01;
          n1 |= 0x01;
        }
        readDigitalPad(n0, n1);
      } else if (config.enable_megadrive && multitapPorts != TAP_SAT_PORTS && (nibble_1 & 0b00001100) == 0b00000000) { //Megadrive ID
        //debugln (F("MEGADRIVE"));
        setTR_Mode(DB9_TR_INPUT);
        readMegadrivePad(nibble_0, nibble_1);
      } else if (config.enable_megadrive && config.enable_megatap && (nibble_0 & 0b00001111) == 0b00000011 && (nibble_1 & 0b00001111) == 0b00001111) { //Megadrive multitap
        setTR_Mode(DB9_TR_OUTPUT);
        //debugln (F("MEGADRIVE MULTITAP"));
//        #ifdef SATLIB_ENABLE_MEGATAP
          readMegaMultiTap();
//        #endif
      } else {
        setTR_Mode(DB9_TR_INPUT);
      }
    
      setTH(HIGH);
    
      if (portState == DB9_TR_OUTPUT)
        setTR(HIGH);
      
      delayMicroseconds(10);
    }

//    #ifdef SATLIB_ENABLE_MEGATAP
    void __not_in_flash_func(readMegaMultiTap)() {
      uint8_t joyIndex = 0;
      uint8_t nibble_0;
      uint16_t tapPortState = 0b0;
      uint8_t nibbles;
      uint8_t tr = 0;
      uint8_t tl_timeout;
      uint8_t i;

      multitapPorts = TAP_MEGA_PORTS;
    
      //read first two nibbles. should be zero and zero
      for (i = 0; i < 2; i++) {
        tl_timeout = setTRAndWaitTL(i);
        if (tl_timeout)
          return;
        delayMicroseconds(4);
      }
    
      //read each port controller ID and store the 4 values in the 16 bits variable
      for (i = 0; i < 16; i+=4) {
        tl_timeout = setTRAndWaitTL(tr);
        if (tl_timeout)
          return;
        delayMicroseconds(4);
        
        tapPortState |= (readNibble() << i);
    
        tr ^= 1;
      }

      //SaturnController& sc = getSaturnController(joyIndex);
      //now read each port
      for (i = 0; i < 16; i+=4) {
        nibble_0 = (tapPortState >> i) & 0b1111;
    
        if (nibble_0 == 0b0000) //3 button. 2 nibbles
          nibbles = 2;
        else if (nibble_0 == 0b0001) //6 buttons. 3 nibbles
          nibbles = 3;
        else if (nibble_0 == 0b0010) //mouse. 6 nibbles
          nibbles = 6;
        else //non-connection. 0 reads
          continue;
    
        if (nibbles == 6) { // Mouse: read 6 nibbles of data
            joyIndex = joyCount++;
            SaturnController& sc = getSaturnController(joyIndex);
            sc.currentState.id = SAT_ID_MOUSE;

            uint8_t mouseNibbles[6];
            for (uint8_t x = 0; x < nibbles; x++) {
                tl_timeout = setTRAndWaitTL(tr);
                if (tl_timeout)
                    return;
                delayMicroseconds(4);
                mouseNibbles[x] = readNibble();
                tr ^= 1;
            }

            decodeMouseNibbles(sc, mouseNibbles);

        } else {
            joyIndex = joyCount++;
            SaturnController& sc = getSaturnController(joyIndex);
            sc.currentState.id = 0b11100000 ^ nibble_0; //megadrive id plus device id
            for (uint8_t x = 0; x < nibbles; x++) {
                tl_timeout = setTRAndWaitTL(tr);
                if (tl_timeout)
                    return;
                delayMicroseconds(4);

                setControlValues(sc, x, readNibble());

                tr ^= 1;
            }
        }
        
        //delayMicroseconds(40);
      }
    }
//    #endif
    
    void __not_in_flash_func(readThreeWire)() {
      uint8_t nibble_0;
      uint8_t nibble_1;
      uint8_t tl_timeout;
    
      for (uint8_t i = 0; i < 2; i++) {
        tl_timeout = setTRAndWaitTL(i);
        if (tl_timeout)
          return;
    
        delayMicroseconds(4);
    
        if (i == 0)
          nibble_0 = readNibble();
        else
          nibble_1 = readNibble();
      }
      if (config.enable_sattap && nibble_0 == 0b00000100 && nibble_1 == 0b00000001) {
        //debugln (F("6P MULTI-TAP"));
//        #ifdef SATLIB_ENABLE_SATTAP
        readMultitap();
//        #endif
      } else {
        readThreeWireController(nibble_0, nibble_1);
      }
    }
    
//    #ifdef SATLIB_ENABLE_SATTAP
    void __not_in_flash_func(readMultitap)() {
      uint8_t i;
      uint8_t tl_timeout;

      multitapPorts = TAP_SAT_PORTS;
    
      //Multitap header is fixed: 6 (ports) and zero (len)
      for (i = 0; i < 2; i++) { // set TR low and high
        tl_timeout = setTRAndWaitTL(i);
        if (tl_timeout)
          return;
        delayMicroseconds(4);
      }
      //delayMicroseconds(5);
    
      for (i = 0; i < 6; i++) {// Read the 6 multitap ports
        readThreeWire();
        delayMicroseconds(4);
      }
    
      for (i = 0; i < 2; i++) { //set TR low and high
        tl_timeout = setTRAndWaitTL(i);
        if (tl_timeout)
          return;
        delayMicroseconds(4);
      }
      
    }
//    #endif
    
    void __not_in_flash_func(readUnhandledPeripheral)(const uint8_t len) {
      const uint8_t nibbles = len * 2;
      uint8_t tr = 0;
      uint8_t tl_timeout;
    
      for (uint8_t i = 0; i < nibbles; i++) {
        tl_timeout = setTRAndWaitTL(tr);
        if (tl_timeout)
          return;
        delayMicroseconds(4);
    
        tr ^= 1;
      }
    }

    void __not_in_flash_func(setControlValues)(SaturnController& sc, const uint8_t page, const uint8_t nibbleTMP) const {
      if (page == 0) { //HAT RLDU
        sc.currentState.digital = (sc.currentState.digital & 0xFFF0) + nibbleTMP;
      } else if (page == 1) { //SACB
        sc.currentState.digital = (sc.currentState.digital & 0xFF0F) + (nibbleTMP << 4);
      } else if (page == 2) { //RXYZ
        sc.currentState.digital = (sc.currentState.digital & 0xF0FF) + (nibbleTMP << 8);
      } else if (page == 3) { //L
        sc.currentState.digital = (sc.currentState.digital & 0x0FFF) + (nibbleTMP << 12);
      } else if (page == 4) { //X axis
        sc.currentState.analogX = nibbleTMP;
      } else if (page == 5) { //Y axis
        sc.currentState.analogY = nibbleTMP;
      } else if (page == 6) { //RT axis
        sc.currentState.analogR = nibbleTMP;
      } else if (page == 7) { //LT axis
        sc.currentState.analogL = nibbleTMP;
#ifdef SATLIB_ENABLE_MISSION6
      } else if (page == 8) { //X axis
        sc.currentState.analogX2 = nibbleTMP;
      } else if (page == 9) { //Y axis
        sc.currentState.analogY2 = nibbleTMP;
#endif
      }
    }

    void __not_in_flash_func(readThreeWireController)(const uint8_t controllerType, const uint8_t dataSize) {
      const uint8_t nibbles = dataSize * 2;
      uint8_t nibbleTMP;
      uint8_t nibble_0 = 0b0;
      uint8_t nibble_1;
      uint8_t tr = 0;
      uint8_t tl_timeout;
      //bool isAnalog = controllerType == 0b00000001;
    
      //check if it's a supported device
    
      if (controllerType == 0b00000000 && dataSize == 0b00000010) {
        //debugln (F("DIGITAL"));
      } else if (controllerType == 0b00000001 && dataSize == 0b00000110) {
        //debugln (F("ANALOG"));
      } else if (controllerType == 0b00000001 && dataSize == 0b00000011) {
        //debugln (F("WHEEL"));
      } else if (controllerType == 0b00000010 && dataSize == 0b00000011) {
        //debugln (F("MOUSE"));
      } else if (controllerType == 0b00000001 && dataSize == 0b00000101) {
        //debugln (F("MISSION STICK THREE-AXIS MODE"));
#ifdef SATLIB_ENABLE_MISSION6
      } else if (controllerType == 0b00000001 && dataSize == 0b00001001) {
        //debugln (F("MISSION STICK SIX-AXIS MODE"));
#endif
      } else if (config.enable_megadrive && controllerType == 0b00001110 && dataSize < 0b00000011) { //megadrive 3 or 6 button pad. ignore mouse
        //debugln (F("MEGADRIVE"));
      } else if (controllerType == 0b00001111 && dataSize == 0b00001111) {
        //debugln (F("NONE"));
        return;
      } else {
        //debugln (F("UNKNOWN"));
        readUnhandledPeripheral(dataSize);
        return;
      }

      if (controllerType == 0b00000010 && dataSize == 0b00000011) {
        const uint8_t joyIndex = joyCount++;
        SaturnController& sc = getSaturnController(joyIndex);
        sc.currentState.id = SAT_ID_MOUSE;

        uint8_t mouseNibbles[6];
        for (uint8_t i = 0; i < nibbles; i++) {
          tl_timeout = setTRAndWaitTL(tr);
          if (tl_timeout)
            return;

          delayMicroseconds(2);
          mouseNibbles[i] = readNibble();
          tr ^= 1;
        }

        decodeMouseNibbles(sc, mouseNibbles);
        return;
      }

      const uint8_t joyIndex = joyCount++;
      SaturnController& sc = getSaturnController(joyIndex); //setControlValues
      
      if(controllerType == 0b00001110) //megadrive id
        sc.currentState.id = (controllerType << 4) ^ (dataSize-1);
      else
        sc.currentState.id = (controllerType << 4) ^ dataSize;

      sc.currentState.debugControllerType = controllerType;
      sc.currentState.debugDataSize = dataSize;
      sc.currentState.debugNibbleCount = min((uint8_t)sizeof(sc.currentState.debugNibbles), nibbles);
    
      for (uint8_t i = 0; i < nibbles; i++) {
        tl_timeout = setTRAndWaitTL(tr);
        if (tl_timeout)
          return;
          
        delayMicroseconds(2);
    
        nibbleTMP = readNibble();
        if (i < sizeof(sc.currentState.debugNibbles)) {
          sc.currentState.debugNibbles[i] = nibbleTMP & 0x0F;
        }
        tr ^= 1;
    
        if (i < 3) { // RLDU SACB RXYZ        Digital/Wheel/Mission3/Analog/Mission6
          setControlValues(sc, i, nibbleTMP);
        } else if(i == 3) { //L               Digital/Wheel/Mission3/Analog/Mission6
          nibbleTMP &= 0b00001000;
          setControlValues(sc, i, nibbleTMP);
        } else if (i == 4) { //X axis         -------/Wheel/Mission3/Analog/Mission6
          nibble_0 = nibbleTMP;
        } else if (i == 5) { //X axis         -------/Wheel/Mission3/Analog/Mission6
          nibble_1 = (nibble_0 & 0xF) << 4 | (nibbleTMP & 0xF);
          setControlValues(sc, 4, nibble_1);
        } else if (i == 6) { //Y axis         -------/-----/Mission3/Analog/Mission6
          nibble_0 = nibbleTMP;
        } else if (i == 7) { //Y axis         -------/-----/Mission3/Analog/Mission6
          nibble_1 = (nibble_0 & 0xF) << 4 | (nibbleTMP & 0xF);
          setControlValues(sc, 5, nibble_1);
        } else if (i == 8) { //RT analog      -------/-----/Mission3/Analog/Mission6
          nibble_0 = nibbleTMP;
        } else if (i == 9) { //RT analog      -------/-----/Mission3/Analog/Mission6
          nibble_1 = (nibble_0 & 0xF) << 4 | (nibbleTMP & 0xF);
          setControlValues(sc, 6, nibble_1);
#ifndef SATLIB_ENABLE_MISSION6   
        } else if (i == 10) { //LT analog     -------/-----/--------/Analog/--------
          nibble_0 = nibbleTMP;
        } else if (i == 11) { //LT analog     -------/-----/--------/Analog/--------
          nibble_1 = (nibble_0 & 0xF) << 4 | (nibbleTMP & 0xF);
          setControlValues(sc, 7, nibble_1);
#else
        } else if (i == 10 || i == 16) { //LT analog  -------/-----/--------/Analog/Mission6
          nibble_0 = nibbleTMP;
        } else if (i == 11 || i == 17) { //LT analog  -------/-----/--------/Analog/Mission6
          nibble_1 = (nibble_0 & 0xF) << 4 | (nibbleTMP & 0xF);
          setControlValues(sc, 7, nibble_1);
        } else if (i == 12) { //X axis         -------/-----/--------/------/Mission6
          nibble_0 = nibbleTMP;
        } else if (i == 13) { //X axis         -------/-----/--------/------/Mission6
          nibble_1 = (nibble_0 & 0xF) << 4 | (nibbleTMP & 0xF);
          setControlValues(sc, 8, nibble_1);
        } else if (i == 14) { //Y axis         -------/-----/--------/------/Mission6
          nibble_0 = nibbleTMP;
        } else if (i == 15) { //Y axis         -------/-----/--------/------/Mission6
          nibble_1 = (nibble_0 & 0xF) << 4 | (nibbleTMP & 0xF);
          setControlValues(sc, 9, nibble_1);
#endif
        }








      }
    }

    void __not_in_flash_func(readMegadrivePad)(uint8_t nibble_0, uint8_t nibble_1) {
      const uint8_t joyIndex = joyCount++;
      SaturnController& sc = getSaturnController(joyIndex);
      sc.currentState.id = SAT_ID_MEGA3; //initialize as 3btn. later will check for 6btn

      //If on first read R and L are pressed then ignore reading.
      if((nibble_0 & 0b00001100) == 0b00000000)
        return;

      // RLDU
      setControlValues(sc, 0, nibble_0 & 0b00001111);
    
      // SACB
      setControlValues(sc, 1, (nibble_0 >> 4) | (nibble_1 >> 2));

      //check if is 6-button pad;
      uint8_t th = 1;
      for (uint8_t i = 0; i < 4; i++) { //set TH high-low-high-low
        setTH(th);
        delayMicroseconds(8);
        th ^= 1;
      }
     
      nibble_0 = readMegadriveBits();
      const bool sixButtonSignature = ((nibble_0 & 0b00001111) == 0b0);

      // A worn connector or a direction transition can produce one all-low
      // sample. Require several signatures in a bounded window before exposing
      // Mega6, then tolerate missed probes because genuine six-button pads can
      // temporarily report as three-button when polled quickly.
      if (sixButtonSignature) {
        if (!sc.sixbtn_confirmed) {
          if (sc.sixbtn_signature_hits < SaturnController::MEGA6_SIGNATURE_CONFIRM_HITS) {
            ++sc.sixbtn_signature_hits;
          }
          if (sc.sixbtn_signature_hits >= SaturnController::MEGA6_SIGNATURE_CONFIRM_HITS) {
            sc.sixbtn_confirmed = true;
          }
        }
        sc.sixbtn_signature_window_misses = 0;
        sc.sixbtn_missed_polls = 0;
      } else if (sc.sixbtn_confirmed) {
        if (sc.sixbtn_missed_polls < SaturnController::MEGA6_TYPE_GRACE_POLLS) {
          ++sc.sixbtn_missed_polls;
        }
        if (sc.sixbtn_missed_polls >= SaturnController::MEGA6_TYPE_GRACE_POLLS) {
          sc.sixbtn_confirmed = false;
          sc.sixbtn_signature_hits = 0;
          sc.sixbtn_signature_window_misses = 0;
        }
      } else if (sc.sixbtn_signature_hits > 0) {
        if (sc.sixbtn_signature_window_misses <
            SaturnController::MEGA6_SIGNATURE_WINDOW_MISSES) {
          ++sc.sixbtn_signature_window_misses;
        }
        if (sc.sixbtn_signature_window_misses >=
            SaturnController::MEGA6_SIGNATURE_WINDOW_MISSES) {
          sc.sixbtn_signature_hits = 0;
          sc.sixbtn_signature_window_misses = 0;
        }
      }

      if (sc.sixbtn_confirmed) {
        sc.currentState.id = SAT_ID_MEGA6;
      }

      if (sixButtonSignature) {
        setTH(HIGH);
        delayMicroseconds(8);
        nibble_0 = readMegadriveBits();
    
        setTH(LOW);
        delayMicroseconds(8);
        nibble_1 = readMegadriveBits();
        //11MXYZ
        setControlValues(sc, 2, nibble_0 & 0b00001111);
    
        //SA1111
        //8bitdo M30 extra buttons
        //...S.H STAR and HOME
        
        //use HOME button as the missing L from saturn
        #ifdef SATLIB_ENABLE_8BITDO_HOME_BTN
          setControlValues(sc, 3, nibble_1 << 3);
        #endif
      } else if (!sc.sixbtn_confirmed ||
                 sc.sixbtn_missed_polls >
                     SaturnController::MEGA6_EXTRA_PAGE_GRACE_POLLS) {
        // Never let a false or stale six-button page hold XYZ/Mode indefinitely.
        setControlValues(sc, 2, 0x0F);
        #ifdef SATLIB_ENABLE_8BITDO_HOME_BTN
          setControlValues(sc, 3, 0x0F);
        #endif
      }
    }

    void __not_in_flash_func(readDigitalPad)(const uint8_t nibble_0, const uint8_t nibble_1) {
      const uint8_t joyIndex = joyCount++;
      uint8_t nibbleTMP;
      //uint8_t currentHatState;

      SaturnController& sc = getSaturnController(joyIndex);
      sc.currentState.id = SAT_ID_DIGITALPAD;

      // L100
      //debugln (nibble_0, BIN);
      setControlValues(sc, 3, nibble_0);
    
      // RLDU
      //setTH(LOW);
      //setTR(HIGH);
      //delayMicroseconds(4);
      //nibbleTMP = readNibble();
      //debugln (nibble_1, BIN);
      setControlValues(sc, 0, nibble_1);
    
      // SACB
      setTH(HIGH);
      setTR(LOW);
      delayMicroseconds(4);
      nibbleTMP = readNibble();
      //debugln (nibbleTMP, BIN);
      setControlValues(sc, 1, nibbleTMP);
    
      // RYXZ
      setTH(LOW);
      setTR(LOW);
      delayMicroseconds(4);
      nibbleTMP = readNibble();
      //debugln (nibbleTMP, BIN);
      setControlValues(sc, 2, nibbleTMP);
    }

  public:
    SaturnPort(uint8_t PIN_D0, uint8_t PIN_D1, uint8_t PIN_D2, uint8_t PIN_D3, uint8_t PIN_TH, uint8_t PIN_TR, uint8_t PIN_TL, SatLibConfig_t _config = SatLibConfig_default)
      : sat_D0(PIN_D0), sat_D1(PIN_D1), sat_D2(PIN_D2), sat_D3(PIN_D3), sat_TH(PIN_TH), sat_TR(PIN_TR), sat_TL(PIN_TL), config(_config) {
    }

    void begin(){
      //init output pins
      pinMode(sat_TH, OUTPUT); digitalWrite(sat_TH, HIGH);//sat_TH.config(OUTPUT, HIGH);
    
      //init input pins with pull-up
      pinMode(sat_D0, INPUT_PULLUP);//sat_D0.config(INPUT, HIGH);
      pinMode(sat_D1, INPUT_PULLUP);//sat_D1.config(INPUT, HIGH);
      pinMode(sat_D2, INPUT_PULLUP);//sat_D2.config(INPUT, HIGH);
      pinMode(sat_D3, INPUT_PULLUP);//sat_D3.config(INPUT, HIGH);
      pinMode(sat_TL, INPUT_PULLUP);//sat_TL.config(INPUT, HIGH);
    
      //init TR pin. Can change to input or output during execution.
      //Defaults to input (with pullup) to support megadrive controllers.
      pinMode(sat_TR, INPUT_PULLUP);//sat_TR.config(INPUT, HIGH);

      multitapPorts = 0;
      //reset all devices to default values
      for (uint8_t i = 0; i < SATLIB_MAX_CTRL; i++) {
        getSaturnController(i).reset(true, true);
      }
    }
    
    void __not_in_flash_func(update)(){
      uint8_t lastJoyCount = joyCount;

      //keep last data
      for (uint8_t i = 0; i < lastJoyCount; i++) {
        getSaturnController(i).copyCurrentToLast();
      }
      
      joyCount = 0;
      multitapPorts = 0;
      
      //resetAll();
      
      readSatPort();

      //if device changed without connect/disconnect. eg: 3d pad analog <> digital
      for (uint8_t i = 0; i < joyCount; i++) {
        SaturnController& sc = getSaturnController(i);
        const bool megadriveSubtypeOnlyChange =
            isMegadriveSubtypeId(sc.currentState.id) &&
            isMegadriveSubtypeId(sc.lastState.id);
        if (sc.currentState.id != sc.lastState.id && !megadriveSubtypeOnlyChange) {
          //debugln(F("Resetting changed device"));
          sc.reset(false, false);
        }
      }

      //reset disconnected device status
      if (lastJoyCount > joyCount) {
        for (uint8_t i = joyCount; i < lastJoyCount; i++) {
          //debugln(F("Resetting disconnected device"));
          getSaturnController(i).reset(true, false);
        }
      }
      
    }

    //Call only on arduino power-on to check for multitap connected
    void detectMultitap() {
      const uint8_t nibbles = 2;
      uint8_t nibble_0;
      uint8_t nibble_1;
      uint8_t nibbleTMP;
      uint8_t tr = 0;
      uint8_t tl_timeout;

      //setTH(HIGH);
      //setTR(HIGH);
      //delayMicroseconds(4);

      tl_timeout = waitTL(1); //Device is not ready yet. Saturn and Mega multitap hold TL high while initializing.
      if (tl_timeout) {
        printf("detectMultitap: TL timeout\n");
        return;
      }

      //readMegadriveBits
      nibble_0 = readNibble();

      setTH(LOW);
      delayMicroseconds(4);
      nibble_1 = readNibble();

      printf("detectMultitap: nibble_0=0x%02X nibble_1=0x%02X\n", nibble_0, nibble_1);

      if ((nibble_0 & 0b00001111) == 0b00000011 && (nibble_1 & 0b00001111) == 0b00001111) { //Megadrive multitap
        setTR_Mode(DB9_TR_OUTPUT);
        multitapPorts = TAP_MEGA_PORTS;
      } else if (nibble_0 == 0b00000001 && nibble_1 == 0b00000001) { //SATURN 3-WIRE-HANDSHAKE
        setTR_Mode(DB9_TR_OUTPUT);
        for (uint8_t i = 0; i < nibbles; i++) {
          tl_timeout = setTRAndWaitTL(i);
          if (tl_timeout)
            return;
          delayMicroseconds(4);
    
          nibbleTMP = readNibble();
    
          if (i == 0)
            nibble_0 = nibbleTMP;
          else
            nibble_1 = nibbleTMP;
    
          tr ^= 1;
        }
    
        if (nibble_0 == 0b00000100 && nibble_1 == 0b00000001) //Is a multitap
          multitapPorts = TAP_SAT_PORTS;
      }
    
      //reset TH and TR to high
      setTH(HIGH);
      
      if (portState == DB9_TR_OUTPUT)
        setTR(HIGH);
    
      delayMicroseconds(4);
    }

    SaturnController& getSaturnController(const uint8_t i) { return controllers[min(i, SATLIB_MAX_CTRL)]; }

    uint8_t getMultitapPorts() const { return multitapPorts; }
    uint8_t getControllerCount() const { return joyCount; }
};

#endif
