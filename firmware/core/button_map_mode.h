#pragma once

#include <stdint.h>

#include "device_mode.h"
#include "../input/shared/input_button_bits.h"
#include "../output/output_mode.h"

inline bool buttonMapModeAppliesToInputMode(DeviceEnum mode) {
  #ifdef ENABLE_INPUT_NES
  if (mode == RZORD_NES) return true;
  #endif
  #ifdef ENABLE_INPUT_SNES
  if (mode == RZORD_SNES) return true;
  #endif
  #ifdef ENABLE_INPUT_N64
  if (mode == RZORD_N64) return true;
  #endif
  #ifdef ENABLE_INPUT_GAMECUBE
  if (mode == RZORD_GAMECUBE) return true;
  #endif
  #ifdef ENABLE_INPUT_WII
  if (mode == RZORD_WII) return true;
  #endif
  #ifdef ENABLE_INPUT_VBOY
  if (mode == RZORD_VBOY) return true;
  #endif
  return false;
}

enum class ButtonMapModePolicy : uint8_t {
  NotApplicable = 0,
  UserSelectable,
  ForceName,
};

// Name/Position is useful only when the source and destination face-button
// layouts differ. Matching Nintendo layouts always preserve their printed
// labels, regardless of a stale Position value saved for another output.
inline ButtonMapModePolicy buttonMapModePolicy(
    DeviceEnum inputMode,
    outputMode_t outputMode,
    bool nsoSpecialActive = false) {
  if (!buttonMapModeAppliesToInputMode(inputMode)) {
    return ButtonMapModePolicy::NotApplicable;
  }

  if (outputMode == OUTPUT_SWITCH || outputMode == OUTPUT_SWITCHPRO) {
    if (nsoSpecialActive && outputMode == OUTPUT_SWITCHPRO) {
      #ifdef ENABLE_INPUT_NES
      if (inputMode == RZORD_NES) return ButtonMapModePolicy::ForceName;
      #endif
      #ifdef ENABLE_INPUT_SNES
      if (inputMode == RZORD_SNES) return ButtonMapModePolicy::ForceName;
      #endif
      #ifdef ENABLE_INPUT_N64
      if (inputMode == RZORD_N64) return ButtonMapModePolicy::ForceName;
      #endif
    }
    #ifdef ENABLE_INPUT_SNES
    if (inputMode == RZORD_SNES) return ButtonMapModePolicy::ForceName;
    #endif
    #ifdef ENABLE_INPUT_WII
    if (inputMode == RZORD_WII) return ButtonMapModePolicy::ForceName;
    #endif
    // N64 and GameCube do not have a Switch Pro face-button layout.
    return ButtonMapModePolicy::UserSelectable;
  }

  #ifdef ENABLE_INPUT_GAMECUBE
  if (inputMode == RZORD_GAMECUBE &&
      (outputMode == OUTPUT_GCWIIU || outputMode == OUTPUT_CONSOLE_GC)) {
    return ButtonMapModePolicy::ForceName;
  }
  #endif
  #ifdef ENABLE_INPUT_NES
  if (inputMode == RZORD_NES && outputMode == OUTPUT_CONSOLE_NES) {
    return ButtonMapModePolicy::ForceName;
  }
  #endif
  #ifdef ENABLE_INPUT_SNES
  if (inputMode == RZORD_SNES && outputMode == OUTPUT_CONSOLE_SNES) {
    return ButtonMapModePolicy::ForceName;
  }
  #endif
  #ifdef ENABLE_INPUT_N64
  if (inputMode == RZORD_N64 && outputMode == OUTPUT_CONSOLE_N64) {
    return ButtonMapModePolicy::ForceName;
  }
  #endif
  #ifdef ENABLE_INPUT_WII
  if (inputMode == RZORD_WII && outputMode == OUTPUT_CONSOLE_WII) {
    return ButtonMapModePolicy::ForceName;
  }
  #endif

  return ButtonMapModePolicy::UserSelectable;
}

inline bool buttonMapModeIsUserSelectable(
    DeviceEnum inputMode,
    outputMode_t outputMode,
    bool nsoSpecialActive = false) {
  return buttonMapModePolicy(inputMode, outputMode, nsoSpecialActive) ==
         ButtonMapModePolicy::UserSelectable;
}

inline bool effectivePositionButtonMap(
    DeviceEnum inputMode,
    outputMode_t outputMode,
    bool nsoSpecialActive,
    uint8_t savedButtonMapMode) {
  return buttonMapModeIsUserSelectable(inputMode, outputMode, nsoSpecialActive) &&
         savedButtonMapMode == 1;
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
