#pragma once

#include <stdint.h>

#include "device_mode.h"
#include "../input/shared/input_button_bits.h"

inline bool buttonMapModeAppliesToInputMode(DeviceEnum mode) {
  switch (mode) {
    #ifdef ENABLE_INPUT_NES
    case RZORD_NES:
    #endif
    #ifdef ENABLE_INPUT_SNES
    case RZORD_SNES:
    #endif
    #ifdef ENABLE_INPUT_N64
    case RZORD_N64:
    #endif
    #ifdef ENABLE_INPUT_GAMECUBE
    case RZORD_GAMECUBE:
    #endif
    #ifdef ENABLE_INPUT_WII
    case RZORD_WII:
    #endif
      return true;
    default:
      return false;
  }
}

inline uint32_t applyNintendoPositionButtonMap(uint32_t buttons, bool positionMapActive) {
  if (!positionMapActive) {
    return buttons;
  }

  uint32_t mapped = buttons & ~(INPUT_A | INPUT_B | INPUT_X | INPUT_Y);
  if (buttons & INPUT_A) mapped |= INPUT_B;
  if (buttons & INPUT_B) mapped |= INPUT_A;
  if (buttons & INPUT_X) mapped |= INPUT_Y;
  if (buttons & INPUT_Y) mapped |= INPUT_X;
  return mapped;
}
