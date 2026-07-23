#pragma once

#include <stddef.h>
#include <stdint.h>

#include "../menu/screensaver_animation.h"
#include "../platform/buzzer.h"
#include "../platform/rgb_led.h"
#include "classic_analog_range.h"
#include "controller_settings_state.h"
#include "settings_store.h"

using SettingDefaultFn = int32_t (*)(DeviceEnum mode);
using SettingSanitizerFn = int32_t (*)(DeviceEnum mode, int32_t value);

struct SettingSpec {
  SettingScope scope;
  SettingValueType value_type;
  uint16_t offset;
  int32_t default_value;
  int32_t min_value;
  int32_t max_value;
  SettingDefaultFn default_fn;
  SettingSanitizerFn sanitizer;
};

inline constexpr int32_t defaultPersistedInputModeValue(DeviceEnum) {
  return (int32_t)DEFAULT_INPUT_MODE;
}

inline constexpr int32_t defaultConfiguredOutputModeValue(DeviceEnum) {
  #ifdef ADAPT_PRIMARY_EGRESS_ESP32_WIRELESS
  return (int32_t)OUTPUT_ESP32_BT;
  #elif defined(ADAPT_PRIMARY_EGRESS_JVS_BOARD)
  return (int32_t)OUTPUT_JVS;
  #elif defined(ADAPT_PRIMARY_EGRESS_CLASSIC_CONSOLE)
  return (int32_t)OUTPUT_CONSOLE_SNES;
  #elif defined(ADAPT_PRIMARY_EGRESS_DB15)
  return (int32_t)OUTPUT_DB15_SUPERGUN;
  #elif defined(PRODUCT_CLASSIC2USB) && defined(FORCE_CLASSIC_XINPUT2P_OUTPUT) && \
        defined(ENABLE_EXPERIMENTAL_XINPUT2P_OUTPUT)
  return (int32_t)OUTPUT_XINPUT2P;
  #elif defined(PRODUCT_CLASSIC2USB) && defined(FORCE_CLASSIC_XINPUT_OUTPUT)
  // Diagnostic-only Xbox 360 build. Keep this on OUTPUT_XINPUT, not
  // OUTPUT_XINPUT2P, so console validation exercises the XSM3 auth path.
  return (int32_t)OUTPUT_XINPUT;
  #elif defined(PRODUCT_CLASSIC2USB) && defined(FORCE_CLASSIC_MANAGEMENT_OUTPUT)
  return (int32_t)OUTPUT_MISTER;
  #elif defined(PRODUCT_CLASSIC2USB)
  return (int32_t)OUTPUT_AUTO;
  #elif defined(ADAPT_PRIMARY_EGRESS_USB_HOST)
  return (int32_t)OUTPUT_AUTO;
  #else
  return (int32_t)OUTPUT_MISTER;
  #endif
}

inline int32_t defaultSpinnerSpeedValue(DeviceEnum mode) {
  return defaultSpinnerSpeedForInputMode(mode);
}

inline int32_t defaultButtonMapModeValue(DeviceEnum mode) {
  return defaultButtonMapModeForInputMode(mode);
}

inline int32_t defaultTriggerModeValue(DeviceEnum mode) {
  return defaultTriggerModeForInputMode(mode);
}

inline int32_t defaultRumbleLevelValue(DeviceEnum mode) {
  return defaultRumbleLevelForInputMode(mode);
}

inline int32_t defaultZButtonModeValue(DeviceEnum mode) {
  return defaultZButtonModeForInputMode(mode);
}

inline int32_t sanitizeSocdValue(DeviceEnum, int32_t value) {
  #if defined(PRODUCT_CLASSIC2USB)
  (void)value;
  return SOCD_OFF;
  #else
  return (value >= 0 && value < SOCD_LAST_ENUM) ? value : SOCD_OFF;
  #endif
}

