/*******************************************************************************
 * Snes controller input library.
 * https://github.com/sonik-br/SnesLib
 * 
 * The library depends on greiman's DigitalIO library.
 * https://github.com/greiman/DigitalIO
 * 
 * I recommend the usage of SukkoPera's fork of DigitalIO as it supports a few more platforms.
 * https://github.com/SukkoPera/DigitalIO
 * 
 * 
 * Works with common snes pad, ntt pad and multitap.
 * Also works with common NES pad, VirtualBoy pad.
 * DO NOT connect lightguns and other unsuported controllers
 * Multitap is detected only when powering the arduino.
*/

#ifndef SNESLIB_H_
#define SNESLIB_H_

#if defined(ARDUINO_ARCH_RP2040)
#include <hardware/gpio.h>
#include <pico/platform.h>
#endif


//Enable SNES_ENABLE_MULTITAP or SNES_MULTI_CONNECTION. Don't enable both.
//On SNES_MULTI_CONNECTION all controllers shares the same CLOCK and LATCH pins. And each controller have it's own DATA pin.
//SNES_MULTI_CONNECTION can be set to 2, 3, or 4. If requiring more then must edit the lib.

//#define SNES_ENABLE_MULTITAP
//#define SNES_MULTI_CONNECTION 2

// #ifdef SNES_ENABLE_MULTITAP
//   #define SNES_MAX_CTRL 4
// #else
//   #ifdef SNES_MULTI_CONNECTION
//     #define SNES_MAX_CTRL SNES_MULTI_CONNECTION
//   #else
//     #define SNES_MAX_CTRL 1
//   #endif
// #endif
#define SNES_MAX_CTRL 4

enum SnesDeviceType_Enum {
  SNES_DEVICE_NONE = 0,
  SNES_DEVICE_NOTSUPPORTED,
  SNES_DEVICE_NES,
  SNES_DEVICE_NES_FOURSCORE,  // NES Four Score adapter (4 players)
  SNES_DEVICE_NES_POWERPAD,   // NES Power Pad (12 buttons via D3/D4)
  SNES_DEVICE_NES_ZAPPER,     // NES Zapper (trigger on D3, light sensor on D4)
  SNES_DEVICE_PAD,
  SNES_DEVICE_NTT,
  SNES_DEVICE_VB,
  SNES_DEVICE_MOUSE
};

enum SnesDigital_Enum {
  SNES_B      = 0x0001, //VB Right D-pad Down, NES A
  SNES_Y      = 0x0002, //VB Right D-pad Left, NES B
  SNES_SELECT = 0x0004,
  SNES_START  = 0x0008,
  SNES_UP     = 0x0010,
  SNES_DOWN   = 0x0020,
  SNES_LEFT   = 0x0040,
  SNES_RIGHT  = 0x0080,
  SNES_A      = 0x0100, //VB Right D-pad Right
  SNES_X      = 0x0200, //VB Right D-pad Up
  SNES_L      = 0x0400,
  SNES_R      = 0x0800 
};

// NES Four Score signature bytes (bits 16-23 of 24-bit read)
// Port 1 ($4016): signature = 0x10 (bit 4 set)
// Port 2 ($4017): signature = 0x20 (bit 5 set)
#define NES_FOURSCORE_SIG_PORT1 0x10
#define NES_FOURSCORE_SIG_PORT2 0x20

enum SnesDigitalNTT_Enum {
  SNES_NTT_0 = 0x0001, //VB B
  SNES_NTT_1 = 0x0002, //VB A
  SNES_NTT_2 = 0x0004,
  SNES_NTT_3 = 0x0008,
  SNES_NTT_4 = 0x0010,
  SNES_NTT_5 = 0x0020,
  SNES_NTT_6 = 0x0040,
  SNES_NTT_7 = 0x0080,
  SNES_NTT_8 = 0x0100,
  SNES_NTT_9 = 0x0200,
  SNES_NTT_STAR = 0x0400,
  SNES_NTT_HASH = 0x0800,
  SNES_NTT_DOT = 0x1000,
  SNES_NTT_C = 0x2000,
  SNES_NTT_UNK = 0x4000, //NOT USED
  SNES_NTT_EQUAL = 0x8000
};

struct SnesControllerState {
  uint8_t id = SNES_DEVICE_NONE;
  uint16_t digital = 0x0; //Dpad and common buttons
  uint16_t extended = 0x0; //Extended data

  // Mouse data
  int8_t mouseX = 0;
  int8_t mouseY = 0;
  uint8_t mouseButtons = 0; // bit 0=Left, bit 1=Right
  uint8_t mouseSensitivity = 0; // 0=low, 1=medium, 2=high

  // Power Pad data (12 buttons, bits 0-11)
  uint16_t powerPadButtons = 0;

  // Zapper data
  bool zapperTrigger = false;     // Trigger button pressed
  bool zapperLightSense = false;  // Light sensor detecting bright area

  bool operator!=(const SnesControllerState& b) const {
    return id != b.id ||
         digital != b.digital ||
         extended != b.extended ||
         mouseX != b.mouseX ||
         mouseY != b.mouseY ||
         mouseButtons != b.mouseButtons ||
         powerPadButtons != b.powerPadButtons ||
         zapperTrigger != b.zapperTrigger ||
         zapperLightSense != b.zapperLightSense;
  }
};

class SnesController {
  public:
        
    SnesControllerState currentState;
    SnesControllerState lastState;

    void reset(const bool resetId = false, bool resetPrevious = false) {
      if (resetId)
        currentState.id = 0xFF;

      currentState.digital = 0x0;
      currentState.extended = 0x0;
      currentState.mouseX = 0;
      currentState.mouseY = 0;
      currentState.mouseButtons = 0;
      currentState.mouseSensitivity = 0;
      currentState.powerPadButtons = 0;
      currentState.zapperTrigger = false;
      currentState.zapperLightSense = false;

      if (resetPrevious) {
        copyCurrentToLast();
      }
    }

