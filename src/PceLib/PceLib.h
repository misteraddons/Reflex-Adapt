/*******************************************************************************
 * PCEngine controller input library.
 * https://github.com/sonik-br/PceLib
 * 
 * The library depends on greiman's DigitalIO library.
 * https://github.com/greiman/DigitalIO
 * 
 * I recommend the usage of SukkoPera's fork of DigitalIO as it supports a few more platforms.
 * https://github.com/SukkoPera/DigitalIO
 * 
 * 
 * It works with pce controllers (2 or 6 buttons).
 * Support for multitap is planned.
 * 
 * Based on documentation from David Shadoff
 * https://github.com/dshadoff/PCE_Controller_Info
 * 
*/

#include "../DigitalIO/DigitalIO.h"

#ifndef PCELIB_H_
#define PCELIB_H_

//Max of 5 controllers per port (with a multitap)
#define PCE_MAX_CTRL 5

#define TAP_PCE_PORTS 5

enum PceDeviceType_Enum {
  PCE_DEVICE_NONE = 0,
  PCE_DEVICE_NOTSUPPORTED,
  PCE_DEVICE_PAD2,
  PCE_DEVICE_PAD6
};

enum PceDigital_Enum {
  PCE_PAD_UP    = 0x0001,
  PCE_PAD_RIGHT = 0x0002,
  PCE_PAD_DOWN  = 0x0004,
  PCE_PAD_LEFT  = 0x0008,
  PCE_1         = 0x0010,
  PCE_2         = 0x0020,
  PCE_SELECT    = 0x0040,
  PCE_RUN       = 0x0080,
  PCE_3         = 0x0100,
  PCE_4         = 0x0200,
  PCE_5         = 0x0400,
  PCE_6         = 0x0800
};

struct PceControllerState {
  PceDeviceType_Enum id = PCE_DEVICE_NONE;
  uint16_t digital = 0xFFFF; //Dpad and buttons
  bool operator!=(const PceControllerState& b) const {
    return id != b.id ||
         digital != b.digital;
  }
};

class PceController {
  public:
        
    PceControllerState currentState;
    PceControllerState lastState;

    void reset(const bool resetId = false, bool resetPrevious = false) {
      if (resetId)
        currentState.id = PCE_DEVICE_NONE;

      currentState.digital = 0xFFFF;

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
  uint16_t digitalRaw() const { return currentState.digital; }
  uint8_t hat() const { return currentState.digital & 0xF; }
  
  bool digitalPressed(const PceDigital_Enum s) const { return (currentState.digital & s) == 0; }
  bool digitalChanged (const PceDigital_Enum s) const { return ((lastState.digital ^ currentState.digital) & s) > 0; }
  bool digitalJustPressed (const PceDigital_Enum s) const { return digitalChanged(s) & digitalPressed(s); }
  bool digitalJustReleased (const PceDigital_Enum s) const { return digitalChanged(s) & !digitalPressed(s); }


  PceDeviceType_Enum deviceType() const {
    return currentState.id;
  }

};

template <uint8_t PIN_SEL, uint8_t PIN_CLR, uint8_t PIN_D0, uint8_t PIN_D1, uint8_t PIN_D2, uint8_t PIN_D3>
class PcePort {
  private:
    DigitalPin<PIN_SEL> pce_SEL;
    DigitalPin<PIN_CLR> pce_CLR;
    DigitalPin<PIN_D0> pce_D0; //U 1 3
    DigitalPin<PIN_D1> pce_D1; //R 2 4
    DigitalPin<PIN_D2> pce_D2; //D S 5
    DigitalPin<PIN_D3> pce_D3; //L R 6

    uint8_t joyCount = 0;
    uint8_t multitapPorts = 0;
    PceController controllers[PCE_MAX_CTRL];

    inline uint8_t __attribute__((always_inline))
    readNibble() const { return (pce_D3 << 3) + (pce_D2 << 2) + (pce_D1 << 1) + pce_D0; }

