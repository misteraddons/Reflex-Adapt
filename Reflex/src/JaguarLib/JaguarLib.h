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

#include "../DigitalIO/DigitalIO.h"

#ifndef JAGLIB_H_
#define JAGLIB_H_

//Max of 4 controllers per port (with a multitap)
#define JAG_MAX_CTRL 1 //4

#define TAP_JAG_PORTS 4

//Debounce is optional.
//#define JAG_DEBOUNCE 8 //milliseconds

enum JagDeviceType_Enum {
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
  uint8_t hat() const { return currentState.digital & 0xF; } //todo fix
  
  bool digitalPressed(const JagDigital_Enum s) const { return (currentState.digital & s) == 0; }
  bool digitalChanged (const JagDigital_Enum s) const { return ((lastState.digital ^ currentState.digital) & s) > 0; }
  bool digitalJustPressed (const JagDigital_Enum s) const { return digitalChanged(s) & digitalPressed(s); }
  bool digitalJustReleased (const JagDigital_Enum s) const { return digitalChanged(s) & !digitalPressed(s); }


  JagDeviceType_Enum deviceType() const {
    return currentState.id;
  }

};

template <uint8_t PIN_J3_J4, uint8_t PIN_J2_J5, uint8_t PIN_J1_J6, uint8_t PIN_J0_J7, uint8_t PIN_B0_B2, uint8_t PIN_B1_B3, uint8_t PIN_J11_J15, uint8_t PIN_J10_J14, uint8_t PIN_J9_J13, uint8_t PIN_J8_J12>
class JagPort {
  private:
    //4 bi-directional pins
    DigitalPin<PIN_J3_J4> jag_J3_J4;
    DigitalPin<PIN_J2_J5> jag_J2_J5;
    DigitalPin<PIN_J1_J6> jag_J1_J6;
    DigitalPin<PIN_J0_J7> jag_J0_J7;
    //6 input pins
    DigitalPin<PIN_B0_B2> jag_B0_B2;
    DigitalPin<PIN_B1_B3> jag_B1_B3;
    DigitalPin<PIN_J11_J15> jag_J11_J15;
    DigitalPin<PIN_J10_J14> jag_J10_J14;
    DigitalPin<PIN_J9_J13> jag_J9_J13;
    DigitalPin<PIN_J8_J12> jag_J8_J12;

    uint8_t joyCount = 0;
    uint8_t multitapPorts = 0;
    JagController controllers[JAG_MAX_CTRL];

    inline uint8_t __attribute__((always_inline))
    readNibble() const { return (jag_B0_B2 << 5) + (jag_B1_B3 << 4) + (jag_J8_J12 << 3) + (jag_J9_J13 << 2) + (jag_J10_J14 << 1) + jag_J11_J15; }

    inline void __attribute__((always_inline))
    setNoRow() {
      jag_J0_J7.write(HIGH);
      jag_J1_J6.write(HIGH);
      jag_J2_J5.write(HIGH);
      jag_J3_J4.write(HIGH);
    }


    inline void __attribute__((always_inline))
    setRow(uint8_t row) {
      setNoRow();
      delayMicroseconds(12);

      switch (row) {
        //pad 1
        case 0: jag_J0_J7.write(LOW); break;
        case 1: jag_J1_J6.write(LOW); break;
        case 2: jag_J2_J5.write(LOW); break;
        case 3: jag_J3_J4.write(LOW); break;

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
      delayMicroseconds(12);
    }

    void readJagPort() {

      //Multitap?
      //setRow(13);
      //if(readNibble() & B00100000 == 0)

      uint8_t nibbles[] = { 0x3F, 0x3F, 0x3F, 0x3F }; //B00111111

      for(uint8_t i = 0; i < 4; ++i) {
        setRow(i);
        nibbles[i] = readNibble();
      }

      //Leave it in default state
      setNoRow();

      const uint8_t joyIndex = joyCount++;
      JagController& sc = getJagController(joyIndex);

      //C2 and C3. Default value is not connected or standard pad
      if(((nibbles[2] & B00100000) == B00100000) && ((nibbles[3] & B00100000) == B00100000))
        sc.currentState.id = JAG_DEVICE_PAD;
      else 
        sc.currentState.id = JAG_DEVICE_NOTSUPPORTED;
        
      for(uint8_t i = 0; i < 4; ++i) {
        setControlValues(sc, i, nibbles[i]);
      }

      //debugln(sc.currentState.digital,BIN);
    }

    void setControlValues(JagController& sc, const uint8_t page, const uint32_t nibbleTMP) const {
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
    void begin(){
      //init output pins
      jag_J3_J4.config(OUTPUT, HIGH);
      jag_J2_J5.config(OUTPUT, HIGH);
      jag_J1_J6.config(OUTPUT, HIGH);
      jag_J0_J7.config(OUTPUT, HIGH);
    
      //init input pins with pull-up
      jag_B0_B2.config(INPUT, HIGH);
      jag_B1_B3.config(INPUT, HIGH);
      jag_J11_J15.config(INPUT, HIGH);
      jag_J10_J14.config(INPUT, HIGH);
      jag_J9_J13.config(INPUT, HIGH);
      jag_J8_J12.config(INPUT, HIGH);

      multitapPorts = 0;
      //reset all devices to default values
      for (uint8_t i = 0; i < JAG_MAX_CTRL; ++i) {
        getJagController(i).reset(true, true);
      }
    }
    
    void update(){
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

      //if device changed without connect/disconnect. eg: 3d pad analog <> digital
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

    JagController& getJagController(const uint8_t i) { return controllers[min(i, JAG_MAX_CTRL)]; }

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