    void copyCurrentToLast() {
      lastState.id = currentState.id;
      lastState.digital = currentState.digital;
      lastState.extended = currentState.extended;
      lastState.mouseX = currentState.mouseX;
      lastState.mouseY = currentState.mouseY;
      lastState.mouseButtons = currentState.mouseButtons;
      lastState.mouseSensitivity = currentState.mouseSensitivity;
      lastState.powerPadButtons = currentState.powerPadButtons;
      lastState.zapperTrigger = currentState.zapperTrigger;
      lastState.zapperLightSense = currentState.zapperLightSense;
    }

    bool deviceJustChanged() const {
      return (currentState.id != lastState.id) ||
      (currentState.id == 0x0 && currentState.extended == 0xFFFF && lastState.extended != 0xFFFF); //SNES
    }

    bool stateChanged() const { return currentState != lastState; }
    uint16_t digitalRaw() const { return currentState.digital; }
    uint16_t extendedRaw() const { return currentState.extended; }
    uint8_t hat() const { return ~currentState.digital >> 4 & 0xF; }
  
    bool digitalPressed(const SnesDigital_Enum s) const { return (currentState.digital & s) != 0; }
    bool digitalChanged (const SnesDigital_Enum s) const { return ((lastState.digital ^ currentState.digital) & s) > 0; }
    bool digitalJustPressed (const SnesDigital_Enum s) const { return digitalChanged(s) & digitalPressed(s); }
    bool digitalJustReleased (const SnesDigital_Enum s) const { return digitalChanged(s) & !digitalPressed(s); }
    
    bool nttPressed(const SnesDigitalNTT_Enum s) const { return (currentState.extended & s) != 0; }
    bool nttChanged(const SnesDigitalNTT_Enum s) const { return ((lastState.extended ^ currentState.extended) & s) > 0; }
    bool nttJustPressed(const SnesDigitalNTT_Enum s) const { return nttChanged(s) & nttPressed(s); }
    bool nttJustReleased(const SnesDigitalNTT_Enum s) const { return nttChanged(s) & !nttPressed(s); }

    SnesDeviceType_Enum deviceType() const {
      if (currentState.id == 0x10) { // Power Pad uses special ID outside 4-bit range
        return SNES_DEVICE_NES_POWERPAD;
      } else if (currentState.id == 0x11) { // Zapper uses special ID outside 4-bit range
        return SNES_DEVICE_NES_ZAPPER;
      } else if (currentState.id == 0x0) {
        // SNES-family pads report ID 0. Some variants can wobble the extra-bit read
        // between 0xFFFF and 0xFFFE during valid button activity.
        return SNES_DEVICE_PAD;
      } else if (currentState.id == 0xF && ((currentState.digital >> 8) & 0xF) == 0xF) { //on NES controller ID is 0x0 and the non-existing buttons are pressed
        return SNES_DEVICE_NES;
      } else if (currentState.id == 0x2) {
        return SNES_DEVICE_NTT;
      } else if (currentState.id == 0x4) {
        return SNES_DEVICE_VB;
      } else if (currentState.id == 0xE || currentState.id == 0x8) { // Mouse: temporarily disabled
        return SNES_DEVICE_NOTSUPPORTED;
      } else {
        return SNES_DEVICE_NOTSUPPORTED;
      }
    }

    // Check if Power Pad button is pressed (buttons 1-12)
    bool powerPadPressed(uint8_t button) const {
      if (button < 1 || button > 12) return false;
      return (currentState.powerPadButtons & (1 << (button - 1))) != 0;
    }
};


// #ifdef SNES_ENABLE_MULTITAP //single port with multitap support
//   template <uint8_t PIN_CLOCK, uint8_t PIN_LATCH, uint8_t PIN_DATA1, uint8_t PIN_DATA2, uint8_t PIN_SELECT>
// #else
//   #ifdef SNES_MULTI_CONNECTION //multiple port without multitap support
//     #if SNES_MULTI_CONNECTION == 2
//       template <uint8_t PIN_CLOCK, uint8_t PIN_LATCH, uint8_t PIN_DATA1, uint8_t PIN_DATA2>
//     #elif SNES_MULTI_CONNECTION == 3
//       template <uint8_t PIN_CLOCK, uint8_t PIN_LATCH, uint8_t PIN_DATA1, uint8_t PIN_DATA2, uint8_t PIN_DATA3>
//     #elif SNES_MULTI_CONNECTION == 4
//       template <uint8_t PIN_CLOCK, uint8_t PIN_LATCH, uint8_t PIN_DATA1, uint8_t PIN_DATA2, uint8_t PIN_DATA3, uint8_t PIN_DATA4>
//     #else
//       #error "Invalid value for SNES_MULTI_CONNECTION"
//     #endif
//   #else //single port without multitap support
//     template <uint8_t PIN_CLOCK, uint8_t PIN_LATCH, uint8_t PIN_DATA1>
//   #endif
// #endif

class SnesPort {
  private:
    static constexpr uint8_t STROBE_HIGH_US = 6;
    static constexpr uint8_t STROBE_LOW_SETTLE_US = 2;
    static constexpr uint8_t CLOCK_LOW_US = 1;
    static constexpr uint8_t CLOCK_HIGH_US = 1;
    static constexpr uint8_t RUMBLE_STROBE_HIGH_US = 12;
    static constexpr uint8_t RUMBLE_STROBE_LOW_SETTLE_US = 6;
    static constexpr uint8_t RUMBLE_CLOCK_LOW_US = 6;
    static constexpr uint8_t RUMBLE_CLOCK_HIGH_US = 6;

    // DigitalPin<PIN_CLOCK> SNES_CLOCK;
    // DigitalPin<PIN_LATCH> SNES_LATCH;
    // DigitalPin<PIN_DATA1> SNES_DATA1;

