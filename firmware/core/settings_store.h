#pragma once

#include <stdint.h>

#include "device_mode.h"
#include "../output/output_mode.h"
#include "turbo.h"

constexpr uint8_t PERSISTED_AB_SLOT_COUNT = 2;
constexpr uint8_t SETTINGS_SCHEMA_VERSION = 1;

constexpr uint16_t PERSISTED_BLOCK_MAGIC_GLOBAL = 0x4753;   // GS
constexpr uint16_t PERSISTED_BLOCK_MAGIC_PER_MODE = 0x504D; // PM

constexpr uint8_t PER_MODE_TURBO_SIZE = TURBO_BTN_COUNT;
constexpr uint8_t PER_MODE_REMAP_SIZE = 19;
constexpr uint8_t MAX_INPUT_MODES = (uint8_t)RZORD_LAST;
constexpr uint8_t HOME_SCREEN_DEBUG_MASK = 0x01;
constexpr uint8_t HOME_BUTTON_LABELS_MASK = 0x02;
constexpr uint8_t HOME_JVS_VIEW_SHIFT = 2;
constexpr uint8_t HOME_JVS_VIEW_MASK = 0x0C;
constexpr uint8_t HOTKEY_HOLD_TIME_SHIFT = 4;
constexpr uint8_t HOTKEY_HOLD_TIME_MASK = 0x70;
constexpr uint8_t CLASSIC_DUAL_MERGE_MASK = 0x80;
constexpr uint8_t MENU_HOTKEY_QUICK_MASK = 0x01;
constexpr uint8_t MENU_HOTKEY_SYSTEM_MASK = 0x02;
constexpr uint8_t MENU_HOTKEY_KIOSK_MASK = 0x04;
constexpr uint8_t MENU_HOTKEY_OPTION_MASK = MENU_HOTKEY_KIOSK_MASK;
constexpr uint8_t MENU_HOTKEY_FORMAT_MASK = 0x80;
constexpr int8_t GUNCON_ALIGNMENT_MIN = -50;
constexpr int8_t GUNCON_ALIGNMENT_MAX = 50;

inline constexpr int8_t normalizeGunconAlignmentOffset(int32_t value) {
  return value < GUNCON_ALIGNMENT_MIN
    ? GUNCON_ALIGNMENT_MIN
    : (value > GUNCON_ALIGNMENT_MAX ? GUNCON_ALIGNMENT_MAX : (int8_t)value);
}

inline constexpr bool menuHotkeyQuickEnabledFromRaw(uint8_t raw) {
  return (raw & MENU_HOTKEY_QUICK_MASK) != 0;
}

inline constexpr bool menuHotkeySystemEnabledFromRaw(uint8_t raw) {
  if ((raw & MENU_HOTKEY_FORMAT_MASK) == 0) {
    return raw == MENU_HOTKEY_QUICK_MASK || (raw & MENU_HOTKEY_SYSTEM_MASK) != 0;
  }
  return (raw & MENU_HOTKEY_SYSTEM_MASK) != 0;
}

inline constexpr bool menuKioskEnabledFromRaw(uint8_t raw) {
  return (raw & MENU_HOTKEY_KIOSK_MASK) != 0;
}

inline constexpr uint8_t encodeMenuHotkeyFlags(bool quick, bool system, uint8_t preserve = 0) {
  return MENU_HOTKEY_FORMAT_MASK |
         (preserve & MENU_HOTKEY_OPTION_MASK) |
         (quick ? MENU_HOTKEY_QUICK_MASK : 0) |
         (system ? MENU_HOTKEY_SYSTEM_MASK : 0);
}

struct PersistedBlockHeader {
  uint16_t magic;
  uint8_t schema_version;
  uint8_t payload_size;
  uint16_t generation;
  uint16_t payload_crc16;
  uint16_t header_crc16;
};

static_assert(sizeof(PersistedBlockHeader) == 10, "PersistedBlockHeader size changed unexpectedly");
constexpr uint16_t PERSISTED_BLOCK_HEADER_SIZE = sizeof(PersistedBlockHeader);