inline int32_t sanitizeConfiguredOutputModeValue(DeviceEnum, int32_t value) {
  if (value == OUTPUT_RESERVED_JOGCON) {
    return OUTPUT_MISTER_JOGCON;
  }
  if (value == OUTPUT_XINPUTW) {
    #ifdef ENABLE_EXPERIMENTAL_XINPUT2P_OUTPUT
    return OUTPUT_XINPUT2P;
    #else
    return OUTPUT_MISTER;
    #endif
  }
  if (value < 0 || value >= OUTPUT_LAST) {
    return defaultConfiguredOutputModeValue(RZORD_NONE);
  }
  #if defined(PRODUCT_CLASSIC2USB)
  if (value == OUTPUT_PS5) {
    return OUTPUT_PS4;
  }
  #endif
  #if defined(PRODUCT_CLASSIC2USB) && defined(FORCE_CLASSIC_XINPUT2P_OUTPUT) && \
      defined(ENABLE_EXPERIMENTAL_XINPUT2P_OUTPUT)
  return OUTPUT_XINPUT2P;
  #endif
  #if defined(PRODUCT_CLASSIC2USB) && defined(FORCE_CLASSIC_XINPUT_OUTPUT)
  // Diagnostic-only Xbox 360 build. This deliberately bypasses the normal
  // Windows XInput2P sanitizer so retail Xbox 360 auth remains testable.
  return OUTPUT_XINPUT;
  #endif
  #if defined(PRODUCT_CLASSIC2USB) && defined(FORCE_CLASSIC_MANAGEMENT_OUTPUT)
  return OUTPUT_MISTER;
  #endif
  #if defined(ADAPT_PRIMARY_EGRESS_CLASSIC_CONSOLE)
  if (value != OUTPUT_CONSOLE_NES && value != OUTPUT_CONSOLE_SNES) {
    return defaultConfiguredOutputModeValue(RZORD_NONE);
  }
  #elif defined(ADAPT_PRIMARY_EGRESS_JVS_BOARD)
  if (value != OUTPUT_JVS) {
    return defaultConfiguredOutputModeValue(RZORD_NONE);
  }
  #elif defined(ADAPT_PRIMARY_EGRESS_DB15)
  if (value != OUTPUT_DB15_SUPERGUN) {
    return defaultConfiguredOutputModeValue(RZORD_NONE);
  }
  #endif
  return value;
}

inline int32_t sanitizeScreensaverAnimationValue(DeviceEnum, int32_t value) {
  return sanitizeScreensaverAnimation((uint8_t)value);
}

inline int32_t sanitizeJogconForceValue(DeviceEnum, int32_t value) {
  switch (value) {
    case 1:
    case 3:
    case 7:
    case 15:
      return value;
    default:
      return 1;
  }
}

inline int32_t sanitizeGunconAlignmentValue(DeviceEnum, int32_t value) {
  return normalizeGunconAlignmentOffset(value);
}

inline int32_t sanitizeSoundEventsValue(DeviceEnum, int32_t value) {
  if ((uint32_t)value == 0xFFFFu) {
    return (int32_t)SND_ALL;
  }
  return (int32_t)((uint16_t)value & SND_ALL);
}

inline constexpr int32_t defaultHotkeyHoldTimeValue(DeviceEnum) {
  return 1;  // 0.5 seconds: enough intent without making menu access sluggish.
}

inline constexpr int32_t defaultKioskModeValue(DeviceEnum) {
  return 0;
}