    const uint8_t PIN_CLOCK;
    const uint8_t PIN_LATCH;
    const uint8_t PIN_DATA1;
    const uint8_t PIN_DATA2;
    const uint8_t PIN_SELECT;
    const bool enable_multitap;
    static constexpr uint8_t NO_CONTROLLER_CONFIRM_FRAMES = 3;
    uint8_t noControllerFrames[SNES_MAX_CTRL] = {};
    bool hadController[SNES_MAX_CTRL] = {};

    // #ifdef SNES_ENABLE_MULTITAP
    //   DigitalPin<PIN_DATA2> SNES_DATA2;
    //   DigitalPin<PIN_SELECT> SNES_MULTITAP;
    // #else
    //   #ifdef SNES_MULTI_CONNECTION
    //     #if SNES_MULTI_CONNECTION > 1
    //       DigitalPin<PIN_DATA2> SNES_DATA2;
    //     #endif
    //     #if SNES_MULTI_CONNECTION > 2
    //       DigitalPin<PIN_DATA3> SNES_DATA3;
    //     #endif
    //     #if SNES_MULTI_CONNECTION > 3
    //       DigitalPin<PIN_DATA4> SNES_DATA4;
    //     #endif
    //   #endif
    // #endif

    uint8_t joyCount = 0;
    uint8_t multitapPorts = 0;
    bool fourScoreDetected = false;  // NES Four Score adapter detected
    bool powerPadMode = false;       // NES Power Pad mode (uses D3/D4 for 12 buttons)
    bool powerPadAutoDetected = false; // True if Power Pad was auto-detected (not manually set)
    bool zapperMode = false;         // NES Zapper mode (trigger on D3, light sensor on D4)
    bool zapperAutoDetected = false; // True if Zapper was auto-detected
    uint8_t queuedRumbleData = 0;
    bool queuedRumbleValid = false;
    SnesController controllers[SNES_MAX_CTRL];

    inline void __attribute__((always_inline))
    writePin(uint8_t pin, bool value) {
#if defined(ARDUINO_ARCH_RP2040)
      gpio_put(pin, value);
#else
      digitalWrite(pin, value ? HIGH : LOW);
#endif
    }

    inline bool __attribute__((always_inline))
    readPin(uint8_t pin) const {
#if defined(ARDUINO_ARCH_RP2040)
      return gpio_get(pin);
#else
      return digitalRead(pin);
#endif
    }

    void transmitRumbleFrame(uint8_t rumbleData, bool prependStrobe, bool guardInterrupts = true) {
      if (PIN_SELECT == 255) return;
      pinMode(PIN_SELECT, OUTPUT);

      // Host USB OUT traffic can interrupt this bit-banged write path in
      // XInput modes. Keep the short RumbleTech command frame timing intact.
      if (guardInterrupts) {
        noInterrupts();
      }
      if (prependStrobe) {
        doRumbleStrobe();
      }

      uint16_t packet = (uint16_t(0x72) << 8) | rumbleData;
      for (int8_t i = 15; i >= 0; --i) {
        writePin(PIN_SELECT, (packet >> i) & 1);
        doRumbleClockCicle();
      }

      writePin(PIN_SELECT, LOW);
      if (guardInterrupts) {
        interrupts();
      }
    }

    inline void __attribute__((always_inline))
    doRumbleStrobe() {
      writePin(PIN_LATCH, HIGH);
      delayMicroseconds(RUMBLE_STROBE_HIGH_US);
      writePin(PIN_LATCH, LOW);
      delayMicroseconds(RUMBLE_STROBE_LOW_SETTLE_US);
    }

    inline void __attribute__((always_inline))
    doRumbleClockCicle() {
      writePin(PIN_CLOCK, LOW);
      delayMicroseconds(RUMBLE_CLOCK_LOW_US);
      writePin(PIN_CLOCK, HIGH);
      delayMicroseconds(RUMBLE_CLOCK_HIGH_US);
    }

    // #endif


    public:
    // #ifdef SNES_ENABLE_MULTITAP
    void detectMultiTap() {
      if (!enable_multitap)
        return;
      //Do the strobe to start reading button values
      writePin(PIN_LATCH, HIGH);//SNES_LATCH.write(HIGH);
      delayMicroseconds(4);

      if (readPin(PIN_DATA2) == 0x0)//SNES_DATA2
        multitapPorts = 4;

      writePin(PIN_LATCH, LOW);//SNES_LATCH.write(LOW);
      delayMicroseconds(6);
    }

    // Detect NES Four Score adapter by reading 24 bits and checking signature
    // Four Score uses both data lines: dat0 for P1+P3, dat1 for P2+P4
    // Signature at bits 16-23: 0x10 on dat0, 0x20 on dat1
    // Detection is "sticky" - once detected, stays until mode change or reset
    void detectFourScore() {
      if (!enable_multitap)  // Need dat1 pin for Four Score
        return;

      // If already detected, stay in Four Score mode (sticky)
      if (fourScoreDetected)
        return;

      // Strobe to start reading
      doStrobe();

      uint32_t data0 = 0;
      uint32_t data1 = 0;

      // Read 24 bits from both data lines
      for (uint8_t i = 0; i < 24; i++) {
        data0 |= (uint32_t)readPin(PIN_DATA1) << i;
        data1 |= (uint32_t)readPin(PIN_DATA2) << i;
        doClockCicle();
      }

      // Invert (active low)
      data0 = ~data0;
      data1 = ~data1;

      // Extract signature bytes (bits 16-23)
      uint8_t sig0 = (data0 >> 16) & 0xFF;
      uint8_t sig1 = (data1 >> 16) & 0xFF;

      // Check for Four Score signatures
      if (sig0 == NES_FOURSCORE_SIG_PORT1 && sig1 == NES_FOURSCORE_SIG_PORT2) {
        fourScoreDetected = true;
      }
    }

    // Enable/disable NES Power Pad mode
    // When enabled, reads 12 buttons from D3 (dat1) and D4 (sel) pins
    // manual=true means user explicitly enabled it (sticky), false means auto-detected
    void setPowerPadMode(bool enabled, bool manual = true) {
      powerPadMode = enabled;
      powerPadAutoDetected = enabled && !manual;
      if (enabled && PIN_SELECT != 255) {
        // Configure sel pin as input for Power Pad D4 data
        pinMode(PIN_SELECT, INPUT_PULLUP);
      }
    }

