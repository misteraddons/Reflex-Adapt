/*******************************************************************************
 * Jaguar controller input library.
 * https://github.com/sonik-br/JaguarLib
 * 
 * The library depends on greiman's DigitalIO library.
 * https://github.com/greiman/DigitalIO
 * 
 * I recommend the usage of SukkoPera's fork of DigitalIO as it supports a few more platforms.
 * https://github.com/SukkoPera/DigitalIO
 * 
 * 
 * It works with jaguar controllers (3 buttons) only.
 * Multitap support might be added in the future if I can get one to test.
 * 
 * Buttons on my jaguar pad bounces a lot when pressing...
 * So I added a debounce feature. Can be enabled or disabled.
 * 
 * Based on documentation from darthcloud's BlueRetro
 * https://github.com/darthcloud/BlueRetro/wiki/Jaguar-interface
 * 
*/

//#include "../DigitalIO/DigitalIO.h"

#ifndef JAGLIB_H_
#define JAGLIB_H_

#if defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
#include "hardware/gpio.h"
#endif

//Max of 4 controllers per port (with a multitap)
#define JAG_MAX_CTRL 1 //4

#define TAP_JAG_PORTS 4

//Debounce is optional.
//#define JAG_DEBOUNCE 8 //milliseconds

enum JagDeviceType_Enum : uint8_t {
  JAG_DEVICE_NONE = 0,
  JAG_DEVICE_NOTSUPPORTED,
  JAG_DEVICE_PAD
};

//enum __attribute__ ((__packed__))
enum JagDigital_Enum {
  JAG_PAD_RIGHT = 0x00001,
  JAG_PAD_LEFT  = 0x00002,
  JAG_PAD_DOWN  = 0x00004,
  JAG_PAD_UP    = 0x00008,
  JAG_A         = 0x00010,
  JAG_PAUSE     = 0x00020,
  JAG_1         = 0x00040,
  JAG_4         = 0x00080,
  JAG_7         = 0x00100,
  JAG_STAR      = 0x00200,
  JAG_B         = 0x00400,
  //JAG_C1      = 0x00800,
  JAG_2         = 0x01000,
  JAG_5         = 0x02000,
  JAG_8         = 0x04000,
  JAG_0         = 0x08000,
  JAG_C         = 0x10000,
  //JAG_C2      = 0x20000,
  JAG_3         = 0x40000,
  JAG_6         = 0x80000,
  JAG_9         = 0x100000,
  JAG_HASH      = 0x200000,
  JAG_OPTION    = 0x400000,
  //JAG_C3      = 0x800000
};

struct JagControllerState {
  JagDeviceType_Enum id = JAG_DEVICE_NONE;
  uint32_t digital = 0xFFFFFFFF; //Dpad and buttons
  bool operator!=(const JagControllerState& b) const {
    return id != b.id ||
         digital != b.digital;
  }
};

class JagController {
  public:
        
    JagControllerState currentState;
    JagControllerState lastState;

    void reset(const bool resetId = false, bool resetPrevious = false) {
      if (resetId)
        currentState.id = JAG_DEVICE_NONE;

      currentState.digital = 0xFFFFFFFF;

      if (resetPrevious) {
        copyCurrentToLast();
      }
    }

    void copyCurrentToLast() {
      lastState.id = currentState.id;
      lastState.digital = currentState.digital;
    }


  bool deviceJustChanged() const { return currentState.id != lastState.id; }
  bool stateChanged() const { return currentState != lastState; }
  uint32_t digitalRaw() const { return currentState.digital; }
  uint8_t hat() const { return currentState.digital & 0xF; }
  
  bool digitalPressed(const JagDigital_Enum s) const { return (currentState.digital & s) == 0; }
  bool digitalChanged (const JagDigital_Enum s) const { return ((lastState.digital ^ currentState.digital) & s) > 0; }
  bool digitalJustPressed (const JagDigital_Enum s) const { return digitalChanged(s) & digitalPressed(s); }
  bool digitalJustReleased (const JagDigital_Enum s) const { return digitalChanged(s) & !digitalPressed(s); }


  JagDeviceType_Enum deviceType() const {
    return currentState.id;
  }

};

