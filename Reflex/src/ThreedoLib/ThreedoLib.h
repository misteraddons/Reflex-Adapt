/*******************************************************************************
 * 3DO controller input library.
 * https://github.com/sonik-br/ThreedoLib
 * 
 * The library depends on greiman's DigitalIO library.
 * https://github.com/greiman/DigitalIO
 * 
 * I recommend the usage of SukkoPera's fork of DigitalIO as it supports a few more platforms.
 * https://github.com/SukkoPera/DigitalIO
 * 
 * 
 * Works with 3DO digital pad.
 * Supports multiple daisy chained devices. (tested with up to two)
 * 
 * Device protocol based on:
 * BlueRetro project
 * https://hackaday.io/project/170365-blueretro/log/190948-3do-interface
 * 
 * Also from
 * http://kaele.com/~kashima/games/3do.html
*/


#include "../DigitalIO/DigitalIO.h"


#ifndef THREEDOLIB_H_
#define THREEDOLIB_H_

#ifndef THREEDO_MAX_CTRL
  #define THREEDO_MAX_CTRL 8 // Max is 8?
#endif

enum ThreedoDeviceType_Enum : uint8_t {
  THREEDO_DEVICE_NONE = 0,
  THREEDO_DEVICE_NOTSUPPORTED,
  THREEDO_DEVICE_PAD
  
  //THREEDO_DEVICE_MOUSE,
  //THREEDO_DEVICE_FLIGHTSTICK,
  //THREEDO_DEVICE_LIGHTGUN,
  //THREEDO_DEVICE_ORBATAK
};

enum ThreedoDigital_Enum {
  THREEDO_DOWN  = (1 << 3),
  THREEDO_UP    = (1 << 4),
  THREEDO_RIGHT = (1 << 5),
  THREEDO_LEFT  = (1 << 6),
  THREEDO_A     = (1 << 7),
  THREEDO_B     = (1 << 8),
  THREEDO_C     = (1 << 9),
  THREEDO_P     = (1 << 10),
  THREEDO_X     = (1 << 11),
  THREEDO_R     = (1 << 12),
  THREEDO_L     = (1 << 13)
};


struct ThreedoControllerState {
  ThreedoDeviceType_Enum id = THREEDO_DEVICE_NONE;
  uint16_t digital = 0x0; //Dpad and buttons. Id data is zeroed in this field

  bool operator!=(const ThreedoControllerState& b) const {
    return id != b.id || digital != b.digital;
  }
};


class ThreedoController {
  public:
  
    ThreedoControllerState currentState;
    ThreedoControllerState lastState;

    void reset(const bool resetId = false, bool resetPrevious = false) {
      if (resetId)
        currentState.id = THREEDO_DEVICE_NONE;

      currentState.digital = 0x0;

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
    uint8_t hat() const { return (uint8_t)currentState.digital >> 3 & 0xFF; }
    bool digitalPressed (const ThreedoDigital_Enum s) const { return (currentState.digital & s) != 0; }
    bool digitalChanged (const ThreedoDigital_Enum s) const { return ((lastState.digital ^ currentState.digital) & s) > 0; }
    bool digitalJustPressed (const ThreedoDigital_Enum s) const { return digitalChanged(s) & digitalPressed(s); }
    bool digitalJustReleased (const ThreedoDigital_Enum s) const { return digitalChanged(s) & !digitalPressed(s); }

    ThreedoDeviceType_Enum deviceType() const { return currentState.id; }
};


template <uint8_t PIN_CLOCK, uint8_t PIN_DOUT, uint8_t PIN_DIN>
class ThreedoPort {
  private:
    DigitalPin<PIN_CLOCK> TDO_CLOCK;
    DigitalPin<PIN_DOUT> TDO_DOUT;
    DigitalPin<PIN_DIN> TDO_DIN;
    uint8_t joyCount = 0;
    ThreedoController controllers[THREEDO_MAX_CTRL];

    //inline void __attribute__((always_inline))
    void doClockCycle() {
      //Hold HI/LOW for 4us each
      //Data can be read at falling edge
      TDO_CLOCK.write(HIGH);
      delayMicroseconds(4);
      TDO_CLOCK.write(LOW);
      delayMicroseconds(4);
    }

    void setControllerValues(const uint8_t i, const ThreedoDeviceType_Enum deviceType, uint16_t data) {
      ThreedoController& sc = get3doController(i);

      if(deviceType == THREEDO_DEVICE_PAD)
        data &= 0x7FFF; //Clear id bits
      else
        data = 0x0;

      sc.currentState.id = deviceType;
      sc.currentState.digital = data;
    }