    bool isPowerPadMode() const { return powerPadMode; }
    bool isPowerPadAutoDetected() const { return powerPadAutoDetected; }

    // Enable/disable NES Zapper mode
    // When enabled, reads trigger from D3 and light sensor from D4
    void setZapperMode(bool enabled, bool manual = true) {
      zapperMode = enabled;
      zapperAutoDetected = enabled && !manual;
      if (enabled && PIN_SELECT != 255) {
        pinMode(PIN_SELECT, INPUT_PULLUP);
      }
    }

    bool isZapperMode() const { return zapperMode; }
    bool isZapperAutoDetected() const { return zapperAutoDetected; }

    bool __not_in_flash_func(fastPollStandardNes)() {
      if (powerPadMode || zapperMode || fourScoreDetected ||
          (enable_multitap && multitapPorts != 0)) {
        return false;
      }

      for (uint8_t i = 0; i < SNES_MAX_CTRL; i++) {
        getSnesController(i).copyCurrentToLast();
      }

      joyCount = 0;
      doStrobe();

      uint16_t raw = 0;
#if defined(ARDUINO_ARCH_RP2040)
      const uint32_t dataMask = 1u << PIN_DATA1;
      for (uint8_t i = 0; i < 16; i++) {
        raw |= ((gpio_get_all() & dataMask) ? 1u : 0u) << i;
        doClockCicle();
      }
#else
      for (uint8_t i = 0; i < 16; i++) {
        raw |= (uint16_t)readPin(PIN_DATA1) << i;
        doClockCicle();
      }
#endif

      SnesController& sc = getSnesController(joyCount++);
      sc.currentState.id = 0xF;
      sc.currentState.digital = ~raw;
      sc.currentState.extended = 0xFFFFu;
      sc.currentState.mouseX = 0;
      sc.currentState.mouseY = 0;
      sc.currentState.mouseButtons = 0;
      sc.currentState.mouseSensitivity = 0;
      sc.currentState.powerPadButtons = 0;
      sc.currentState.zapperTrigger = false;
      sc.currentState.zapperLightSense = false;
      noControllerFrames[0] = 0;
      hadController[0] = true;
      return true;
    }

    // Probe D3/D4 for Power Pad button activity
    // Returns: 0 = no activity, 1 = Power Pad button pressed
    // Only triggers on actual button press (LOW), not floating pins (HIGH with pullup)
    uint8_t probeD3D4Activity() {
      if (!enable_multitap || PIN_DATA2 == 255)
        return 0;

      // Temporarily configure D4 as input (it's normally OUTPUT for multitap select)
      if (PIN_SELECT != 255) {
        pinMode(PIN_SELECT, INPUT_PULLUP);
      }

      // Small delay for pullup to stabilize
      delayMicroseconds(10);

      // Read D3 and D4 raw values (8 clocks for D3, 4 for D4)
      uint8_t d3_raw = 0;
      uint8_t d4_raw = 0;

      doStrobe();

      for (uint8_t i = 0; i < 8; i++) {
        d3_raw |= (readPin(PIN_DATA2) << i);
        if (i < 4 && PIN_SELECT != 255) {
          d4_raw |= (readPin(PIN_SELECT) << i);
        }
        doClockCicle();
      }

      // Restore D4 as output for multitap (only if not in Power Pad mode)
      if (PIN_SELECT != 255 && !powerPadMode) {
        pinMode(PIN_SELECT, OUTPUT);
        writePin(PIN_SELECT, HIGH);
      }

      // Invert (active low) - buttons pressed = bits set after inversion
      d3_raw = ~d3_raw;
      d4_raw = ~d4_raw & 0x0F;

      // No button activity = no Power Pad detection
      // With pullups and nothing connected/no buttons pressed, d3_raw and d4_raw should be 0
      if (d3_raw == 0 && d4_raw == 0)
        return 0;

      // Button activity detected on D3/D4 = Power Pad
      return 1;
    }

    private:

    // Read NES Power Pad - 12 buttons from D3 and D4 data lines
    // D3 (dat1/PIN_DATA2): buttons 2, 1, 5, 9, 6, 10, 11, 7 (bits 0-7)
    // D4 (sel/PIN_SELECT): buttons 4, 3, 12, 8 (bits 0-3)
    // Returns 12-bit value with each bit = button number (bit 0 = button 1, etc.)
    void readPowerPad() {
      uint8_t d3_raw = 0;
      uint8_t d4_raw = 0;

      // Strobe already done in update()
      // Clock 8 times to read D3 and D4
      for (uint8_t i = 0; i < 8; i++) {
        d3_raw |= (readPin(PIN_DATA2) << i);
        if (i < 4) {
          d4_raw |= (readPin(PIN_SELECT) << i);
        }
        doClockCicle();
      }

      // Invert (active low)
      d3_raw = ~d3_raw;
      d4_raw = ~d4_raw & 0x0F;

      // Map raw bits to button numbers (1-12)
      // D3 clock order: buttons 2, 1, 5, 9, 6, 10, 11, 7
      // D4 clock order: buttons 4, 3, 12, 8
      uint16_t buttons = 0;

      // From D3 (bit index -> button number)
      if (d3_raw & 0x01) buttons |= (1 << 1);   // bit 0 -> button 2
      if (d3_raw & 0x02) buttons |= (1 << 0);   // bit 1 -> button 1
      if (d3_raw & 0x04) buttons |= (1 << 4);   // bit 2 -> button 5
      if (d3_raw & 0x08) buttons |= (1 << 8);   // bit 3 -> button 9
      if (d3_raw & 0x10) buttons |= (1 << 5);   // bit 4 -> button 6
      if (d3_raw & 0x20) buttons |= (1 << 9);   // bit 5 -> button 10
      if (d3_raw & 0x40) buttons |= (1 << 10);  // bit 6 -> button 11
      if (d3_raw & 0x80) buttons |= (1 << 6);   // bit 7 -> button 7

      // From D4 (bit index -> button number)
      if (d4_raw & 0x01) buttons |= (1 << 3);   // bit 0 -> button 4
      if (d4_raw & 0x02) buttons |= (1 << 2);   // bit 1 -> button 3
      if (d4_raw & 0x04) buttons |= (1 << 11);  // bit 2 -> button 12
      if (d4_raw & 0x08) buttons |= (1 << 7);   // bit 3 -> button 8

      // Set controller as Power Pad type (id = 0x10 for Power Pad)
      SnesController& sc = getSnesController(joyCount++);
      sc.currentState.id = 0x10;  // Special Power Pad ID
      sc.currentState.powerPadButtons = buttons;
      sc.currentState.digital = 0;
      sc.currentState.extended = 0xFFFF;
    }