inline constexpr SettingSpec kSettingSpecs[] = {
  {SettingScope::Global,  SettingValueType::UInt8,  (uint16_t)offsetof(GlobalSettingsRecord, persisted_input_mode),   (int32_t)DEFAULT_INPUT_MODE, 0, RZORD_LAST - 1, defaultPersistedInputModeValue, nullptr},
  {SettingScope::Global,  SettingValueType::UInt8,  (uint16_t)offsetof(GlobalSettingsRecord, configured_output_mode), (int32_t)OUTPUT_AUTO,       0, OUTPUT_LAST - 1, defaultConfiguredOutputModeValue, sanitizeConfiguredOutputModeValue},
  {SettingScope::Global,  SettingValueType::UInt8,  (uint16_t)offsetof(GlobalSettingsRecord, display_contrast),       128,                          1, 254,              nullptr, nullptr},
  {SettingScope::Global,  SettingValueType::UInt8,  (uint16_t)offsetof(GlobalSettingsRecord, latency_test),           0,                            0, 1,                nullptr, nullptr},
  {SettingScope::Global,  SettingValueType::UInt8,  (uint16_t)offsetof(GlobalSettingsRecord, latency_controller_in_loop), 0,                         0, 1,                nullptr, nullptr},
  {SettingScope::Global,  SettingValueType::UInt8,  (uint16_t)offsetof(GlobalSettingsRecord, latency_host_type),      0,                            0, 2,                nullptr, nullptr},
  {SettingScope::Global,  SettingValueType::UInt8,  (uint16_t)offsetof(GlobalSettingsRecord, buzzer_mode),            1,                            0, 2,                nullptr, nullptr},
  {SettingScope::Global,  SettingValueType::UInt16, (uint16_t)offsetof(GlobalSettingsRecord, sound_events),           (int32_t)SND_ALL,             0, SND_ALL,          nullptr, sanitizeSoundEventsValue},
  {SettingScope::Global,  SettingValueType::UInt8,  (uint16_t)offsetof(GlobalSettingsRecord, led_mode),               (int32_t)LED_MODE_OFF,        LED_MODE_OFF, LED_MODE_LAST - 1, nullptr, nullptr},
  {SettingScope::Global,  SettingValueType::UInt8,  (uint16_t)offsetof(GlobalSettingsRecord, led_brightness),         128,                          0, 254,              nullptr, nullptr},
  {SettingScope::Global,  SettingValueType::UInt8,  (uint16_t)offsetof(GlobalSettingsRecord, screensaver_mode),       4,                            0, 6,                nullptr, nullptr},
  {SettingScope::Global,  SettingValueType::UInt8,  (uint16_t)offsetof(GlobalSettingsRecord, screensaver_animation),  0,                            0, 13,               nullptr, sanitizeScreensaverAnimationValue},
  {SettingScope::Global,  SettingValueType::UInt8,  (uint16_t)offsetof(GlobalSettingsRecord, menu_hotkey),            0,                            0, 1,                nullptr, nullptr},
  {SettingScope::Global,  SettingValueType::UInt8,  (uint16_t)offsetof(GlobalSettingsRecord, home_hotkey),            1,                            0, 1,                nullptr, nullptr},
  {SettingScope::Global,  SettingValueType::UInt8,  (uint16_t)offsetof(GlobalSettingsRecord, capture_hotkey),         0,                            0, 1,                nullptr, nullptr},
  {SettingScope::Global,  SettingValueType::UInt8,  (uint16_t)offsetof(GlobalSettingsRecord, win_output),             0,                            0, 2,                nullptr, nullptr},
  {SettingScope::Global,  SettingValueType::UInt8,  (uint16_t)offsetof(GlobalSettingsRecord, psx_periph),             0,                            0, 0,                nullptr, nullptr},
  {SettingScope::Global,  SettingValueType::UInt8,  (uint16_t)offsetof(GlobalSettingsRecord, snes_rumbletech),        0,                            0, 1,                nullptr, nullptr},
  {SettingScope::Global,  SettingValueType::UInt8,  (uint16_t)offsetof(GlobalSettingsRecord, home_screen_debug),      0,                            0, 1,                nullptr, nullptr},
  // Reserved: the retired home-button-label setting must remain at this index.
  {SettingScope::Global,  SettingValueType::UInt8,  (uint16_t)offsetof(GlobalSettingsRecord, home_screen_debug),      0,                            0, 0,                nullptr, nullptr},
  {SettingScope::Global,  SettingValueType::UInt8,  (uint16_t)offsetof(GlobalSettingsRecord, home_screen_debug),      0,                            0, 1,                nullptr, nullptr},
  {SettingScope::Global,  SettingValueType::UInt8,  (uint16_t)offsetof(GlobalSettingsRecord, home_screen_debug),      1,                            0, 6,                defaultHotkeyHoldTimeValue, nullptr},
  {SettingScope::PerMode, SettingValueType::UInt8,  (uint16_t)offsetof(PerModeSettingsRecord, deadzone_percent),      0,                            0, 30,               nullptr, nullptr},
  {SettingScope::PerMode, SettingValueType::UInt8,  (uint16_t)offsetof(PerModeSettingsRecord, socd),                 SOCD_OFF,                     0, SOCD_LAST_ENUM - 1, nullptr, sanitizeSocdValue},
  {SettingScope::PerMode, SettingValueType::UInt8,  (uint16_t)offsetof(PerModeSettingsRecord, dpad_mode),            0,                            0, 3,                nullptr, nullptr},
  {SettingScope::PerMode, SettingValueType::UInt8,  (uint16_t)offsetof(PerModeSettingsRecord, stick_invert),         0,                            0, 3,                nullptr, nullptr},
  {SettingScope::PerMode, SettingValueType::UInt8,  (uint16_t)offsetof(PerModeSettingsRecord, rumble_level),         3,                            0, 3,                defaultRumbleLevelValue, nullptr},
  {SettingScope::PerMode, SettingValueType::UInt8,  (uint16_t)offsetof(PerModeSettingsRecord, trigger_mode),         0,                            0, TRIGGER_MODE_BOTH, defaultTriggerModeValue, nullptr},
  {SettingScope::PerMode, SettingValueType::UInt8,  (uint16_t)offsetof(PerModeSettingsRecord, spinner_speed),        2,                            0, 4,                defaultSpinnerSpeedValue, nullptr},
  {SettingScope::PerMode, SettingValueType::UInt8,  (uint16_t)offsetof(PerModeSettingsRecord, button_map_mode),      0,                            0, 1,                defaultButtonMapModeValue, nullptr},
  {SettingScope::PerMode, SettingValueType::UInt8,  (uint16_t)offsetof(PerModeSettingsRecord, n64_z_mode),           1,                            0, 1,                defaultZButtonModeValue, nullptr},
  {SettingScope::PerMode, SettingValueType::UInt8,  (uint16_t)offsetof(PerModeSettingsRecord, n64_cstick_mode),      0,                            0, 2,                nullptr, nullptr},
  {SettingScope::PerMode, SettingValueType::UInt8,  (uint16_t)offsetof(PerModeSettingsRecord, wii_analog_range),     CLASSIC_ANALOG_RANGE_DEFAULT, 0, CLASSIC_ANALOG_RANGE_COUNT - 1, nullptr, nullptr},
  {SettingScope::PerMode, SettingValueType::UInt8,  (uint16_t)offsetof(PerModeSettingsRecord, n64_analog_range),     CLASSIC_ANALOG_RANGE_DEFAULT, 0, CLASSIC_ANALOG_RANGE_COUNT - 1, nullptr, nullptr},
  {SettingScope::PerMode, SettingValueType::UInt8,  (uint16_t)offsetof(PerModeSettingsRecord, nso_special),          0,                            0, 1,                nullptr, nullptr},
  {SettingScope::PerMode, SettingValueType::UInt8,  (uint16_t)offsetof(PerModeSettingsRecord, powerpad_mode),        0,                            0, 1,                nullptr, nullptr},
  {SettingScope::PerMode, SettingValueType::Int8,   (uint16_t)offsetof(PerModeSettingsRecord, guncon_offset_x),      0, GUNCON_ALIGNMENT_MIN, GUNCON_ALIGNMENT_MAX, nullptr, sanitizeGunconAlignmentValue},
  {SettingScope::PerMode, SettingValueType::Int8,   (uint16_t)offsetof(PerModeSettingsRecord, guncon_offset_y),      0, GUNCON_ALIGNMENT_MIN, GUNCON_ALIGNMENT_MAX, nullptr, sanitizeGunconAlignmentValue},
  {SettingScope::PerMode, SettingValueType::UInt8,  (uint16_t)offsetof(PerModeSettingsRecord, reserved_musical_buttons), 0,                         0, 0,                nullptr, nullptr},
  {SettingScope::PerMode, SettingValueType::UInt8,  (uint16_t)offsetof(PerModeSettingsRecord, reserved_mouse_analog),      0, 0, 0,  nullptr, nullptr},
  {SettingScope::PerMode, SettingValueType::UInt8,  (uint16_t)offsetof(PerModeSettingsRecord, reserved_mouse_sensitivity), 5, 5, 5,  nullptr, nullptr},
  {SettingScope::PerMode, SettingValueType::UInt8,  (uint16_t)offsetof(PerModeSettingsRecord, reserved_mouse_stick),       0, 0, 0,  nullptr, nullptr},
  {SettingScope::PerMode, SettingValueType::UInt8,  (uint16_t)offsetof(PerModeSettingsRecord, jogcon_mode),          0,                            0, 3,                nullptr, nullptr},
  {SettingScope::PerMode, SettingValueType::UInt8,  (uint16_t)offsetof(PerModeSettingsRecord, jogcon_force),         1,                            0, 15,               nullptr, sanitizeJogconForceValue},
  {SettingScope::PerMode, SettingValueType::UInt8,  (uint16_t)offsetof(PerModeSettingsRecord, wheel_sensitivity),    1,                            0, 2,                nullptr, nullptr},
  {SettingScope::PerMode, SettingValueType::UInt8,  (uint16_t)offsetof(PerModeSettingsRecord, jogcon_digital_map),   0,                            0, 3,                nullptr, nullptr},
  {SettingScope::PerMode, SettingValueType::UInt8,  (uint16_t)offsetof(PerModeSettingsRecord, jogcon_wheel_axis),    0,                            0, 1,                nullptr, nullptr},
  {SettingScope::PerMode, SettingValueType::Int8,   (uint16_t)offsetof(PerModeSettingsRecord, analog_cal_lx_min),   -128,                       -128, 127,             nullptr, nullptr},
  {SettingScope::PerMode, SettingValueType::Int8,   (uint16_t)offsetof(PerModeSettingsRecord, analog_cal_lx_max),    127,                       -128, 127,             nullptr, nullptr},
  {SettingScope::PerMode, SettingValueType::Int8,   (uint16_t)offsetof(PerModeSettingsRecord, analog_cal_ly_min),   -128,                       -128, 127,             nullptr, nullptr},
  {SettingScope::PerMode, SettingValueType::Int8,   (uint16_t)offsetof(PerModeSettingsRecord, analog_cal_ly_max),    127,                       -128, 127,             nullptr, nullptr},
  {SettingScope::PerMode, SettingValueType::Int8,   (uint16_t)offsetof(PerModeSettingsRecord, analog_cal_rx_min),   -128,                       -128, 127,             nullptr, nullptr},
  {SettingScope::PerMode, SettingValueType::Int8,   (uint16_t)offsetof(PerModeSettingsRecord, analog_cal_rx_max),    127,                       -128, 127,             nullptr, nullptr},
  {SettingScope::PerMode, SettingValueType::Int8,   (uint16_t)offsetof(PerModeSettingsRecord, analog_cal_ry_min),   -128,                       -128, 127,             nullptr, nullptr},
  {SettingScope::PerMode, SettingValueType::Int8,   (uint16_t)offsetof(PerModeSettingsRecord, analog_cal_ry_max),    127,                       -128, 127,             nullptr, nullptr},
  {SettingScope::PerMode, SettingValueType::UInt8,  (uint16_t)offsetof(PerModeSettingsRecord, analog_cal_enabled),   0,                            0, 1,                nullptr, nullptr},
  // Stored in the dedicated per-mode ClassicDualMergeRecord, not this compatibility field.
  {SettingScope::PerMode, SettingValueType::UInt8,  (uint16_t)offsetof(PerModeSettingsRecord, reserved_musical_buttons), 0,                          0, 1,                nullptr, nullptr},
  {SettingScope::Global,  SettingValueType::UInt8,  (uint16_t)offsetof(GlobalSettingsRecord, menu_hotkey),            0,                            0, 1,                nullptr, nullptr},
  {SettingScope::Global,  SettingValueType::UInt8,  (uint16_t)offsetof(GlobalSettingsRecord, menu_hotkey),            0,                            0, 1,                defaultKioskModeValue, nullptr},
};