struct GlobalSettingsRecord {
  uint8_t persisted_input_mode;
  uint8_t configured_output_mode;
  uint8_t display_contrast;
  uint8_t latency_test;
  uint8_t latency_controller_in_loop;
  uint8_t latency_host_type;
  uint8_t buzzer_mode;
  uint8_t led_mode;
  uint8_t led_brightness;
  uint8_t screensaver_mode;
  uint8_t screensaver_animation;
  uint8_t menu_hotkey;
  uint8_t home_hotkey;
  uint8_t capture_hotkey;
  uint8_t win_output;
  uint8_t psx_periph;
  uint8_t snes_rumbletech;
  uint8_t home_screen_debug;
  uint16_t sound_events;
};

static_assert(sizeof(GlobalSettingsRecord) == 20, "GlobalSettingsRecord size changed unexpectedly");

struct PerModeSettingsRecord {
  uint8_t deadzone_percent;
  uint8_t socd;
  uint8_t dpad_mode;
  uint8_t stick_invert;
  uint8_t rumble_level;
  uint8_t trigger_mode;
  uint8_t spinner_speed;
  uint8_t button_map_mode;
  uint8_t n64_z_mode;
  uint8_t n64_cstick_mode;
  uint8_t wii_analog_range;
  uint8_t n64_analog_range;
  uint8_t nso_special;
  uint8_t powerpad_mode;
  int8_t guncon_offset_x;
  int8_t guncon_offset_y;
  uint8_t reserved_musical_buttons;
  // Preserve these bytes for compatibility with existing per-mode EEPROM data.
  uint8_t reserved_mouse_analog;
  uint8_t reserved_mouse_sensitivity;
  uint8_t reserved_mouse_stick;
  uint8_t jogcon_mode;
  uint8_t jogcon_force;
  uint8_t wheel_sensitivity;
  uint8_t jogcon_digital_map;
  uint8_t jogcon_wheel_axis;
  int8_t analog_cal_lx_min;
  int8_t analog_cal_lx_max;
  int8_t analog_cal_ly_min;
  int8_t analog_cal_ly_max;
  int8_t analog_cal_rx_min;
  int8_t analog_cal_rx_max;
  int8_t analog_cal_ry_min;
  int8_t analog_cal_ry_max;
  uint8_t analog_cal_enabled;
  uint8_t turbo_rates[PER_MODE_TURBO_SIZE];
  uint8_t remaps[PER_MODE_REMAP_SIZE];
};

static_assert(sizeof(PerModeSettingsRecord) == 70, "PerModeSettingsRecord size changed unexpectedly");

using PerModeQuickSettings = PerModeSettingsRecord;

enum class SettingScope : uint8_t {
  Global = 0,
  PerMode,
};

enum class SettingValueType : uint8_t {
  UInt8 = 0,
  Int8,
  UInt16,
};

enum class SettingId : uint8_t {
  PersistedInputMode = 0,
  ConfiguredOutputMode,
  DisplayContrast,
  LatencyTest,
  LatencyControllerInLoop,
  LatencyHostType,
  BuzzerMode,
  SoundEvents,
  LedMode,
  LedBrightness,
  ScreensaverMode,
  ScreensaverAnimation,
  MenuHotkey,
  HomeHotkey,
  CaptureHotkey,
  WinOutput,
  PsxPeriph,
  SnesRumbleTech,
  HomeScreenDebug,
  HomeButtonLabels,
  HomeJvsView,
  HotkeyHoldTime,
  Deadzone,
  Socd,
  DpadMode,
  StickInvert,
  RumbleLevel,
  TriggerMode,
  SpinnerSpeed,
  ButtonMapMode,
  N64ZMode,
  N64CStickMode,
  WiiAnalogRange,
  N64AnalogRange,
  NsoSpecial,
  PowerpadMode,
  GunconOffsetX,
  GunconOffsetY,
  ReservedMusicalButtons,
  ReservedMouseAnalog,
  ReservedMouseSensitivity,
  ReservedMouseStick,
  JogconMode,
  JogconForce,
  JogconRange,
  JogconDigitalMap,
  JogconWheelAxis,
  AnalogCalLXMin,
  AnalogCalLXMax,
  AnalogCalLYMin,
  AnalogCalLYMax,
  AnalogCalRXMin,
  AnalogCalRXMax,
  AnalogCalRYMin,
  AnalogCalRYMax,
  AnalogCalEnabled,
  ClassicDualMerge,
  SystemMenuHotkey,
  KioskMode,
  Count,
};

