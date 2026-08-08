#pragma once

// Advanced Button Handler
// Detects single press, double tap, and long press

#include <Arduino.h>

// Timing constants (ms)
#define BTN_DEBOUNCE_MS      50    // Debounce time
#define BTN_DOUBLE_TAP_MS    300   // Max time between taps for double-tap
#define BTN_LONG_PRESS_MS    3000  // Time to hold for long press (3 seconds)

enum ButtonEvent : uint8_t {
  BTN_EVENT_NONE = 0,
  BTN_EVENT_SINGLE,      // Single press (after release, no second tap)
  BTN_EVENT_DOUBLE,      // Double tap detected
  BTN_EVENT_LONG,        // Long press
  BTN_EVENT_LONG_RELEASE // Released after long press
};

class ButtonHandler {
private:
  uint8_t pin;
  bool active_low;

  // State tracking
  bool last_raw;
  bool current_state;
  uint32_t last_change_time;
  uint32_t press_start_time;

  // Event detection state
  uint8_t tap_count;
  uint32_t first_tap_time;
  bool long_press_fired;
  bool waiting_for_double_tap;
  bool suppress_until_release;

  // Pending event (for events detected during delay polling)
  ButtonEvent pendingEvent;

  bool readButton();

public:
  ButtonHandler(uint8_t pin_num, bool is_active_low = true);
  void begin();
  void suppressUntilRelease();
  bool isPressed();

  // Poll button state without consuming events
  // Use this during blocking delays to keep timing accurate
  // Events will be preserved for the next update() call
  void poll();

  // Call this every loop iteration
  // Returns the detected event (if any)
  ButtonEvent update();
};

// BOOTSEL Button Handler - same as ButtonHandler but reads BOOTSEL instead of GPIO
// Note: BOOTSEL is defined by the RP2040 core and reads the boot button
class BootselButtonHandler {
private:
  // State tracking
  bool last_raw;
  bool current_state;
  uint32_t last_change_time;
  uint32_t press_start_time;

  // Event detection state
  uint8_t tap_count;
  uint32_t first_tap_time;
  bool long_press_fired;
  bool waiting_for_double_tap;
  bool suppress_until_release;

  // Pending event (for events detected during delay polling)
  ButtonEvent pendingEvent;

  bool readButton();

public:
  BootselButtonHandler();
  void begin();
  void suppressUntilRelease();
  bool isPressed();

  // Poll button state without consuming events
  void poll();

  // Call this every loop iteration
  // Returns the detected event (if any)
  ButtonEvent update();
};