static_assert((sizeof(kSettingSpecs) / sizeof(kSettingSpecs[0])) == static_cast<size_t>(SettingId::Count),
              "Setting registry is out of sync with SettingId");

inline constexpr const SettingSpec& settingSpec(SettingId id) {
  return kSettingSpecs[static_cast<size_t>(id)];
}

inline constexpr bool settingIsGlobal(SettingId id) {
  return settingSpec(id).scope == SettingScope::Global;
}

inline constexpr bool settingIsPerMode(SettingId id) {
  return settingSpec(id).scope == SettingScope::PerMode;
}

inline int32_t defaultSettingValue(SettingId id, DeviceEnum mode) {
  const SettingSpec& spec = settingSpec(id);
  return spec.default_fn ? spec.default_fn(mode) : spec.default_value;
}

inline int32_t sanitizeSettingValue(SettingId id, DeviceEnum mode, int32_t value) {
  const SettingSpec& spec = settingSpec(id);
  if (spec.sanitizer != nullptr) {
    return spec.sanitizer(mode, value);
  }
  if (value < spec.min_value || value > spec.max_value) {
    return defaultSettingValue(id, mode);
  }
  return value;
}

template <typename RecordT>
inline int32_t readRecordSettingValue(const RecordT& record, SettingId id) {
  const SettingSpec& spec = settingSpec(id);
  const uint8_t* base = reinterpret_cast<const uint8_t*>(&record);
  const uint8_t* field = base + spec.offset;
  switch (spec.value_type) {
    case SettingValueType::UInt8:
      if (id == SettingId::MenuHotkey) {
        return menuHotkeyQuickEnabledFromRaw(*reinterpret_cast<const uint8_t*>(field)) ? 1 : 0;
      }
      if (id == SettingId::SystemMenuHotkey) {
        return menuHotkeySystemEnabledFromRaw(*reinterpret_cast<const uint8_t*>(field)) ? 1 : 0;
      }
      if (id == SettingId::KioskMode) {
        return menuKioskEnabledFromRaw(*reinterpret_cast<const uint8_t*>(field)) ? 1 : 0;
      }
      if (id == SettingId::HomeScreenDebug) {
        return (*reinterpret_cast<const uint8_t*>(field) & HOME_SCREEN_DEBUG_MASK) ? 1 : 0;
      }
      if (id == SettingId::HomeButtonLabels) {
        return (*reinterpret_cast<const uint8_t*>(field) & HOME_BUTTON_LABELS_MASK) ? 1 : 0;
      }
      if (id == SettingId::HomeJvsView) {
        return (*reinterpret_cast<const uint8_t*>(field) & HOME_JVS_VIEW_MASK) >> HOME_JVS_VIEW_SHIFT;
      }
      if (id == SettingId::HotkeyHoldTime) {
        return (*reinterpret_cast<const uint8_t*>(field) & HOTKEY_HOLD_TIME_MASK) >> HOTKEY_HOLD_TIME_SHIFT;
      }
      if (id == SettingId::ClassicDualMerge) return 0;
      return *reinterpret_cast<const uint8_t*>(field);
    case SettingValueType::Int8:
      return *reinterpret_cast<const int8_t*>(field);
    case SettingValueType::UInt16:
      return *reinterpret_cast<const uint16_t*>(field);
  }
  return 0;
}