//template <uint8_t PIN_J3_J4, uint8_t PIN_J2_J5, uint8_t PIN_J1_J6, uint8_t PIN_J0_J7, uint8_t PIN_B0_B2, uint8_t PIN_B1_B3, uint8_t PIN_J11_J15, uint8_t PIN_J10_J14, uint8_t PIN_J9_J13, uint8_t PIN_J8_J12>
class JagPort {
  private:
    static constexpr uint8_t JAG_ROW_COUNT = 4;
    static constexpr uint8_t JAG_IDLE_NIBBLE = 0x3F;
    static constexpr uint8_t JAG_C2C3_MASK = 0x20;
    static constexpr uint8_t JAG_DISCONNECT_GRACE_FRAMES = 2;
    //4 output pins pins (row selection)
    const uint8_t jag_J3_J4;//DigitalPin<PIN_J3_J4> jag_J3_J4;
    const uint8_t jag_J2_J5;//DigitalPin<PIN_J2_J5> jag_J2_J5;
    const uint8_t jag_J1_J6;//DigitalPin<PIN_J1_J6> jag_J1_J6;
    const uint8_t jag_J0_J7;//DigitalPin<PIN_J0_J7> jag_J0_J7;
    //6 input pins (column data)
    const uint8_t jag_B0_B2;//DigitalPin<PIN_B0_B2> jag_B0_B2;
    const uint8_t jag_B1_B3;//DigitalPin<PIN_B1_B3> jag_B1_B3;
    const uint8_t jag_J11_J15;//DigitalPin<PIN_J11_J15> jag_J11_J15;
    const uint8_t jag_J10_J14;//DigitalPin<PIN_J10_J14> jag_J10_J14;
    const uint8_t jag_J9_J13;//DigitalPin<PIN_J9_J13> jag_J9_J13;
    const uint8_t jag_J8_J12;//DigitalPin<PIN_J8_J12> jag_J8_J12;
    const uint32_t jag_row_mask;

    uint8_t joyCount = 0;
    uint8_t multitapPorts = 0;
    uint8_t lastRow = 0xFF;
    uint8_t signatureMisses = 0;
    JagController controllers[JAG_MAX_CTRL];

    inline void __attribute__((always_inline))
    writePin(const uint8_t pin, const bool value) const {
#if defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
      gpio_put(pin, value ? 1 : 0);
#else
      digitalWrite(pin, value ? HIGH : LOW);
#endif
    }

    inline uint8_t __attribute__((always_inline))
    readPin(const uint8_t pin) const {
#if defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
      return gpio_get(pin) ? 1 : 0;
#else
      return digitalRead(pin) ? 1 : 0;
#endif
    }

    inline uint32_t __attribute__((always_inline))
    rowMaskForIndex(const uint8_t row) const {
      switch (row) {
        case 0: return 1u << jag_J0_J7;
        case 1: return 1u << jag_J1_J6;
        case 2: return 1u << jag_J2_J5;
        case 3: return 1u << jag_J3_J4;
        default: return 0;
      }
    }

    inline uint8_t __attribute__((always_inline))
    readNibble() const {
#if defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
      const uint32_t pins = gpio_get_all();
      return (((pins >> jag_B0_B2) & 1u) << 5) |
             (((pins >> jag_B1_B3) & 1u) << 4) |
             (((pins >> jag_J8_J12) & 1u) << 3) |
             (((pins >> jag_J9_J13) & 1u) << 2) |
             (((pins >> jag_J10_J14) & 1u) << 1) |
             ((pins >> jag_J11_J15) & 1u);
#else
      return (readPin(jag_B0_B2) << 5) + (readPin(jag_B1_B3) << 4) + (readPin(jag_J8_J12) << 3) + (readPin(jag_J9_J13) << 2) + (readPin(jag_J10_J14) << 1) + readPin(jag_J11_J15);
#endif
    }

    inline void __attribute__((always_inline))
    setNoRow() {
#if defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
      gpio_put_masked(jag_row_mask, jag_row_mask);
#else
      writePin(jag_J3_J4, HIGH);
      writePin(jag_J2_J5, HIGH);
      writePin(jag_J1_J6, HIGH);
      writePin(jag_J0_J7, HIGH);
#endif
      lastRow = 0xFF;
    }


