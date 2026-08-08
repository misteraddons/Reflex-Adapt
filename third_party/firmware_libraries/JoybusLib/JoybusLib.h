/*******************************************************************************
 * Nintendo Joybus controller input library.
 * https://github.com/sonik-br/JoybusLib
 * 
 * The library is a wrapper over Dorigian's Joybus-PIO
 * https://github.com/Doridian/Joybus-PIO
 * 
 * Also used for reference
 * https://hitmen.c02.at/files/yagcd/yagcd/chap9.html
 * https://github.com/joeldipops/TransferBoy/blob/master/docs/TransferPakReference.md
 * https://github.com/DragonMinded/libdragon
 * 
*/

#ifndef JOYBUSLIB_H_
#define JOYBUSLIB_H_

#include "joybus_pio.hpp"
#include "joybus_generic.hpp"
#include "joybus_gamecube.hpp"
#include "joybus_n64.hpp"
#include "../../../firmware/platform/latency_trace_gpio.h"

#if defined(ENABLE_INPUT_GBA) && !defined(JOYBUS_DISABLE_GBA_LINK)
  #define JOYBUS_ENABLE_GBA_LINK 1
#else
  #define JOYBUS_ENABLE_GBA_LINK 0
#endif

#if JOYBUS_ENABLE_GBA_LINK
  #include "joybus_gba.hpp"
  #include "data_inputrom.hpp" // GBA rom to report buttons. To be used with a GBA-GC link cable
#endif



//testing
//0000000000000100  0x0004  GBA
//0000010100000000  0x0500  N64 controller
//0000000000000001  0x0001  N64 mic
//0000000000000010  0x0002  N64 keyboard
//0000001000000000  0x0200  N64 mouse
//0000100100000000  0x0900  GC controller
//0000100000100000  0x0820  GC keyboard
//0000100000000000  0x0800  GC wheel
//1110100101000000  0xE940  GCW invalid origin
//1110100101100000  0xE960  GCW valid origin


enum JoybusDeviceType_Enum {
  JOYBUS_DEVICE_NONE = 0,
  JOYBUS_DEVICE_GBA,
  JOYBUS_DEVICE_N64PAD,
  JOYBUS_DEVICE_N64MOUSE,
  JOYBUS_DEVICE_GCPAD,
  JOYBUS_DEVICE_GCWIRELESS,
  JOYBUS_DEVICE_GCWHEEL,
  JOYBUS_DEVICE_NOTSUPPORTED
//  JOYBUS_DEVICE_N64MIC,
//  JOYBUS_DEVICE_N64KEYBOARD,
//  JOYBUS_DEVICE_GCKEYBOARD,
};

enum JoybusAnalog_Enum {
  JOYBUS_ANALOG_X = 0,
  JOYBUS_ANALOG_Y,
  JOYBUS_ANALOG_CX,
  JOYBUS_ANALOG_CY,
  JOYBUS_ANALOG_L,
  JOYBUS_ANALOG_R,
  JOYBUS_ANALOG_WHEEL,
  JOYBUS_ANALOG_GAS,
  JOYBUS_ANALOG_BRAKE
};

typedef enum GBAButton {
  GBAB_A_BUTTON   = 0b0000000001,
  GBAB_B_BUTTON   = 0b0000000010,
  GBAB_SELECT     = 0b0000000100,
  GBAB_START      = 0b0000001000,
  GBAB_DPAD_RIGHT = 0b0000010000,
  GBAB_DPAD_LEFT  = 0b0000100000,
  GBAB_DPAD_UP    = 0b0001000000,
  GBAB_DPAD_DOWN  = 0b0010000000,
  GBAB_R_TRIGGER  = 0b0100000000,
  GBAB_L_TRIGGER  = 0b1000000000,
} GBAButton;

typedef uint16_t GBAState;

struct JoybusDeviceState {
  JoybusDeviceType_Enum id;
  union {
    uint8_t data[sizeof(NormalizedGCControllerState)];
    GBAState gba_state;
    N64ControllerState n64pad_state;
    N64MouseState n64mouse_state;
    NormalizedGCControllerState gcpad_state;
    GCWheelState gcwheel_state;
  };
  