    void readPcePort() {
      uint8_t nibble_0;
      uint8_t nibble_1;

      //A 2-button controller doesn't care about the CLR line,
      //but due to the way it is connected to the 74HC157,
      //all 4 outputs will be LOW when CLR is HIGH.


      //detect controller
      pce_SEL.write(HIGH);
      pce_CLR.write(HIGH);
      delayMicroseconds(2);
      nibble_0 = readNibble();
      //detect controller
      
      if(nibble_0 == B0) { //controller is connected
        const uint8_t joyIndex = joyCount++;
        PceController& pc = getPceController(joyIndex);
        
        //6btn pad requires double reading.
        const uint8_t scanTimes = (pc.currentState.id == PCE_DEVICE_NONE || pc.currentState.id == PCE_DEVICE_PAD6) ? 2 : 1;
        bool is6btn = false;

        //if (pc.currentState.id == PCE_DEVICE_NONE)
        //  pc.currentState.id = PCE_DEVICE_PAD2;//set it as 2btn here. 6btn will be checked below

        for(uint8_t scan = 0; scan < scanTimes; scan++) {
           if(scan == 1) {
            pce_SEL.write(HIGH);
            pce_CLR.write(HIGH);
            delayMicroseconds(2);
          }

          pce_CLR.write(LOW);
          delayMicroseconds(2);
          
          nibble_0 = readNibble(); //DPAD
  
          pce_SEL.write(LOW);
          delayMicroseconds(2);
          nibble_1 = readNibble(); //BTN
  
          if(nibble_0 == B0) { //6btn pad
            is6btn = true;
            setControlValues(pc, 2, nibble_1); // 6543
          } else { //2btn pad
            setControlValues(pc, 0, nibble_0); // LDRU
            setControlValues(pc, 1, nibble_1); // RS21
          }
        } //end for scan

        //if reached here and is not a 6btn pad, assume it as a 2btn pad
        pc.currentState.id = is6btn ? PCE_DEVICE_PAD6 : PCE_DEVICE_PAD2;

      } //end if controller detected
      
      pce_SEL.write(LOW);
      pce_CLR.write(LOW);
      delayMicroseconds(10);
    }

    void setControlValues(PceController& pc, const uint8_t page, const uint8_t nibbleTMP) const {
      if (page == 0) //HAT LDRU
        pc.currentState.digital = (pc.currentState.digital & 0xFFF0) + nibbleTMP;
      else if (page == 1) //RS21
        pc.currentState.digital = (pc.currentState.digital & 0xFF0F) + (nibbleTMP << 4);
      else if (page == 2) //6543
        pc.currentState.digital = (pc.currentState.digital & 0xF0FF) + (nibbleTMP << 8);
    }

  public:
    void begin(){
      //init output pins
      pce_SEL.config(OUTPUT, LOW);
      pce_CLR.config(OUTPUT, LOW);
    
      //init input pins with pull-up
      pce_D0.config(INPUT, HIGH);
      pce_D1.config(INPUT, HIGH);
      pce_D2.config(INPUT, HIGH);
      pce_D3.config(INPUT, HIGH);

      multitapPorts = 0;
      //reset all devices to default values
      for (uint8_t i = 0; i < PCE_MAX_CTRL; i++) {
        getPceController(i).reset(true, true);
      }
    }
    
    void update(){
      //keep last data
      for (uint8_t i = 0; i < PCE_MAX_CTRL; i++) {
        getPceController(i).copyCurrentToLast();
      }
      
      uint8_t lastJoyCount = joyCount;
      joyCount = 0;
      multitapPorts = 0;
      
      //resetAll();
      
      readPcePort();

      //if device changed without connect/disconnect. eg: 2btn <> 6btn
      for (uint8_t i = 0; i < joyCount; i++) {
        PceController& pc = getPceController(i);
        if (pc.currentState.id != pc.lastState.id) {
          //debugln(F("Resetting changed device"));
          pc.reset(false, false);
        }
      }

      //reset disconnected device status
      if (lastJoyCount > joyCount) {
        for (uint8_t i = joyCount; i < lastJoyCount; i++) {
          //debugln(F("Resetting disconnected device"));
          getPceController(i).reset(true, false);
        }
      }
      
    }

    //Call only on arduino power-on to check for multitap connected
    void detectMultitap() {
    }

    PceController& getPceController(const uint8_t i) { return controllers[min(i, PCE_MAX_CTRL)]; }

    uint8_t getMultitapPorts() const { return multitapPorts; }
    uint8_t getControllerCount() const { return joyCount; }
};

#endif