    // Read NES Zapper - trigger on D3, light sensor on D4
    // D3 (dat1/PIN_DATA2): Trigger button (active low)
    // D4 (sel/PIN_SELECT): Light sensor (active low when detecting light)
    void readZapper() {
      // Read first bit from D3 (trigger) and D4 (light sensor)
      bool trigger = !readPin(PIN_DATA2);  // Active low
      bool lightSense = (PIN_SELECT != 255) ? !readPin(PIN_SELECT) : false;

      // Clock once to complete the read cycle
      doClockCicle();

      // Set controller as Zapper type (id = 0x11 for Zapper)
      SnesController& sc = getSnesController(joyCount++);
      sc.currentState.id = 0x11;  // Special Zapper ID
      sc.currentState.zapperTrigger = trigger;
      sc.currentState.zapperLightSense = lightSense;
      sc.currentState.digital = 0;
      sc.currentState.extended = 0xFFFF;
      sc.currentState.powerPadButtons = 0;
    }


    inline void __attribute__((always_inline))
    doStrobe() {
      writePin(PIN_LATCH, HIGH);//SNES_LATCH.write(HIGH);
      delayMicroseconds(STROBE_HIGH_US);
      writePin(PIN_LATCH, LOW);//SNES_LATCH.write(LOW);
      delayMicroseconds(STROBE_LOW_SETTLE_US);
    }


    inline void __attribute__((always_inline))
    doClockCicle() {
      writePin(PIN_CLOCK, LOW);//SNES_CLOCK.write(LOW);
      delayMicroseconds(CLOCK_LOW_US);
      writePin(PIN_CLOCK, HIGH);//SNES_CLOCK.write(HIGH);
      delayMicroseconds(CLOCK_HIGH_US);
    }

    inline uint8_t __attribute__((always_inline))
    getExtendCount(const uint8_t id1) const {
      if (id1 == 0xD) //NTT PAD
        return 16;
      else if (id1 == 0x1 || id1 == 0x7) //Mouse: need 16 more bits for movement data (raw 0x1->inverted 0xE, raw 0x7->inverted 0x8)
        return 16;
      else
        return 1;
    }