  bool operator!=(const JoybusDeviceState& b) const {
    return id != b.id || (memcmp(data, b.data, sizeof(data)));
  }
} __attribute__((packed)) ;

static constexpr uint32_t N64_ACCESSORY_TIMEOUT_MS = 10;

class JoybusPioTimeoutScope {
  private:
    uint32_t previousTimeoutMs;

  public:
    explicit JoybusPioTimeoutScope(uint32_t timeoutMs)
      : previousTimeoutMs(joybus_pio_get_timeout_ms()) {
      joybus_pio_set_timeout_ms(timeoutMs);
    }

    ~JoybusPioTimeoutScope() {
      joybus_pio_set_timeout_ms(previousTimeoutMs);
    }
};

class JoybusPort {
  private:
    const PIO _pio;
    const uint8_t _sm;
    const uint8_t _pin;
    
    JoybusPIOInstance joybus_pio;
    
    bool pio_inited;

    bool inited;
    bool initedType;
    JoybusControllerInfo info;
    
    bool rumbleEnabled;
    bool wheelMotorEnabled;
    uint8_t wheelFlags;

    bool rumblePackDetected;
    bool rumblePackPendingCommand;
    uint8_t rumblePackProbeAttempts;
    int rumblePackLastProbeResult;
    int rumblePackLastMotorResult;
    JoybusMemoryWriteDiag rumblePackLastMotorDiag;
    uint8_t rumblePackLastProbeByte;
    uint32_t rumblePackLastMotorMs;
    uint32_t n64LastInfoRefreshMs;

    JoybusDeviceState currentState;
    JoybusDeviceState lastState;

    GCControllerState gc_origin { .valid = false };
    GCControllerState gc_raw_state;
    
    void copyCurrentToLast() {
      lastState.id = currentState.id;
      memcpy(lastState.data, currentState.data, sizeof(currentState.data));
    }

    void reset(const bool resetId = false, const bool resetPrevious = false) {
      if (resetId) {
        currentState.id = JOYBUS_DEVICE_NONE;
      }

      memset(currentState.data, 0, sizeof(currentState.data));

      if (resetPrevious)
        copyCurrentToLast();
    }

    bool n64AccessoryPresent() const {
      return (info.aux & 0x01) != 0;
    }

    bool probeN64RumblePak(const uint8_t n64_cmd_delay) {
      constexpr uint8_t kRumblePakProbeLimit = 8;
      if (!n64AccessoryPresent() || rumblePackDetected || rumblePackProbeAttempts >= kRumblePakProbeLimit) {
        return rumblePackDetected;
      }

      rumblePackProbeAttempts++;
      JoybusPioTimeoutScope timeoutScope(N64_ACCESSORY_TIMEOUT_MS);
      rumblePackLastProbeByte = 0;
      uint8_t data[JOYBUS_BLOCK_SIZE];
      memset(data, 0xFE, sizeof(data));
      int res = joybus_n64_write_memory(joybus_pio, 0x8000, data);
      rumblePackLastProbeResult = res;
      delayMicroseconds(n64_cmd_delay);

      memset(data, 0x80, sizeof(data));
      res = joybus_n64_write_memory(joybus_pio, 0x8000, data);
      rumblePackLastProbeResult = res;
      delayMicroseconds(n64_cmd_delay);

      memset(data, 0x00, sizeof(data));
      res = joybus_n64_read_memory(joybus_pio, 0x8000, data);
      rumblePackLastProbeResult = res;
      rumblePackLastProbeByte = data[0];
      if (res == JOYBUS_BLOCK_SIZE && data[0] == 0x80) {
        rumblePackDetected = true;
      }

      if (rumblePackDetected) {
        rumblePackPendingCommand = rumbleEnabled;
      }
      delayMicroseconds(n64_cmd_delay);
      return rumblePackDetected;
    }

