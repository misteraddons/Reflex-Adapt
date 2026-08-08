#pragma once

#include <stdint.h>

#include "../core/device_mode.h"
#include "../core/controller_settings_state.h"

enum class RumbleStrengthSupport : uint8_t {
  None = 0,
  Binary,
  Variable,
};

struct RumbleCapabilities {
  RumbleStrengthSupport strength = RumbleStrengthSupport::None;
  uint8_t motor_count = 0;
};

constexpr bool rumbleTypeNameEquals(const char* actual, const char* expected) {
  if (actual == nullptr || expected == nullptr) {
    return false;
  }
  while (*actual != '\0' && *expected != '\0') {
    if (*actual++ != *expected++) {
      return false;
    }
  }
  return *actual == *expected;
}

constexpr RumbleCapabilities potentialRumbleCapabilitiesForMode(DeviceEnum mode) {
  switch (mode) {
    #ifdef ENABLE_INPUT_PSX
    case RZORD_PSX:
      return {RumbleStrengthSupport::Variable, 2};
    #endif
    #ifdef ENABLE_INPUT_SNES
    case RZORD_SNES:
      return {RumbleStrengthSupport::Variable, 2};
    #endif
    #ifdef ENABLE_INPUT_N64
    case RZORD_N64:
      return {RumbleStrengthSupport::Binary, 1};
    #endif
    #ifdef ENABLE_INPUT_GAMECUBE
    case RZORD_GAMECUBE:
      return {RumbleStrengthSupport::Binary, 1};
    #endif
    default:
      return {};
  }
}

constexpr RumbleCapabilities rumbleCapabilitiesForController(
    DeviceEnum mode,
    bool connected,
    const char* type_name) {
  if (!connected) {
    return {};
  }

  switch (mode) {
    #ifdef ENABLE_INPUT_PSX
    case RZORD_PSX:
      if (rumbleTypeNameEquals(type_name, "DualShock") ||
          rumbleTypeNameEquals(type_name, "DualShock2")) {
        return {RumbleStrengthSupport::Variable, 2};
      }
      return {};
    #endif
    #ifdef ENABLE_INPUT_SNES
    case RZORD_SNES:
      // RumbleTech has no unique input signature, but it otherwise identifies
      // as a standard SNES pad. Exclude mice, multitaps, and other peripherals.
      return rumbleTypeNameEquals(type_name, "SNES Pad")
               ? RumbleCapabilities{RumbleStrengthSupport::Variable, 2}
               : RumbleCapabilities{};
    #endif
    #ifdef ENABLE_INPUT_N64
    case RZORD_N64:
      return rumbleTypeNameEquals(type_name, "N64 Rumble")
               ? RumbleCapabilities{RumbleStrengthSupport::Binary, 1}
               : RumbleCapabilities{};
    #endif
    #ifdef ENABLE_INPUT_GAMECUBE
    case RZORD_GAMECUBE:
      return rumbleTypeNameEquals(type_name, "GC Pad")
               ? RumbleCapabilities{RumbleStrengthSupport::Binary, 1}
               : RumbleCapabilities{};
    #endif
    default:
      return {};
  }
}

constexpr RumbleCapabilities combineRumbleCapabilities(
    RumbleCapabilities left,
    RumbleCapabilities right) {
  return {
    static_cast<uint8_t>(right.strength) > static_cast<uint8_t>(left.strength)
      ? right.strength
      : left.strength,
    right.motor_count > left.motor_count ? right.motor_count : left.motor_count,
  };
}

constexpr bool rumbleHasVariableStrength(RumbleCapabilities capabilities) {
  return capabilities.strength == RumbleStrengthSupport::Variable;
}

constexpr uint8_t normalizeRumbleLevelForSupport(
    RumbleStrengthSupport support,
    uint8_t level) {
  if (support == RumbleStrengthSupport::None) {
    return RUMBLE_MODE_OFF;
  }
  if (support == RumbleStrengthSupport::Binary) {
    return level == RUMBLE_MODE_OFF ? RUMBLE_MODE_OFF : RUMBLE_MODE_VARIABLE;
  }
  return level <= RUMBLE_MODE_MAX ? level : RUMBLE_MODE_VARIABLE;
}

constexpr uint8_t cycleRumbleLevelForSupport(
    RumbleStrengthSupport support,
    uint8_t level,
    bool forward) {
  const uint8_t normalized = normalizeRumbleLevelForSupport(support, level);
  if (support == RumbleStrengthSupport::None) {
    return RUMBLE_MODE_OFF;
  }
  if (support == RumbleStrengthSupport::Binary) {
    return normalized == RUMBLE_MODE_OFF
             ? RUMBLE_MODE_VARIABLE
             : RUMBLE_MODE_OFF;
  }
  return forward
           ? static_cast<uint8_t>((normalized + 1) % 3)
           : static_cast<uint8_t>(normalized == 0 ? RUMBLE_MODE_MAX : normalized - 1);
}