    inline void __attribute__((always_inline))
    setControllerValues(uint8_t id, uint16_t data, uint16_t extended) {
      //note: lib always read at least 1 bit into Extended
      id = ~id & 0xF;
      data = ~data;
      extended = ~extended;
      uint8_t slot = joyCount;
      bool noControllerSignature = (id == 0x0 && extended == 0xFFFE && data == 0x0000);
      if (noControllerSignature) {
        // Debounce disconnects: single noisy SNES frames can mimic the unplug signature.
        if (slot < SNES_MAX_CTRL && hadController[slot] && noControllerFrames[slot] < NO_CONTROLLER_CONFIRM_FRAMES) {
          noControllerFrames[slot]++;
          joyCount++;
        }
        return;
      }

      if (slot < SNES_MAX_CTRL) {
        noControllerFrames[slot] = 0;
        hadController[slot] = true;
      }

      SnesController& sc = getSnesController(joyCount++);
      if (id == 0xF && ((data >> 8) & 0xF) == 0xF) { //nes
        sc.currentState.id = id;
        sc.currentState.extended = extended;
      } else if (id == 0xE || id == 0x8) { // Mouse: ID 0xE (common) or 0x8 (some variants)
        sc.currentState.id = id;
        sc.currentState.extended = extended;

        // Debug: store raw extended for display (extern defined in Input_Snes.h)
        extern uint16_t snes_debug_mouse_ext;
        snes_debug_mouse_ext = extended;

        // Parse mouse buttons from data byte 2 (bits 8-9)
        // After inversion: bit 8 = right, bit 9 = left - swap them
        uint8_t rawButtons = (data >> 8) & 0x03;
        sc.currentState.mouseButtons = ((rawButtons & 1) << 1) | ((rawButtons >> 1) & 1);

        // Sensitivity from data bits 10-11 (after inversion, need to re-invert)
        sc.currentState.mouseSensitivity = (~data >> 10) & 0x03;

        // Parse movement from extended data
        // extended is already inverted from line 407, use directly (don't double-invert)
        // Y: bits 0-6 = magnitude, bit 7 = direction
        // X: bits 8-14 = magnitude, bit 15 = direction
        uint8_t yRaw = extended & 0xFF;
        uint8_t xRaw = (extended >> 8) & 0xFF;

        // Sign-magnitude format: bit 7 = direction, bits 0-6 = magnitude
        // Direction: 1 = up/left (negative), 0 = down/right (positive)
        int8_t yMag = yRaw & 0x7F;
        int8_t xMag = xRaw & 0x7F;
        bool yDir = (yRaw >> 7) & 1; // 1 = up
        bool xDir = (xRaw >> 7) & 1; // 1 = left

        sc.currentState.mouseY = yDir ? -yMag : yMag;
        sc.currentState.mouseX = xDir ? -xMag : xMag;
      } else {
        if ((id & 0x4) == 0x4) { //vboy
          sc.currentState.id = 0x4; // set VB id with its constant bit after copying variable bits
          sc.currentState.extended = id & 0x3; //Copy A/B bits to extended.
        } else { //snes, ntt, other
          sc.currentState.id = id;
          sc.currentState.extended = extended;
        }
      }
      sc.currentState.digital = data; //common data
    }

#ifdef SNES_MULTI_CONNECTION
    void __not_in_flash_func(readSingleController)(bool interleaveRumble = false) {
      unsigned int fromController1 = 0x0;
      unsigned int fromControllerExtended1 = 0x0;

      #if SNES_MULTI_CONNECTION > 1
        unsigned int fromController2 = 0x0;
        unsigned int fromControllerExtended2 = 0x0;
      #endif
      #if SNES_MULTI_CONNECTION > 2
        unsigned int fromController3 = 0x0;
        unsigned int fromControllerExtended3 = 0x0;
      #endif
      #if SNES_MULTI_CONNECTION > 3
        unsigned int fromController4 = 0x0;
        unsigned int fromControllerExtended4 = 0x0;
      #endif
      
      uint8_t i;
      //uint8_t joyIndex = 0;
      
      //Do the strobe to start reading button values
      //doStrobe();

      const bool guardRumbleTiming = interleaveRumble && PIN_SELECT != 255;
      if (guardRumbleTiming) {
        noInterrupts();
      }
      uint16_t rumblePacket = (uint16_t(0x72) << 8) | queuedRumbleData;
      for (i = 0; i < 16; i++) {
        if (guardRumbleTiming) {
          writePin(PIN_SELECT, (rumblePacket & 0x8000) ? HIGH : LOW);
          rumblePacket <<= 1;
        }

        //read the value, shift it and store it as a bit on fromController:
        fromController1 |= readPin(PIN_DATA1) << i;//SNES_DATA1

        #if SNES_MULTI_CONNECTION > 1
          fromController2 |= SNES_DATA2 << i;
        #endif
        #if SNES_MULTI_CONNECTION > 2
          fromController3 |= SNES_DATA3 << i;
        #endif
        #if SNES_MULTI_CONNECTION > 3
          fromController4 |= SNES_DATA4 << i;
        #endif

        //More one cycle on the clock pin...
        doClockCicle();
      }

      if (guardRumbleTiming) {
        writePin(PIN_SELECT, LOW);
        interrupts();
      }

      const uint8_t id1 = (fromController1 >> 12);
      uint8_t extend = getExtendCount(id1);

      // Debug: store extend count
      extern uint8_t snes_debug_extend_count;
      snes_debug_extend_count = extend;

      /*#if SNES_MULTI_CONNECTION == 2
        const uint8_t id2 = (fromController2 >> 12);
        extend = max(extend, getExtendCount(id2));
      #endif*/
      #if SNES_MULTI_CONNECTION > 1
        const uint8_t id2 = (fromController2 >> 12);
        extend = max(extend, getExtendCount(id2));
      #endif
      #if SNES_MULTI_CONNECTION > 2
        const uint8_t id3 = (fromController3 >> 12);
        extend = max(extend, getExtendCount(id3));
      #endif
      #if SNES_MULTI_CONNECTION > 3
        const uint8_t id4 = (fromController4 >> 12);
        extend = max(extend, getExtendCount(id4));
      #endif
      
      for (i = 0; i < extend; i++) {
        //read the value, shift it and store it as a bit on fromController:
        fromControllerExtended1 |= readPin(PIN_DATA1) << i;//SNES_DATA1

        #if SNES_MULTI_CONNECTION > 1
          fromControllerExtended2 |= SNES_DATA2 << i;
        #endif
        #if SNES_MULTI_CONNECTION > 2
          fromControllerExtended3 |= SNES_DATA3 << i;
        #endif
        #if SNES_MULTI_CONNECTION > 3
          fromControllerExtended4 |= SNES_DATA4 << i;
        #endif

        //More one cycle on the clock pin...
        doClockCicle();
      }

      setControllerValues(id1, fromController1, fromControllerExtended1);

      #if SNES_MULTI_CONNECTION > 1
        setControllerValues(id2, fromController2, fromControllerExtended2);
      #endif
      #if SNES_MULTI_CONNECTION > 2
        setControllerValues(id3, fromController3, fromControllerExtended3);
      #endif
      #if SNES_MULTI_CONNECTION > 3
        setControllerValues(id4, fromController4, fromControllerExtended4);
      #endif
    }
#else
    void __not_in_flash_func(readSingleController)(bool interleaveRumble = false) {
      unsigned int fromController = 0x0;
      unsigned int fromControllerExtended = 0x0;
      uint8_t i;
      //uint8_t joyIndex = 0;
      
      //Do the strobe to start reading button values
      //doStrobe();

      const bool guardRumbleTiming = interleaveRumble && PIN_SELECT != 255;
      if (guardRumbleTiming) {
        noInterrupts();
      }
      uint16_t rumblePacket = (uint16_t(0x72) << 8) | queuedRumbleData;
      for (i = 0; i < 16; i++) {
        if (guardRumbleTiming) {
          writePin(PIN_SELECT, (rumblePacket & 0x8000) ? HIGH : LOW);
          rumblePacket <<= 1;
        }

        //read the value, shift it and store it as a bit on fromController:
        fromController |= readPin(PIN_DATA1) << i;//SNES_DATA1

        //More one cycle on the clock pin...
        doClockCicle();
      }
      if (guardRumbleTiming) {
        writePin(PIN_SELECT, LOW);
        interrupts();
      }

      const uint8_t id1 = (fromController >> 12);
      const uint8_t extend = getExtendCount(id1);
      
      for (i = 0; i < extend; i++) {
        //read the value, shift it and store it as a bit on fromController:
        fromControllerExtended |= readPin(PIN_DATA1) << i;//SNES_DATA1

        //More one cycle on the clock pin...
        doClockCicle();
      }

      setControllerValues(id1, fromController, fromControllerExtended);
    }
#endif