    void readControllers() {
      //Hold clock high for 500us before start reading
      //External loop must handle this... Let the device rest and not query it
      //TDO_CLOCK.write(HIGH);
      //delayMicroseconds(500);
      //TDO_CLOCK.write(LOW);
      //delayMicroseconds(500);

      TDO_CLOCK.write(LOW);
      delayMicroseconds(4);

      //hold DOUT low while reading
      TDO_DOUT.write(LOW);

      //Try to read devices up to THREEDO_MAX_CTRL
      for(uint8_t padIndex = 0; padIndex < THREEDO_MAX_CTRL; ++padIndex) {

        ThreedoDeviceType_Enum deviceType = THREEDO_DEVICE_NONE;

        uint16_t fromController = 0x0;

        //Read 16 bits of data
        for(uint8_t i = 0; i < 16; ++i) {
          const uint8_t dataIn = TDO_DIN;
          
          //Shift the received bit and set
          if (dataIn)
            fromController |= (0x1 << i);

          //Do a clock cycle
          doClockCycle();
        }

        //Check device id
        if (fromController == 0xFFFF) { //no controller connected, exit
          //deviceType = THREEDO_DEVICE_NONE;
          break;
        } else if ( (fromController & 0xC007) == 0x1 ) { //ditigal pad
          deviceType = THREEDO_DEVICE_PAD;
        } else {
          //uint8_t needToRead = 0;
          //Other device types might need to read more data
          /*
          if ( (uint8_t)fromController == 0x4D ) { //LIGHTGUN
            //Needs more 2 bytes
            needToRead = 2*8;
            //deviceType = THREEDO_DEVICE_LIGHTGUN;
            deviceType = THREEDO_DEVICE_NOTSUPPORTED;
          } else if ( (uint8_t)fromController == 0x92 ) { //MOUSE
            //Needs more 2 bytes
            needToRead = 2*8;
            //deviceType = THREEDO_DEVICE_MOUSE;
            deviceType = THREEDO_DEVICE_NOTSUPPORTED;
          } else if ( (fromController & 0xFFFF) == 0x11 ) { //ORBATAK
            //Needs more 2 bytes and check the data
            //(EXTRADATA & 0xFF00) == 0x0 )
            //deviceType = THREEDO_DEVICE_ORBATAK;
            needToRead = 2*8;
            deviceType = THREEDO_DEVICE_NOTSUPPORTED;
          } else if ( (fromController & 0xFFFF) == 0xDE8 ) { //FLIGHTSTICK
            //Needs more 7 bytes and check the data
            //( (EXTRADATA & 0xF ) == 0x1 )
            needToRead = 7*8;
            //deviceType = THREEDO_DEVICE_FLIGHTSTICK;
            deviceType = THREEDO_DEVICE_NOTSUPPORTED;
          } else {
            //No known id detected. read garbage? discard data
            deviceType = THREEDO_DEVICE_NONE;
          }
          */

          //no known id detected. read garbage? discard data
          deviceType = THREEDO_DEVICE_NONE;
    
          //Read and discard additional data for unsupported devices
          /*
          for(uint8_t i = 0; i < needToRead; ++i) {
            //Do a clock cycle
            doClockCycle();
          }
          */
        }
        
        if (deviceType == THREEDO_DEVICE_NONE)
          break;

        //If reached here, a device is connected and is valid
        ++joyCount;
      
        setControllerValues(padIndex, deviceType, fromController);
      }//end for pads
    
      //Device reading ended
      //Put pins in default state
      TDO_DOUT.write(HIGH);
      delayMicroseconds(4); //need?
      TDO_CLOCK.write(HIGH);
      delayMicroseconds(4); //need?
    }
    
  public:
    void begin() {
      
      //Init pins
      TDO_CLOCK.config(OUTPUT, HIGH);
      TDO_DOUT.config(OUTPUT, HIGH);
      TDO_DIN.config(INPUT, HIGH);
      delayMicroseconds(100);
    }

    void update() {
      //keep last data
      for (uint8_t i = 0; i < THREEDO_MAX_CTRL; ++i) {
        get3doController(i).copyCurrentToLast();
      }

      const uint8_t lastJoyCount = joyCount;
      joyCount = 0;

      readControllers();

      //Reset disconnected device status
      if (lastJoyCount > joyCount) {
        for (uint8_t i = joyCount; i < lastJoyCount; ++i) {
          //debugln(F("Resetting disconnected device"));
          get3doController(i).reset(true, false);
        }
      }
    }

    ThreedoController& get3doController(const uint8_t i) { return controllers[min(i, THREEDO_MAX_CTRL)]; }
    uint8_t getControllerCount() const { return joyCount; }
};

#endif


/* Device id reference
 *  https://github.com/libretro/opera-libretro/blob/068c69ff784f2abaea69cdf1b8d3d9d39ac4826e/libopera/opera_pbus.c
 *  
 *  100----- ------00                   DIGITALPAD (total 2 bytes)
 *  00------ -----001                   inverse
 *  
 *  01001101 -------- -------- -------- LIGHTGUN (total 4 bytes)
 *  10110010                            inverse
 *  
 *  01001001 -------- -------- -------- MOUSE (total 4 bytes)
 *  10010010                            inverse
 *  
 *  11000000 00000000 -------- 00000000 ORBATAK (total 4+4 bytes. starts with mouse info)
 *  00000000 -------- 00000000 00000011 inverse
 *  
 *  00010111 10110000 1000---- -------- FLIGHTSTICK (total 9 bytes?)
 *  ----0001 00001101 11101000          inverse
*/
