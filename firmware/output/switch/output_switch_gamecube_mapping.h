#pragma once

#include <stdint.h>

#include "../../core/gamecube_switch_left_mode.h"

extern "C++" {

namespace switch_gamecube {

struct LeftShoulderButtons {
  bool l;
  bool zl;
};

inline LeftShoulderButtons map_left_shoulder(
    bool hard_switch_pressed,
    bool analog_threshold_pressed,
    bool analog_triggers_to_right_stick,
    uint8_t assignment) {
  const bool pressed =
      hard_switch_pressed ||
      (!analog_triggers_to_right_stick && analog_threshold_pressed);
  if (sanitizeGameCubeLSwitchMode(assignment) == GAMECUBE_L_SWITCH_L) {
    return {pressed, false};
  }
  return {false, pressed};
}

}  // namespace switch_gamecube

}  // extern "C++"