    // Read NES Four Score - 4 controllers from 2 data lines
    // dat0: P1 (bits 0-7), P3 (bits 8-15), signature (bits 16-23)
    // dat1: P2 (bits 0-7), P4 (bits 8-15), signature (bits 16-23)
    void readNESFourScore() {
      uint32_t data0 = 0;
      uint32_t data1 = 0;

      // Read 24 bits from both data lines (already strobed in update())
      for (uint8_t i = 0; i < 24; i++) {
        data0 |= (uint32_t)readPin(PIN_DATA1) << i;
        data1 |= (uint32_t)readPin(PIN_DATA2) << i;
        doClockCicle();
      }

      // Invert (active low)
      data0 = ~data0;
      data1 = ~data1;

      // Extract controller data (8 bits each)
      uint8_t p1_data = data0 & 0xFF;         // P1: dat0 bits 0-7
      uint8_t p2_data = data1 & 0xFF;         // P2: dat1 bits 0-7
      uint8_t p3_data = (data0 >> 8) & 0xFF;  // P3: dat0 bits 8-15
      uint8_t p4_data = (data1 >> 8) & 0xFF;  // P4: dat1 bits 8-15

      // Set up all 4 controllers as NES Four Score type
      // NES button order: A, B, Select, Start, Up, Down, Left, Right (bits 0-7)
      // Map to SNES-style bit positions for consistency
      auto mapNEStoSNES = [](uint8_t nes) -> uint16_t {
        uint16_t snes = 0;
        // NES bit 0 = A -> SNES Y (NES A maps to SNES Y position)
        // NES bit 1 = B -> SNES B
        // NES bit 2 = Select -> SNES Select
        // NES bit 3 = Start -> SNES Start
        // NES bit 4 = Up -> SNES Up
        // NES bit 5 = Down -> SNES Down
        // NES bit 6 = Left -> SNES Left
        // NES bit 7 = Right -> SNES Right
        if (nes & 0x01) snes |= SNES_Y;       // A -> Y
        if (nes & 0x02) snes |= SNES_B;       // B -> B
        if (nes & 0x04) snes |= SNES_SELECT;  // Select
        if (nes & 0x08) snes |= SNES_START;   // Start
        if (nes & 0x10) snes |= SNES_UP;      // Up
        if (nes & 0x20) snes |= SNES_DOWN;    // Down
        if (nes & 0x40) snes |= SNES_LEFT;    // Left
        if (nes & 0x80) snes |= SNES_RIGHT;   // Right
        return snes;
      };

      // Player 1
      SnesController& c1 = getSnesController(joyCount++);
      c1.currentState.id = 0xF;  // NES ID
      c1.currentState.digital = mapNEStoSNES(p1_data) | 0x0F00;  // Set upper nibble like NES
      c1.currentState.extended = 0xFFFF;

      // Player 2
      SnesController& c2 = getSnesController(joyCount++);
      c2.currentState.id = 0xF;
      c2.currentState.digital = mapNEStoSNES(p2_data) | 0x0F00;
      c2.currentState.extended = 0xFFFF;

      // Player 3
      SnesController& c3 = getSnesController(joyCount++);
      c3.currentState.id = 0xF;
      c3.currentState.digital = mapNEStoSNES(p3_data) | 0x0F00;
      c3.currentState.extended = 0xFFFF;

      // Player 4
      SnesController& c4 = getSnesController(joyCount++);
      c4.currentState.id = 0xF;
      c4.currentState.digital = mapNEStoSNES(p4_data) | 0x0F00;
      c4.currentState.extended = 0xFFFF;
    }

//    #ifdef SNES_ENABLE_MULTITAP
    void readMultitap() {
      if (!enable_multitap)
        return;
      uint8_t i;

      for (uint8_t multitappair = 0; multitappair < 2; multitappair++) {
        unsigned int fromController1 = 0x00;
        unsigned int fromControllerExtended1 = 0x00;
        unsigned int fromController2 = 0x00;
        unsigned int fromControllerExtended2 = 0x00;

        // Select the controller pair BEFORE strobe
        if(multitappair == 0) //first pair of controllers (1 & 2)
          writePin(PIN_SELECT, HIGH);
        else //second pair of controllers (3 & 4)
          writePin(PIN_SELECT, LOW);
        delayMicroseconds(6);

        // Now do strobe for this pair
        doStrobe();

        for (i = 0; i < 16; i++) {
          fromController1 |= readPin(PIN_DATA1) << i;
          fromController2 |= readPin(PIN_DATA2) << i;
          doClockCicle();
        }

        uint8_t id1 = (fromController1 >> 12);
        uint8_t id2 = (fromController2 >> 12);
        uint8_t extend1 = getExtendCount(id1);
        uint8_t extend2 = getExtendCount(id2);

        for (i = 0; i < max(extend1, extend2); i++) {
          fromControllerExtended1 |= readPin(PIN_DATA1) << i;
          fromControllerExtended2 |= readPin(PIN_DATA2) << i;
          doClockCicle();
        }

        setControllerValues(id1, fromController1, fromControllerExtended1);
        setControllerValues(id2, fromController2, fromControllerExtended2);
      }

      //leave it as default high
      writePin(PIN_SELECT, HIGH);
    }
//    #endif

    public:
  //uint8_t PIN_CLOCK, uint8_t PIN_LATCH, uint8_t PIN_DATA1, uint8_t PIN_DATA2, uint8_t PIN_SELECT
    SnesPort(uint8_t _PIN_CLOCK, uint8_t _PIN_LATCH, uint8_t _PIN_DATA1, uint8_t _PIN_DATA2, uint8_t _PIN_SELECT)
      : PIN_CLOCK(_PIN_CLOCK), PIN_LATCH(_PIN_LATCH), PIN_DATA1(_PIN_DATA1), PIN_DATA2(_PIN_DATA2), PIN_SELECT(_PIN_SELECT),
        enable_multitap(_PIN_DATA2 != 255 && _PIN_SELECT != 255) {
    }

