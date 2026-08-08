#pragma once

#include <Arduino.h>
#include <cstring>

#include "controller_settings_state.h"
#include "controller_state.h"
#include "device_runtime_state.h"
#include "settings_store.h"
#include "../output/output_mode.h"

outputMode_t get_effective_output_mode();

constexpr uint32_t N64_OUTPUT_L1_BIT = 1UL << 4;
constexpr uint32_t N64_OUTPUT_L2_BIT = 1UL << 6;
constexpr uint32_t GC_OUTPUT_R1_BIT = 1UL << 5;
constexpr uint32_t GC_OUTPUT_SELECT_BIT = 1UL << 11;

inline bool shouldApplyGameCubeZOutputMode() {
#ifdef ENABLE_INPUT_GAMECUBE
  return deviceMode == RZORD_GAMECUBE &&
         get_effective_output_mode() != OUTPUT_GCWIIU;
#else
  return false;
#endif
}

inline bool controllerFrameTypeNameIsN64(const controller_state_t& frame) {
  return std::strncmp(frame.controller_type_name, "N64", 3) == 0;
}

inline bool controllerFrameTypeNameIsGameCube(const controller_state_t& frame) {
  return std::strcmp(frame.controller_type_name, "GC Pad") == 0 ||
         std::strcmp(frame.controller_type_name, "WaveBird") == 0;
}

inline bool controllerFrameTypeNameIsKnownJoybus(const controller_state_t& frame) {
  return controllerFrameTypeNameIsN64(frame) ||
         controllerFrameTypeNameIsGameCube(frame) ||
         std::strcmp(frame.controller_type_name, "GBA") == 0;
}

inline uint8_t joybusFrameZOutputMode(DeviceEnum frameMode) {
  return (deviceMode == frameMode) ? n64_z_mode : defaultZButtonModeForInputMode(frameMode);
}

inline bool shouldApplyN64ZOutputMode(const controller_state_t& frame) {
#ifdef ENABLE_INPUT_N64
  if (controllerFrameTypeNameIsKnownJoybus(frame)) {
    return controllerFrameTypeNameIsN64(frame) && joybusFrameZOutputMode(RZORD_N64) == 0;
  }
  return deviceMode == RZORD_N64 && n64_z_mode == 0;
#else
  (void)frame;
  return false;
#endif
}

inline bool shouldApplyGameCubeZOutputMode(const controller_state_t& frame) {
#ifdef ENABLE_INPUT_GAMECUBE
  if (controllerFrameTypeNameIsKnownJoybus(frame)) {
    return controllerFrameTypeNameIsGameCube(frame) &&
           joybusFrameZOutputMode(RZORD_GAMECUBE) == 0 &&
           get_effective_output_mode() != OUTPUT_GCWIIU;
  }
  return shouldApplyGameCubeZOutputMode();
#else
  (void)frame;
  return false;
#endif
}

// Apply shared Z-button output policy after remap/turbo/2P merge.
// N64: mode 0 combines Z into L1; mode 1 keeps Z on L2.
// GameCube: mode 0 maps Z to R1/RB; mode 1 keeps Z on Select/Back.
inline uint32_t applyZButtonOutputMode(uint32_t buttons) {
#ifdef ENABLE_INPUT_N64
  if (deviceMode == RZORD_N64 && n64_z_mode == 0 && (buttons & N64_OUTPUT_L2_BIT)) {
    buttons |= N64_OUTPUT_L1_BIT;
    buttons &= ~N64_OUTPUT_L2_BIT;
  }
#endif
#ifdef ENABLE_INPUT_GAMECUBE
  if (shouldApplyGameCubeZOutputMode() && n64_z_mode == 0 && (buttons & GC_OUTPUT_SELECT_BIT)) {
    buttons |= GC_OUTPUT_R1_BIT;
    buttons &= ~GC_OUTPUT_SELECT_BIT;
  }
#endif
  return buttons;
}

inline uint32_t applyZButtonOutputMode(uint32_t buttons, const controller_state_t& frame) {
#ifdef ENABLE_INPUT_N64
  if (shouldApplyN64ZOutputMode(frame) && (buttons & N64_OUTPUT_L2_BIT)) {
    buttons |= N64_OUTPUT_L1_BIT;
    buttons &= ~N64_OUTPUT_L2_BIT;
  }
#endif
#ifdef ENABLE_INPUT_GAMECUBE
  if (shouldApplyGameCubeZOutputMode(frame) && (buttons & GC_OUTPUT_SELECT_BIT)) {
    buttons |= GC_OUTPUT_R1_BIT;
    buttons &= ~GC_OUTPUT_SELECT_BIT;
  }
#endif
  return buttons;
}

// Compatibility name kept for older call sites/tests; this now covers GameCube Z too.
inline uint32_t applyN64ZMode(uint32_t buttons) {
  return applyZButtonOutputMode(buttons);
}

inline uint32_t applyN64ZMode(uint32_t buttons, const controller_state_t& frame) {
  return applyZButtonOutputMode(buttons, frame);
}

inline bool isN64ZCombined() {
#ifdef ENABLE_INPUT_N64
  return deviceMode == RZORD_N64 && n64_z_mode == 0;
#else
  return false;
#endif
}

inline bool isGameCubeZMappedToR1() {
#ifdef ENABLE_INPUT_GAMECUBE
  return shouldApplyGameCubeZOutputMode() && n64_z_mode == 0;
#else
  return false;
#endif
}