    inline void __attribute__((always_inline))
    setRow(const uint8_t row) {
      //Clear last used row. Need to change how this works if/when using multitap.
#if !(defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED))
      if (lastRow < JAG_ROW_COUNT) {
        switch (lastRow) {
          case 0: writePin(jag_J0_J7, HIGH); break;
          case 1: writePin(jag_J1_J6, HIGH); break;
          case 2: writePin(jag_J2_J5, HIGH); break;
          case 3: writePin(jag_J3_J4, HIGH); break;
        }
      }
#endif

      if (row >= JAG_ROW_COUNT) {
        lastRow = 0xFF;
        return;
      }

#if defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
      gpio_put_masked(jag_row_mask, jag_row_mask & ~rowMaskForIndex(row));
#else
      switch (row) {
        //pad 1
        case 0: writePin(jag_J0_J7, LOW); break;
        case 1: writePin(jag_J1_J6, LOW); break;
        case 2: writePin(jag_J2_J5, LOW); break;
        case 3: writePin(jag_J3_J4, LOW); break;

        /* multitap
        //pad2
        case 4:
          jag_J0_J7.write(LOW);
          jag_J1_J6.write(LOW);
          jag_J2_J5.write(LOW);
          jag_J3_J4.write(LOW);
          break;
        case 5:
          //jag_J0_J7.write(LOW);
          jag_J1_J6.write(LOW);
          jag_J2_J5.write(LOW);
          jag_J3_J4.write(LOW);
        break;
        
        case 6:
          jag_J0_J7.write(LOW);
          //jag_J1_J6.write(LOW);
          jag_J2_J5.write(LOW);
          jag_J3_J4.write(LOW);
          break;
        case 7:
          //jag_J0_J7.write(LOW);
          //jag_J1_J6.write(LOW);
          jag_J2_J5.write(LOW);
          jag_J3_J4.write(LOW);
          break;
        
        //pad3
        case 8:
          jag_J0_J7.write(LOW);
          jag_J1_J6.write(LOW);
          //jag_J2_J5.write(LOW);
          jag_J3_J4.write(LOW);
          break;
        case 9:
          //jag_J0_J7.write(LOW);
          jag_J1_J6.write(LOW);
          //jag_J2_J5.write(LOW);
          jag_J3_J4.write(LOW);
          break;
        case 10:
          jag_J0_J7.write(LOW);
          //jag_J1_J6.write(LOW);
          //jag_J2_J5.write(LOW);
          jag_J3_J4.write(LOW);
          break;
        case 11:
          jag_J0_J7.write(LOW);
          jag_J1_J6.write(LOW);
          jag_J2_J5.write(LOW);
          //jag_J3_J4.write(LOW);
          break;
        
        //pad4
        case 12:
          //jag_J0_J7.write(LOW);
          jag_J1_J6.write(LOW);
          jag_J2_J5.write(LOW);
          //jag_J3_J4.write(LOW);
          break;
        case 13:
          jag_J0_J7.write(LOW);
          //jag_J1_J6.write(LOW);
          jag_J2_J5.write(LOW);
          //jag_J3_J4.write(LOW);
          break;
        case 14:
          jag_J0_J7.write(LOW);
          jag_J1_J6.write(LOW);
          //jag_J2_J5.write(LOW);
          //jag_J3_J4.write(LOW);
          break;
        case 15:
          //jag_J0_J7.write(LOW);
          //jag_J1_J6.write(LOW);
          //jag_J2_J5.write(LOW);
          //jag_J3_J4.write(LOW);
          break;
        */
      }
#endif
      delayMicroseconds(8); //Seems to work with 4us but let's be safe
      lastRow = row;
    }

    inline void __not_in_flash_func(readMatrix)(uint8_t* nibbles) {
      setNoRow();
      for (uint8_t row = 0; row < JAG_ROW_COUNT; ++row) {
        setRow(row);
        nibbles[row] = readNibble();
      }
      setNoRow();
    }

    inline bool __not_in_flash_func(hasJaguarPadSignature)(const uint8_t* nibbles) const {
      return ((nibbles[2] & JAG_C2C3_MASK) == JAG_C2C3_MASK) &&
             ((nibbles[3] & JAG_C2C3_MASK) == JAG_C2C3_MASK);
    }

    inline bool __not_in_flash_func(hasNonIdleActivity)(const uint8_t* nibbles) const {
      for (uint8_t row = 0; row < JAG_ROW_COUNT; ++row) {
        if (nibbles[row] != JAG_IDLE_NIBBLE)
          return true;
      }
      return false;
    }

    inline bool __not_in_flash_func(hasRowVariant)(const uint8_t* nibbles) const {
      return !(nibbles[0] == nibbles[1] && nibbles[0] == nibbles[2] && nibbles[0] == nibbles[3]);
    }