template <typename RecordT>
inline void writeRecordSettingValue(RecordT& record, SettingId id, int32_t value) {
  const SettingSpec& spec = settingSpec(id);
  uint8_t* base = reinterpret_cast<uint8_t*>(&record);
  uint8_t* field = base + spec.offset;
  switch (spec.value_type) {
    case SettingValueType::UInt8:
      if (id == SettingId::MenuHotkey) {
        uint8_t& target = *reinterpret_cast<uint8_t*>(field);
        target = encodeMenuHotkeyFlags(value != 0, menuHotkeySystemEnabledFromRaw(target), target);
        break;
      }
      if (id == SettingId::SystemMenuHotkey) {
        uint8_t& target = *reinterpret_cast<uint8_t*>(field);
        target = encodeMenuHotkeyFlags(menuHotkeyQuickEnabledFromRaw(target), value != 0, target);
        break;
      }
      if (id == SettingId::KioskMode) {
        uint8_t& target = *reinterpret_cast<uint8_t*>(field);
        uint8_t preserve = (target & (uint8_t)~MENU_HOTKEY_KIOSK_MASK) |
                           (value ? MENU_HOTKEY_KIOSK_MASK : 0);
        target = encodeMenuHotkeyFlags(
          menuHotkeyQuickEnabledFromRaw(target),
          menuHotkeySystemEnabledFromRaw(target),
          preserve
        );
        break;
      }
      if (id == SettingId::HomeScreenDebug) {
        uint8_t& target = *reinterpret_cast<uint8_t*>(field);
        target = (target & (uint8_t)~HOME_SCREEN_DEBUG_MASK) |
                 (value ? HOME_SCREEN_DEBUG_MASK : 0);
        break;
      }
      if (id == SettingId::HomeButtonLabels) {
        uint8_t& target = *reinterpret_cast<uint8_t*>(field);
        target = (target & (uint8_t)~HOME_BUTTON_LABELS_MASK) |
                 (value ? HOME_BUTTON_LABELS_MASK : 0);
        break;
      }
      if (id == SettingId::HomeJvsView) {
        uint8_t& target = *reinterpret_cast<uint8_t*>(field);
        target = (target & (uint8_t)~HOME_JVS_VIEW_MASK) |
                 (uint8_t)(((uint8_t)value << HOME_JVS_VIEW_SHIFT) & HOME_JVS_VIEW_MASK);
        break;
      }
      if (id == SettingId::HotkeyHoldTime) {
        uint8_t& target = *reinterpret_cast<uint8_t*>(field);
        target = (target & (uint8_t)~HOTKEY_HOLD_TIME_MASK) |
                 (uint8_t)(((uint8_t)value << HOTKEY_HOLD_TIME_SHIFT) & HOTKEY_HOLD_TIME_MASK);
        break;
      }
      if (id == SettingId::ClassicDualMerge) break;
      *reinterpret_cast<uint8_t*>(field) = (uint8_t)value;
      break;
    case SettingValueType::Int8:
      *reinterpret_cast<int8_t*>(field) = (int8_t)value;
      break;
    case SettingValueType::UInt16:
      *reinterpret_cast<uint16_t*>(field) = (uint16_t)value;
      break;
  }
}
