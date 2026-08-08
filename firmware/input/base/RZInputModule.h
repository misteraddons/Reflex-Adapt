/*******************************************************************************
 * Base input module for RetroZordReborn
 * By Matheus Fraguas (sonik-br)
*/

#ifndef RETROZORDINPUTMODULE_H_
#define RETROZORDINPUTMODULE_H_

#include <Arduino.h>

#include "../../core/device_mode.h"
#include "../../core/firmware_support.h"
#include "../runtime/input_frame_runtime.h"
#include "../state/input_port_scan_runtime_state.h"
#include "../../output/output_capabilities.h"

class ButtonDebouncer {
  private:
    const uint32_t buttonsMask;
    uint8_t debounceTimeMs;
    uint32_t debouncedAtMs[32];
    uint32_t debouncedState;
    uint32_t lastDebouncedState;
    __force_inline uint32_t buttonToMask(uint8_t button) const { return 1UL << button; }

  public:
    ButtonDebouncer(uint32_t _buttonsMask, uint8_t _debounceTimeMs);

    bool update();
    uint32_t getDebouced() const;
    uint32_t getLast() const;
    bool digitalPressed(const int8_t button) const;
    bool digitalChanged (const uint8_t button) const;
    bool digitalJustPressed (const uint8_t button) const;
    bool digitalJustReleased (const uint8_t button) const;
};


class RZInputModule {
  public:
    RZInputModule() { }
    virtual ~RZInputModule() = default;
    static inline uint32_t pollInterval {16000};
    virtual const char* getUsbId() = 0;
    virtual void configureBcdDeviceVersion() = 0;
    virtual const char* getDescription() = 0;
    virtual void setup() = 0;
    virtual void setup2() = 0;
    virtual bool poll() = 0;
    virtual void afterOutputFrameSent(bool polled, bool updated) {
      (void)polled;
      (void)updated;
    }
    virtual void setPhysicalPortMask(uint8_t mask);
    virtual bool hasPhysicalConnectionForHotSwap() const { return false; }
    virtual const char* physicalConnectionDisplayName() const { return nullptr; }

    

  protected:
    enum empty_port_behaviour_enum {
      EMPTY_PORT_ALWAYS_SCAN,
      EMPTY_PORT_USE_INTERVAL, // use polling_empty_interval_ms value
    };
    empty_port_behaviour_enum empty_port_behaviour {EMPTY_PORT_ALWAYS_SCAN};
    uint16_t polling_empty_interval_ms {1000};
    uint8_t physical_port_mask {0xFF};

    void resetState(const uint8_t index);
    bool physical_port_enabled(uint8_t port) const;
    bool canChangeMode();
    uint16_t convertAnalog(const uint8_t value);
    bool canUseAnalogTrigger();
    void beginPollCycle();
    bool endPollCycle();
    void markPollUpdated();
    void setUpdated(const uint8_t index);
    void setPortLed(const uint8_t index, const uint8_t state);
    virtual bool is_port_connected(const uint8_t index) { return true; }

    
    
    
    bool should_poll_port(uint8_t input_ports, uint8_t port);
};


inline bool RZInputModule::canChangeMode() {
  return output_allows_runtime_input_mode_change();
}

#endif