    void begin() {
      multitapPorts = 0;
      fourScoreDetected = false;
      powerPadMode = false;
      powerPadAutoDetected = false;
      zapperMode = false;
      zapperAutoDetected = false;
      queuedRumbleData = 0;
      queuedRumbleValid = false;
      memset(noControllerFrames, 0, sizeof(noControllerFrames));
      memset(hadController, 0, sizeof(hadController));

      //init pins
      pinMode(PIN_LATCH, OUTPUT);writePin(PIN_LATCH, LOW);//SNES_LATCH.config(OUTPUT, LOW);
      pinMode(PIN_CLOCK, OUTPUT);writePin(PIN_CLOCK, HIGH);//SNES_CLOCK.config(OUTPUT, HIGH);
      pinMode(PIN_DATA1, INPUT_PULLUP);//SNES_DATA1.config(INPUT, HIGH);
      if (PIN_SELECT != 255) {
        // Keep pin 6 (IOBit / select) in a known idle state even with no multitap.
        pinMode(PIN_SELECT, OUTPUT);
        writePin(PIN_SELECT, LOW);
      }
      
      if (enable_multitap) {
        pinMode(PIN_DATA2, INPUT_PULLUP);//SNES_DATA2.config(INPUT, HIGH);
        delay(8);
        detectMultiTap();
      }

      #ifdef SNES_ENABLE_MULTITAP
        SNES_MULTITAP.config(OUTPUT, HIGH);
        SNES_DATA2.config(INPUT, HIGH);
        detectMultiTap();
      #else
        #if SNES_MULTI_CONNECTION > 1
          SNES_DATA2.config(INPUT, HIGH);
        #endif
        #if SNES_MULTI_CONNECTION > 2
          SNES_DATA3.config(INPUT, HIGH);
        #endif
        #if SNES_MULTI_CONNECTION > 3
          SNES_DATA4.config(INPUT, HIGH);
        #endif
      #endif
    }

    void __not_in_flash_func(update)() {
      //keep last data
      for (uint8_t i = 0; i < SNES_MAX_CTRL; i++) {
        getSnesController(i).copyCurrentToLast();
      }

      uint8_t lastJoyCount = joyCount;
      joyCount = 0;
      //multitapPorts = 0;
      const bool queuedRumbleForSinglePad =
        queuedRumbleValid && PIN_SELECT != 255 &&
        !(powerPadMode || zapperMode || fourScoreDetected ||
          (enable_multitap && multitapPorts != 0));

      // NES Zapper mode (manual or auto-detected)
      if (zapperMode) {
        doStrobe();
        readZapper();
      }
      // NES Power Pad takes priority when enabled (manual or auto-detected)
      // Once detected, Power Pad mode is "sticky" until mode change or reset
      else if (powerPadMode) {
        doStrobe();
        readPowerPad();
      }
      // NES Four Score takes priority when detected
      else if (fourScoreDetected) {
        doStrobe();
        readNESFourScore();
      } else if (enable_multitap) { //      #ifdef SNES_ENABLE_MULTITAP
        if (multitapPorts == 0) {
          doStrobe();
          readSingleController();
        } else {
          // readMultitap() does its own strobe per controller pair
          readMultitap();
        }

        // Auto-detect Power Pad if no controller found on D0 and D3/D4 has activity
        if (joyCount == 0 && enable_multitap && PIN_DATA2 != 255) {
          uint8_t activity = probeD3D4Activity();
          if (activity == 1) {
            // Power Pad detected - auto-enable
            setPowerPadMode(true, false);  // false = auto-detected
          }
        }
    } else { //      #else
        doStrobe();
        readSingleController();
    } //      #endif

      //reset disconnected device status
      if (lastJoyCount > joyCount) {
        for (uint8_t i = joyCount; i < lastJoyCount; i++) {
          //debugln(F("Resetting disconnected device"));
          if (i < SNES_MAX_CTRL) {
            noControllerFrames[i] = 0;
            hadController[i] = false;
          }
          getSnesController(i).reset(true, false);
        }
      }

      // RumbleTech command frames are sent immediately after the normal SNES
      // read. Interleaving command bits into the 16 read clocks can corrupt
      // tested pads and still fail to latch the motor command.
      if (queuedRumbleForSinglePad) {
        sendQueuedRumbleDuringPoll(false);
      } else {
        queuedRumbleValid = false;
      }

    }

    // Store rumble byte to be sent at the end of the next single-pad poll.
    void queueRumble(uint8_t rumbleData) {
      queuedRumbleData = rumbleData;
      queuedRumbleValid = true;
    }

    // Send SNES Rumble data (Doom FX3 / RumbleTech protocol)
    // Packet is 16 bits MSB-first:
    // [15:8] = sentry 0x72, [7:4] = right motor/light, [3:0] = left motor/heavy
    // Sequence: clear shift register with latch pulse, then output each bit and pulse clock.
    void sendRumble(uint8_t rumbleData) {
      transmitRumbleFrame(rumbleData, true);
    }

    void sendQueuedRumbleDuringPoll(bool interruptsAlreadyGuarded = false) {
      if (!queuedRumbleValid || PIN_SELECT == 255) return;
      if ((enable_multitap && multitapPorts != 0) || powerPadMode || zapperMode || fourScoreDetected) {
        queuedRumbleValid = false;
        return;
      }

      transmitRumbleFrame(queuedRumbleData, false, !interruptsAlreadyGuarded);
      queuedRumbleValid = false;
    }

    SnesController& getSnesController(const uint8_t i) { return controllers[min(i, uint8_t(SNES_MAX_CTRL - 1))]; }

    uint8_t getMultitapPorts() const { return multitapPorts; }
    uint8_t getControllerCount() const { return joyCount; }
    bool isMultitapEnabled() const { return enable_multitap; }
    bool isFourScoreDetected() const { return fourScoreDetected; }
    void setFourScoreMode(bool enabled) { fourScoreDetected = enabled; }
    uint8_t getDataPin2() const { return PIN_DATA2; }
    uint8_t getSelectPin() const { return PIN_SELECT; }
};

#endif