    void __not_in_flash_func(readPort)() {
      const uint16_t gc_wireless_bitmask = 0x8900;
      const uint8_t gba_cmd_delay = 100; //us //100?
      const uint8_t n64_cmd_delay =  80; //us
      const uint16_t n64_init_to_poll_delay = 2500; //us
      constexpr uint16_t kN64InfoRefreshMs = 16;
      constexpr uint16_t kGameCubeInfoRefreshMs = 16;
      constexpr uint16_t kN64RumbleKeepaliveMs = 16;
      const uint8_t gc_cmd_delay  =  20; //us
      
      if (!pio_inited)
        return;

      if (!inited) {
        info = joybus_handshake(joybus_pio, true); // 10ms timeout
    
        if (info.type == 0) {
          currentState.id = JOYBUS_DEVICE_NONE;
          gc_origin.valid = false;
          return;
        }
        inited = true;
        initedType = false;
        rumblePackDetected = false;
        rumblePackPendingCommand = false;
        rumblePackProbeAttempts = 0;
        rumblePackLastProbeResult = 0;
        rumblePackLastMotorResult = 0;
        rumblePackLastMotorDiag = {};
        rumblePackLastProbeByte = 0;
        rumblePackLastMotorMs = 0;
        n64LastInfoRefreshMs = millis();
      }
    
      if (initedType) {
        bool refreshInfo = true;
        const uint32_t now = millis();
        if (info.type == 0x0500) {
          refreshInfo = n64LastInfoRefreshMs == 0 ||
                        (uint32_t)(now - n64LastInfoRefreshMs) >= kN64InfoRefreshMs;
        } else if (info.type == 0x0900 ||
                   info.type == 0x0800 ||
                   ((info.type & gc_wireless_bitmask) == gc_wireless_bitmask)) {
          refreshInfo = n64LastInfoRefreshMs == 0 ||
                        (uint32_t)(now - n64LastInfoRefreshMs) >= kGameCubeInfoRefreshMs;
        }

        if (refreshInfo) {
          LatencyPhaseTraceScope joybusInfoTrace(LATENCY_TRACE_PHASE_JOYBUS_INFO);
          JoybusControllerInfo info_cur = joybus_handshake(joybus_pio, false);
          if (info.type == 0x0500 ||
              info.type == 0x0900 ||
              info.type == 0x0800 ||
              ((info.type & gc_wireless_bitmask) == gc_wireless_bitmask)) {
            n64LastInfoRefreshMs = millis();
          }
          if (info_cur.type != info.type) {
            bool wireless = (info_cur.type & gc_wireless_bitmask) == gc_wireless_bitmask;
            if (wireless) { // wireless receiver just got a controller connected
              info.type = info_cur.type;
              info.aux = info_cur.aux;
              return;
            } else {
              inited = false;
              initedType = false;
              currentState.id = JOYBUS_DEVICE_NONE;
              gc_origin.valid = false;
              rumblePackLastProbeResult = 0;
              rumblePackLastMotorResult = 0;
              rumblePackLastMotorDiag = {};
              rumblePackLastProbeByte = 0;
              rumblePackLastMotorMs = 0;
              n64LastInfoRefreshMs = 0;
              return;
            }
          }
      
          switch (info.type) {
            case 0x0004: // GBA
              delayMicroseconds(gba_cmd_delay);
              break;
            case 0x0500: // N64
              if (info_cur.aux != info.aux) {
                rumblePackDetected = false;
                rumblePackPendingCommand = rumbleEnabled;
                rumblePackProbeAttempts = 0;
                rumblePackLastProbeResult = 0;
                rumblePackLastMotorResult = 0;
                rumblePackLastMotorDiag = {};
                rumblePackLastProbeByte = 0;
                rumblePackLastMotorMs = 0;
              }
              info.aux = info_cur.aux;
              delayMicroseconds(n64_cmd_delay);
              break;
            case 0x0900: // GameCube
              delayMicroseconds(gc_cmd_delay);
              break;
            default:
              delayMicroseconds(100);
              break;
          }
        }
      }
    
      bool wireless = (info.type & gc_wireless_bitmask) == gc_wireless_bitmask;
      bool wireless_origin_valid = (info.type & 0x20) == 0x20;

      #if JOYBUS_ENABLE_GBA_LINK
      if (info.type == 0x0004) { // GBA
        if (!initedType) {
          int res = joybus_gba_boot(joybus_pio, ROM_gba, ROM_gba_len); // GBA booting... 
          delayMicroseconds(gba_cmd_delay);
          if (res < 0) {
            if (res == -1000) { // Invalid device
              inited = false;
            }
            return;
          }
          res = joybus_gba_default_handshake(joybus_pio, ROM_gba, ROM_gba_len); // Handshaking...
          delayMicroseconds(gba_cmd_delay);
          if (res < 0) {
            return;
          }
          initedType = true;
          delay(1);
        }
    
        uint8_t data[4];
        int res = joybus_gba_read(joybus_pio, data); // Querying GBA...
        delayMicroseconds(gba_cmd_delay);
        if (res < 0) {
          return;
        }
        
        memcpy(&currentState.gba_state, data, sizeof(currentState.gba_state));
        currentState.gba_state = ~currentState.gba_state;
        currentState.id = JOYBUS_DEVICE_GBA;
        
      } else if (info.type == 0x0500) { // N64
      #else
      if (info.type == 0x0004) { // GBA support is not compiled into this firmware.
        currentState.id = JOYBUS_DEVICE_NOTSUPPORTED;
        return;
      } else if (info.type == 0x0500) { // N64
      #endif

        // Some wireless N64 receivers require a longer settle time between
        // identify/init and the first poll command.
        if (!initedType) {
          delayMicroseconds(n64_init_to_poll_delay);
        }

        N64ControllerState state;
        {
          LatencyPhaseTraceScope n64ReadTrace(LATENCY_TRACE_PHASE_N64_READ);
          state = joybus_n64_read_controller(joybus_pio); // Querying N64...
        }
        if (!state.valid) {
          return;
        }
        currentState.n64pad_state = state;
        currentState.id = JOYBUS_DEVICE_N64PAD;

        if (n64AccessoryPresent()) {
          LatencyPhaseTraceScope n64AccessoryTrace(LATENCY_TRACE_PHASE_N64_ACCESSORY);
          probeN64RumblePak(n64_cmd_delay);
        }
        
        if (rumblePackDetected && rumbleEnabled && !rumblePackPendingCommand &&
            (rumblePackLastMotorMs == 0 ||
             (millis() - rumblePackLastMotorMs) >= kN64RumbleKeepaliveMs)) {
          rumblePackPendingCommand = true;
        }

        if (rumblePackDetected && rumblePackPendingCommand) {
          LatencyPhaseTraceScope n64RumbleTrace(LATENCY_TRACE_PHASE_N64_RUMBLE);
          JoybusPioTimeoutScope timeoutScope(N64_ACCESSORY_TIMEOUT_MS);
          delayMicroseconds(n64_cmd_delay);
          uint8_t data[JOYBUS_BLOCK_SIZE];
          memset(data, rumbleEnabled ? 0x01 : 0x00, sizeof(data));
          int res = joybus_n64_write_memory_diag(
              joybus_pio, 0xC000, data, &rumblePackLastMotorDiag);
          rumblePackLastMotorResult = res;
          delayMicroseconds(n64_cmd_delay);
          if (res == 1) { // success. clear pending
            rumblePackPendingCommand = false;
            rumblePackLastMotorMs = millis();
          }
        }

      } else if (info.type == 0x0200) { // N64 Mouse
        // N64 Mouse uses same protocol as controller but returns relative movement
        N64ControllerState state = joybus_n64_read_controller(joybus_pio); // Querying N64 Mouse...
        if (!state.valid) {
          return;
        }
        // Map controller state to mouse state
        // The data format is the same: buttons + X/Y, but X/Y are relative movement
        currentState.n64mouse_state.buttons = state.buttons;
        currentState.n64mouse_state.movement_x = state.joystick_x;
        currentState.n64mouse_state.movement_y = state.joystick_y;
        currentState.n64mouse_state.valid = true;
        currentState.id = JOYBUS_DEVICE_N64MOUSE;

      } else if (info.type == 0x0900 || wireless) { // GameCube
        if (!initedType || (wireless && !gc_origin.valid && wireless_origin_valid)) {
          gc_origin = joybus_gc_recalibrate(joybus_pio); // Recalibrating GC...
          delayMicroseconds(gc_cmd_delay);

          //read and keep origin. faster polling, but needs some testing
          if (!initedType && info.type == 0x0900) {
            //delay(1);
            gc_origin = joybus_gc_probe_origin(joybus_pio); // Probing GC origin...
            delayMicroseconds(gc_cmd_delay);
          }

        } else {
          //read origin on every poll
//          gc_origin = joybus_gc_probe_origin(joybus_pio); // Probing GC origin...
//          delayMicroseconds(gc_cmd_delay);
        }
        
        if (!gc_origin.valid) {
          return;
        }

        gc_raw_state = joybus_gc_short_poll(joybus_pio, rumbleEnabled); // Querying GC Position...
        if (!gc_raw_state.valid) {
          return;
        }
        
        NormalizedGCControllerState state = gc_normalize(gc_raw_state, gc_origin);

        if (state.analog_l < 0) state.analog_l = 0;
        if (state.analog_r < 0) state.analog_r = 0;
        
        currentState.gcpad_state = state;
        currentState.id = wireless ? JOYBUS_DEVICE_GCPAD : JOYBUS_DEVICE_GCPAD;
    
      } else if (info.type == 0x0800) { // GameCube Wheel
        if (!initedType) {
          //origin = joybus_gc_recalibrate(joybus_pio); // Recalibrating WHEEL... not needed
          gc_origin.valid = 1;
        } else {
          //origin = joybus_gc_probe_origin(joybus_pio); // Probing WHEEL origin... not needed
          //delayMicroseconds(10); // 8 seems fine
          gc_origin.valid = 1;
        }
        if (!gc_origin.valid) {
          return;
        }
        gc_raw_state = joybus_gc_wheel_poll(joybus_pio, wheelMotorEnabled, wheelFlags); // Querying WHEEL Position...
        if (!gc_raw_state.valid) {
          return;
        }

        GCWheelState state = gc_parse_wheel(gc_raw_state);
        currentState.gcwheel_state = state;
        currentState.id = JOYBUS_DEVICE_GCWHEEL;
    
      } else if (info.type == 0xA800) { // GameCube Wavebird receiver, no controller connected
        initedType = true;
        //currentState.id = JOYBUS_DEVICE_GCPAD;
        currentState.id = JOYBUS_DEVICE_GCWIRELESS;
      } else {
        inited = false;
        initedType = false;
        return;
      }
    
      initedType = true;
    }// end readPort

    
  public:
    JoybusPort(PIO pio, uint8_t sm, uint8_t pin) : _pio(pio), _sm(sm), _pin(pin), pio_inited(false) { }
                    //0020461
    //gba upload time 7341741 us
    
    void begin() {
      joybus_pio = joybus_pio_program_init(_pio, _sm, _pin);
    
      inited = false;
      initedType = false;
      info.type = 0;
      info.aux = 0;
    
      rumbleEnabled = false;
      wheelMotorEnabled = false;
      wheelFlags = 0x80;
      rumblePackDetected = false;
      rumblePackPendingCommand = false;
      rumblePackProbeAttempts = 0;
      rumblePackLastProbeResult = 0;
      rumblePackLastMotorResult = 0;
      rumblePackLastMotorDiag = {};
      rumblePackLastProbeByte = 0;
      rumblePackLastMotorMs = 0;
      n64LastInfoRefreshMs = 0;
    
      reset(true, true);
      pio_inited = true;
    }

    void setRumble(bool rumble) {
      if (rumbleEnabled != rumble)
        rumblePackPendingCommand = true; //set for N64
      rumbleEnabled = rumble;
    }

    bool isRumblePakConnected() const {
      return rumblePackDetected;
    }

    bool isRumbleCommandPending() const {
      return rumblePackPendingCommand;
    }

    uint8_t rumbleProbeAttempts() const {
      return rumblePackProbeAttempts;
    }

    int rumbleLastProbeResult() const {
      return rumblePackLastProbeResult;
    }

    uint8_t rumbleLastProbeByte() const {
      return rumblePackLastProbeByte;
    }

    int rumbleLastMotorResult() const {
      return rumblePackLastMotorResult;
    }

    const JoybusMemoryWriteDiag& rumbleLastMotorDiag() const {
      return rumblePackLastMotorDiag;
    }

    uint8_t accessoryAux() const {
      return info.aux;
    }

    int readN64Memory(uint16_t address, uint8_t response[JOYBUS_BLOCK_SIZE]) {
      if (!pio_inited || currentState.id != JOYBUS_DEVICE_N64PAD) {
        return -1000;
      }
      JoybusPioTimeoutScope timeoutScope(N64_ACCESSORY_TIMEOUT_MS);
      return joybus_n64_read_memory(joybus_pio, address, response);
    }

    int writeN64Memory(uint16_t address, uint8_t data[JOYBUS_BLOCK_SIZE]) {
      if (!pio_inited || currentState.id != JOYBUS_DEVICE_N64PAD) {
        return -1000;
      }
      JoybusPioTimeoutScope timeoutScope(N64_ACCESSORY_TIMEOUT_MS);
      return joybus_n64_write_memory(joybus_pio, address, data);
    }

    int writeN64MemoryDiag(uint16_t address, uint8_t data[JOYBUS_BLOCK_SIZE],
                           JoybusMemoryWriteDiag* diag) {
      if (!pio_inited || currentState.id != JOYBUS_DEVICE_N64PAD) {
        if (diag != nullptr) {
          diag->address = address;
          diag->checksummed_address = 0;
          diag->command = 0x03;
          diag->expected_checksum = 0;
          diag->response_checksum = 0;
          diag->transport_result = -1000;
          diag->result = -1000;
        }
        return -1000;
      }
      JoybusPioTimeoutScope timeoutScope(N64_ACCESSORY_TIMEOUT_MS);
      return joybus_n64_write_memory_diag(joybus_pio, address, data, diag);
    }
    
    void setFFB(bool motor, uint8_t direction) {
      wheelMotorEnabled = motor;
      wheelFlags = direction;
    }
    
    GCControllerState getGCOrigin() const { return gc_origin; }
    GCControllerState getGCRaw() const { return gc_raw_state; }

    bool deviceJustChanged() const { return currentState.id != lastState.id; }
    bool stateChanged() const { return currentState != lastState; }

    bool digitalPressed(const N64Button s) const { return (currentState.n64pad_state.buttons & s) > 0; }
    bool digitalChanged(const N64Button s) const { return ((lastState.n64pad_state.buttons ^ currentState.n64pad_state.buttons) & s) > 0; }
    bool digitalJustPressed(const N64Button s) const { return digitalChanged(s) & digitalPressed(s); }
    bool digitalJustReleased(const N64Button s) const { return digitalChanged(s) & !digitalPressed(s); }

    bool digitalPressed(const GBAButton s) const { return (currentState.gba_state & s) > 0; }
    bool digitalChanged(const GBAButton s) const { return ((lastState.gba_state ^ currentState.gba_state) & s) > 0; }
    bool digitalJustPressed(const GBAButton s) const { return digitalChanged(s) & digitalPressed(s); }
    bool digitalJustReleased(const GBAButton s) const { return digitalChanged(s) & !digitalPressed(s); }

    bool digitalPressed(const GCButton s) const { return (currentState.gcpad_state.buttons & s) > 0; }
    bool digitalChanged(const GCButton s) const { return ((lastState.gcpad_state.buttons ^ currentState.gcpad_state.buttons) & s) > 0; }
    bool digitalJustPressed(const GCButton s) const { return digitalChanged(s) & digitalPressed(s); }
    bool digitalJustReleased(const GCButton s) const { return digitalChanged(s) & !digitalPressed(s); }

    int analog(const JoybusAnalog_Enum s) const {
      switch (currentState.id) {
        case JOYBUS_DEVICE_N64PAD:
        {
          switch (s) {
            case JOYBUS_ANALOG_X:
              return currentState.n64pad_state.joystick_x;
            case JOYBUS_ANALOG_Y:
              return currentState.n64pad_state.joystick_y;
            default:
              return 0;
          }
          break;
        }
        case JOYBUS_DEVICE_GCPAD:
        {
          switch (s) {
            case JOYBUS_ANALOG_X:
              return currentState.gcpad_state.joystick_x;
            case JOYBUS_ANALOG_Y:
              return currentState.gcpad_state.joystick_y;
            case JOYBUS_ANALOG_CX:
              return currentState.gcpad_state.cstick_x;
            case JOYBUS_ANALOG_CY:
              return currentState.gcpad_state.cstick_y;
            case JOYBUS_ANALOG_L:
              return currentState.gcpad_state.analog_l;
            case JOYBUS_ANALOG_R:
              return currentState.gcpad_state.analog_r;
            default:
              return 0;
          }
          break;
        }
        case JOYBUS_DEVICE_GCWHEEL:
        {
          switch (s) {
            case JOYBUS_ANALOG_L:
              return currentState.gcwheel_state.analog_l;
            case JOYBUS_ANALOG_R:
              return currentState.gcwheel_state.analog_r;
            case JOYBUS_ANALOG_WHEEL:
              return currentState.gcwheel_state.wheel;
            case JOYBUS_ANALOG_GAS:
              return currentState.gcwheel_state.gas_pedal;
            case JOYBUS_ANALOG_BRAKE:
              return currentState.gcwheel_state.brake_pedal;
            default:
              return 0;
          }
          break;
        }
        default:
          return 0;
      }
      return 0;
    }

    bool analogChanged(const JoybusAnalog_Enum s) const {
      switch (currentState.id) {
        case JOYBUS_DEVICE_N64PAD:
        {
          switch (s) {
            case JOYBUS_ANALOG_X:
              return currentState.n64pad_state.joystick_x != lastState.n64pad_state.joystick_x;
            case JOYBUS_ANALOG_Y:
              return currentState.n64pad_state.joystick_y != lastState.n64pad_state.joystick_y;
            default:
              return false;
          }
          break;
        }
        case JOYBUS_DEVICE_GCPAD:
        {
          switch (s) {
            case JOYBUS_ANALOG_X:
              return currentState.gcpad_state.joystick_x != lastState.gcpad_state.joystick_x;
            case JOYBUS_ANALOG_Y:
              return currentState.gcpad_state.joystick_y != lastState.gcpad_state.joystick_y;
            case JOYBUS_ANALOG_CX:
              return currentState.gcpad_state.cstick_x != lastState.gcpad_state.cstick_x;
            case JOYBUS_ANALOG_CY:
              return currentState.gcpad_state.cstick_y != lastState.gcpad_state.cstick_y;
            case JOYBUS_ANALOG_L:
              return currentState.gcpad_state.analog_l != lastState.gcpad_state.analog_l;
            case JOYBUS_ANALOG_R:
              return currentState.gcpad_state.analog_r != lastState.gcpad_state.analog_r;
            default:
              return false;
          }
          break;
        }
        case JOYBUS_DEVICE_GCWHEEL:
        {
          switch (s) {
            case JOYBUS_ANALOG_L:
              return currentState.gcwheel_state.analog_l != lastState.gcwheel_state.analog_l;
            case JOYBUS_ANALOG_R:
              return currentState.gcwheel_state.analog_r != lastState.gcwheel_state.analog_r;
            case JOYBUS_ANALOG_WHEEL:
              return currentState.gcwheel_state.wheel != lastState.gcwheel_state.wheel;
            case JOYBUS_ANALOG_GAS:
              return currentState.gcwheel_state.gas_pedal != lastState.gcwheel_state.gas_pedal;
            case JOYBUS_ANALOG_BRAKE:
              return currentState.gcwheel_state.brake_pedal != lastState.gcwheel_state.brake_pedal;
            default:
              return false;
          }
          break;
        }
        default:
          return false;
      }
      return false;
    }


    JoybusDeviceType_Enum deviceType() const { return currentState.id; }
    GBAState getGBAState() const { return currentState.gba_state; }
    N64ControllerState getN64PadState() const { return currentState.n64pad_state; }
    N64MouseState getN64MouseState() const { return currentState.n64mouse_state; }
    NormalizedGCControllerState getGCPadState() const { return currentState.gcpad_state; }
    GCWheelState getGCWheelState() const { return currentState.gcwheel_state; }

    void __not_in_flash_func(update)() {
      if (!pio_inited)
        return;

      //keep last data
      copyCurrentToLast();
        
      readPort();

      if (currentState.id == JOYBUS_DEVICE_NONE)
        reset(false, false);
    }
};

#endif