    void __not_in_flash_func(readJagPort)() {
      uint8_t nibbles_a[JAG_ROW_COUNT] = { JAG_IDLE_NIBBLE, JAG_IDLE_NIBBLE, JAG_IDLE_NIBBLE, JAG_IDLE_NIBBLE };
      uint8_t nibbles_b[JAG_ROW_COUNT] = { JAG_IDLE_NIBBLE, JAG_IDLE_NIBBLE, JAG_IDLE_NIBBLE, JAG_IDLE_NIBBLE };
      JagController& lastController = getJagController(0);
      readMatrix(nibbles_a);

      const bool sig_a = hasJaguarPadSignature(nibbles_a);
      const bool non_idle_a = hasNonIdleActivity(nibbles_a);
      const bool row_variant_a = hasRowVariant(nibbles_a);
      const bool knownPad = lastController.currentState.id == JAG_DEVICE_PAD;

      const uint8_t* nibbles = nibbles_a;
      bool useNeutralState = false;

      if (!knownPad || !sig_a) {
        delayMicroseconds(8);
        readMatrix(nibbles_b);

        const bool sig_b = hasJaguarPadSignature(nibbles_b);
        const bool non_idle_b = hasNonIdleActivity(nibbles_b);
        const bool row_variant_b = hasRowVariant(nibbles_b);

        uint8_t diff_bits = 0;
        for (uint8_t row = 0; row < JAG_ROW_COUNT; ++row) {
          diff_bits += __builtin_popcount((unsigned)(nibbles_a[row] ^ nibbles_b[row]));
        }
        const bool stable = diff_bits <= 2;
        const bool any_activity = non_idle_a || non_idle_b;
        const bool row_activity = ((row_variant_a && non_idle_a) || (row_variant_b && non_idle_b));
        const bool weak_activity = row_activity && (stable || knownPad);

        // Jaguar runtime scanning is more forgiving than the initial autodetect
        // pass: once we're already in Jaguar mode, a stable row-varying read is
        // enough to treat the port as present, and a brief signature miss should
        // not instantly drop player 2.
        if (!(sig_a || sig_b)) {
          if (weak_activity || any_activity) {
            nibbles = non_idle_b ? nibbles_b : nibbles_a;
            signatureMisses = 0;
          } else if (knownPad && signatureMisses < JAG_DISCONNECT_GRACE_FRAMES) {
            ++signatureMisses;
            useNeutralState = true;
          } else {
            signatureMisses = 0;
            return;
          }
        } else {
          signatureMisses = 0;
          nibbles = sig_b ? nibbles_b : nibbles_a;
        }
      } else {
        signatureMisses = 0;
      }

      const uint8_t joyIndex = joyCount++;
      JagController& sc = getJagController(joyIndex);
      sc.currentState.id = JAG_DEVICE_PAD;

      if (useNeutralState) {
        sc.currentState.digital = 0xFFFFFFFF;
        return;
      }

      for(uint8_t i = 0; i < JAG_ROW_COUNT; ++i) {
        setControlValues(sc, i, nibbles[i]);
      }

      //debugln(sc.currentState.digital,BIN);
    }

    void __not_in_flash_func(setControlValues)(JagController& sc, const uint8_t page, const uint32_t nibbleTMP) const {
      if (page == 0) { //Pause A UDLR
        sc.currentState.digital = (sc.currentState.digital & 0xFFFFFFC0) + nibbleTMP;
      } else if (page == 1) { //C1 B*741
        sc.currentState.digital = (sc.currentState.digital & 0xFFFFF03F) + (nibbleTMP << 6);
      } else if (page == 2) { //C2 C0852
        sc.currentState.digital = (sc.currentState.digital & 0xFFFC0FFF) + (nibbleTMP << 12);
      } else if (page == 3) { //C3 Option #963
        sc.currentState.digital = (sc.currentState.digital & 0xFF03FFFF) + (nibbleTMP << 18);
      }
    }

  public:
//template <uint8_t PIN_J3_J4, uint8_t PIN_J2_J5, uint8_t PIN_J1_J6, uint8_t PIN_J0_J7, uint8_t PIN_B0_B2, uint8_t PIN_B1_B3, uint8_t PIN_J11_J15, uint8_t PIN_J10_J14, uint8_t PIN_J9_J13, uint8_t PIN_J8_J12>

