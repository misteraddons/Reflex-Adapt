#pragma once

#include <stdint.h>

// D-pad mode: 0=DPad (normal), 1=Left Stick, 2=Right Stick, 3=Buttons
enum dpad_mode_t : uint8_t {
  DPAD_MODE_DPAD = 0,
  DPAD_MODE_LEFT_STICK = 1,
  DPAD_MODE_RIGHT_STICK = 2,
  DPAD_MODE_BUTTONS = 3
};

static inline uint8_t effective_dpad_mode_for_sticks(uint8_t mode,
                                                     bool has_left_stick,
                                                     bool has_right_stick) {
  (void)has_left_stick;
  (void)has_right_stick;
  // D-pad-to-stick is an explicit override. If the target stick exists, the
  // D-pad intentionally replaces that physical stick for the output report.
  return mode;
}
