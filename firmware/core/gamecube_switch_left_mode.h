#pragma once

#include <stdint.h>

constexpr uint8_t GAMECUBE_L_SWITCH_ZL = 0;
constexpr uint8_t GAMECUBE_L_SWITCH_L = 1;

inline constexpr uint8_t sanitizeGameCubeLSwitchMode(uint8_t value) {
  return value == GAMECUBE_L_SWITCH_L
           ? GAMECUBE_L_SWITCH_L
           : GAMECUBE_L_SWITCH_ZL;
}