    JagPort(uint8_t PIN_J3_J4, uint8_t PIN_J2_J5, uint8_t PIN_J1_J6, uint8_t PIN_J0_J7, uint8_t PIN_B0_B2, uint8_t PIN_B1_B3, uint8_t PIN_J11_J15, uint8_t PIN_J10_J14, uint8_t PIN_J9_J13, uint8_t PIN_J8_J12)
      : jag_J3_J4(PIN_J3_J4), jag_J2_J5(PIN_J2_J5), jag_J1_J6(PIN_J1_J6), jag_J0_J7(PIN_J0_J7), jag_B0_B2(PIN_B0_B2), jag_B1_B3(PIN_B1_B3), jag_J11_J15(PIN_J11_J15), jag_J10_J14(PIN_J10_J14), jag_J9_J13(PIN_J9_J13), jag_J8_J12(PIN_J8_J12),
        jag_row_mask((1u << PIN_J3_J4) | (1u << PIN_J2_J5) | (1u << PIN_J1_J6) | (1u << PIN_J0_J7)) {
    }

    void begin(){
      //init output pins
      pinMode(jag_J3_J4, OUTPUT); digitalWrite(jag_J3_J4, HIGH);//jag_J3_J4.config(OUTPUT, HIGH);
      pinMode(jag_J2_J5, OUTPUT); digitalWrite(jag_J2_J5, HIGH);//jag_J2_J5.config(OUTPUT, HIGH);
      pinMode(jag_J1_J6, OUTPUT); digitalWrite(jag_J1_J6, HIGH);//jag_J1_J6.config(OUTPUT, HIGH);
      pinMode(jag_J0_J7, OUTPUT); digitalWrite(jag_J0_J7, HIGH);//jag_J0_J7.config(OUTPUT, HIGH);
    
      //init input pins with pull-up
      pinMode(jag_B0_B2, INPUT_PULLUP);//jag_B0_B2.config(INPUT, HIGH);
      pinMode(jag_B1_B3, INPUT_PULLUP);//jag_B1_B3.config(INPUT, HIGH);
      pinMode(jag_J11_J15, INPUT_PULLUP);//jag_J11_J15.config(INPUT, HIGH);
      pinMode(jag_J10_J14, INPUT_PULLUP);//jag_J10_J14.config(INPUT, HIGH);
      pinMode(jag_J9_J13, INPUT_PULLUP);//jag_J9_J13.config(INPUT, HIGH);
      pinMode(jag_J8_J12, INPUT_PULLUP);//jag_J8_J12.config(INPUT, HIGH);

      multitapPorts = 0;
      //reset all devices to default values
      for (uint8_t i = 0; i < JAG_MAX_CTRL; ++i) {
        getJagController(i).reset(true, true);
      }
      setNoRow();
    }
    
    void __not_in_flash_func(update)(){
      //keep last data
      for (uint8_t i = 0; i < JAG_MAX_CTRL; ++i) {
        getJagController(i).copyCurrentToLast();
      }
      
      uint8_t lastJoyCount = joyCount;
      joyCount = 0;
      multitapPorts = 0;
      
      //resetAll();
      
      readJagPort();
      
      #ifdef JAG_DEBOUNCE
        debounce();
      #endif

      //if device changed without connect/disconnect.
      for (uint8_t i = 0; i < joyCount; ++i) {
        JagController& sc = getJagController(i);
        if (sc.currentState.id != sc.lastState.id) {
          //debugln(F("Resetting changed device"));
          sc.reset(false, false);
        }
      }

      //reset disconnected device status
      if (lastJoyCount > joyCount) {
        for (uint8_t i = joyCount; i < lastJoyCount; ++i) {
          //debugln(F("Resetting disconnected device"));
          getJagController(i).reset(true, false);
        }
      }
      
    }

    //Call only on arduino power-on to check for multitap connected
    void detectMultitap() {
    }

    JagController& getJagController(const uint8_t i) { return controllers[min<uint8_t>(i, JAG_MAX_CTRL - 1)]; }

    uint8_t getMultitapPorts() const { return multitapPorts; }
    uint8_t getControllerCount() const { return joyCount; }

    #ifdef JAG_DEBOUNCE
    void debounce() {
      static uint32_t debouncedAt[23];

      JagController& sc = getJagController(0);

      if(!sc.stateChanged())
        return;

      const uint32_t updateMillis = millis();

      for (uint8_t i = 0; i < 23; ++i) {
        const JagDigital_Enum e = (JagDigital_Enum)(1UL << i);
        if(sc.digitalChanged(e)) {
          if((updateMillis - debouncedAt[i]) > JAG_DEBOUNCE) { //keep value, update debouncedAt
            debouncedAt[i] = updateMillis;
          } else { //restore debounced value
            bitWrite(sc.currentState.digital, i, bitRead(sc.lastState.digital, i)); //sc.digitalJustPressed(e)
          }
        }
      }
    }
    #endif //JAG_DEBOUNCE
};

#endif