extern uint16_t g_eeprom_remap_index;

bool isConcreteModeForPerSettings(DeviceEnum mode);
uint8_t defaultButtonMapModeForInputMode(DeviceEnum mode);
uint8_t defaultZButtonModeForInputMode(DeviceEnum mode);
uint8_t defaultSpinnerSpeedForInputMode(DeviceEnum mode);
uint8_t defaultTriggerModeForInputMode(DeviceEnum mode);
uint8_t defaultRumbleLevelForInputMode(DeviceEnum mode);

bool readGlobalSettings(GlobalSettingsRecord& settings);
void writeGlobalSettings(const GlobalSettingsRecord& settings);
bool readPerModeSettings(DeviceEnum mode, PerModeSettingsRecord& settings);
void writePerModeSettings(DeviceEnum mode, const PerModeSettingsRecord& settings);

int32_t loadSettingValue(SettingId id, DeviceEnum mode);
void saveSettingValue(SettingId id, int32_t value, DeviceEnum mode);

void sanitizeGlobalSettings(GlobalSettingsRecord& settings);
void sanitizePerModeSettings(DeviceEnum mode, PerModeSettingsRecord& settings);
void applyPerModeSettings(const PerModeSettingsRecord& settings);
void captureRuntimePerModeSettings(DeviceEnum mode, PerModeSettingsRecord& settings);
void captureMenuPerModeSettings(DeviceEnum mode, PerModeSettingsRecord& settings);
void savePerModeSettingsFromRuntime(DeviceEnum mode);
void savePerModeSettingsFromMenu(DeviceEnum mode);

inline bool readPerModeQuickSettings(DeviceEnum mode, PerModeQuickSettings& settings) {
  return readPerModeSettings(mode, settings);
}

inline void writePerModeQuickSettings(DeviceEnum mode, const PerModeQuickSettings& settings) {
  writePerModeSettings(mode, settings);
}

inline void sanitizePerModeQuickSettings(DeviceEnum mode, PerModeQuickSettings& settings) {
  sanitizePerModeSettings(mode, settings);
}

inline void applyPerModeQuickSettings(const PerModeQuickSettings& settings) {
  applyPerModeSettings(settings);
}

inline void captureRuntimeQuickSettings(DeviceEnum mode, PerModeQuickSettings& settings) {
  captureRuntimePerModeSettings(mode, settings);
}

inline void captureMenuQuickSettings(DeviceEnum mode, PerModeQuickSettings& settings) {
  captureMenuPerModeSettings(mode, settings);
}

inline void savePerModeQuickSettingsFromRuntime(DeviceEnum mode) {
  savePerModeSettingsFromRuntime(mode);
}

inline void savePerModeQuickSettingsFromMenu(DeviceEnum mode) {
  savePerModeSettingsFromMenu(mode);
}

void loadPerModeSettings();
void persistConfiguredInputMode(DeviceEnum mode);
void saveModeSettings();
bool saveMenuModeSettings();
void saveModeSettings(DeviceEnum newInputMode, outputMode_t newOutputMode, const uint8_t* tempRates,
                      const PerModeSettingsRecord& selectedSettings);

void saveSystemSettingByte(SettingId id, uint8_t value);
void saveSystemSettingWord(SettingId id, uint16_t value);
void saveSoundSettings(uint8_t buzzerModeValue, uint16_t soundEvents);
void saveSoundEventSettings(uint16_t soundEvents);
void saveScreensaverSettings(uint8_t screensaverModeValue, uint8_t screensaverAnimationValue);
void saveGunconOffsets(int8_t offsetX, int8_t offsetY);
void saveButtonRemaps();
void factoryResetSettings();

TurboInputMode getTurboInputModeForDeviceMode(DeviceEnum mode);

#include "settings_layout.h"
