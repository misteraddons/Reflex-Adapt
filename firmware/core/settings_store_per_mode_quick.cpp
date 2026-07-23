#include "analog_calibration_state.h"
#include "button_remap.h"
#include "classic_analog_range.h"
#include "controller_settings_state.h"
#include "device_runtime_state.h"
#include "eeprom_helper.h"
#include "settings_layout.h"
#include "settings_store_per_mode_internal.h"
#include "../menu/menu_mode_state.h"
#include "../menu/menu_runtime_state.h"
#include "../menu/menu_working_state.h"
#include "../output/output_runtime_state.h"
#include "../platform/rgb_led.h"
#include "turbo.h"

#include <string.h>

namespace {

constexpr uint8_t kLegacyPerModeTurboSize = 14;
constexpr uint8_t kLegacyPerModeRemapSize = 16;

struct LegacyPerModeSettingsRecord {
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
  uint8_t turbo_rates[kLegacyPerModeTurboSize];
  uint8_t remaps[kLegacyPerModeRemapSize];
};

static_assert(sizeof(LegacyPerModeSettingsRecord) == 64, "Legacy per-mode settings size changed unexpectedly");
static_assert(offsetof(LegacyPerModeSettingsRecord, turbo_rates) ==
              offsetof(PerModeSettingsRecord, turbo_rates),
              "Legacy per-mode settings prefix must match current record");

constexpr uint16_t kLegacyPerModeSettingsRecordSize =
  PERSISTED_BLOCK_HEADER_SIZE + sizeof(LegacyPerModeSettingsRecord);

uint16_t legacyPerModeSettingsRecordBase(DeviceEnum mode) {
  return SETTINGS_PER_MODE_RECORD_BASE +
         ((uint16_t)mode * PERSISTED_AB_SLOT_COUNT * kLegacyPerModeSettingsRecordSize);
}

PerModeSettingsRecord buildDefaultPerModeSettings(DeviceEnum mode) {
  PerModeSettingsRecord settings = {};
  settings.deadzone_percent = 0;
  settings.socd = SOCD_OFF;
  settings.dpad_mode = 0;
  settings.stick_invert = 0;
  settings.rumble_level = defaultRumbleLevelForInputMode(mode);
  settings.trigger_mode = defaultTriggerModeForInputMode(mode);
  settings.spinner_speed = defaultSpinnerSpeedForInputMode(mode);
  settings.button_map_mode = defaultButtonMapModeForInputMode(mode);
  settings.n64_z_mode = defaultZButtonModeForInputMode(mode);
  settings.n64_cstick_mode = 0;
  settings.wii_analog_range = CLASSIC_ANALOG_RANGE_DEFAULT;
  settings.n64_analog_range = CLASSIC_ANALOG_RANGE_DEFAULT;
  settings.nso_special = 0;
  settings.powerpad_mode = 0;
  settings.guncon_offset_x = 0;
  settings.guncon_offset_y = 0;
  settings.reserved_musical_buttons = 0;
  settings.reserved_mouse_analog = 0;
  settings.reserved_mouse_sensitivity = 5;
  settings.reserved_mouse_stick = 0;
  settings.jogcon_mode = 0;
  settings.jogcon_force = 1;
  settings.wheel_sensitivity = 1;
  settings.jogcon_digital_map = 0;
  settings.jogcon_wheel_axis = 0;
  settings.analog_cal_lx_min = -128;
  settings.analog_cal_lx_max = 127;
  settings.analog_cal_ly_min = -128;
  settings.analog_cal_ly_max = 127;
  settings.analog_cal_rx_min = -128;
  settings.analog_cal_rx_max = 127;
  settings.analog_cal_ry_min = -128;
  settings.analog_cal_ry_max = 127;
  settings.analog_cal_enabled = 0;
  for (uint8_t i = 0; i < PER_MODE_TURBO_SIZE; ++i) {
    settings.turbo_rates[i] = TURBO_OFF;
  }
  for (uint8_t i = 0; i < PER_MODE_REMAP_SIZE; ++i) {
    settings.remaps[i] = i;
  }
  sanitizePerModeSettings(mode, settings);
  return settings;
}

bool readLegacyPerModeSettings(DeviceEnum mode, PerModeSettingsRecord& settings) {
  LegacyPerModeSettingsRecord legacy{};
  uint8_t slot = 0;
  uint16_t generation = 0;
  if (!readLatestPersistedRecordAB(
        legacyPerModeSettingsRecordBase(mode),
        kLegacyPerModeSettingsRecordSize,
        PERSISTED_BLOCK_MAGIC_PER_MODE,
        slot,
        generation,
        legacy)) {
    return false;
  }

  settings = buildDefaultPerModeSettings(mode);
  memcpy(&settings, &legacy, offsetof(PerModeSettingsRecord, turbo_rates));
  for (uint8_t i = 0; i < kLegacyPerModeTurboSize && i < PER_MODE_TURBO_SIZE; ++i) {
    settings.turbo_rates[i] = legacy.turbo_rates[i];
  }
  for (uint8_t i = 0; i < kLegacyPerModeRemapSize && i < PER_MODE_REMAP_SIZE; ++i) {
    settings.remaps[i] = legacy.remaps[i];
  }
  sanitizePerModeSettings(mode, settings);
  return true;
}

}  // namespace

bool readPerModeSettings(DeviceEnum mode, PerModeSettingsRecord& settings) {
  settings = buildDefaultPerModeSettings(mode);
  if (!isConcreteModeForPerSettings(mode)) {
    return false;
  }

  uint8_t slot = 0;
  uint16_t generation = 0;
  if (!readLatestPersistedRecordAB(
        getPerModeSettingsRecordBase(mode),
        PER_MODE_SETTINGS_RECORD_SIZE,
        PERSISTED_BLOCK_MAGIC_PER_MODE,
        slot,
        generation,
        settings)) {
    if (readLegacyPerModeSettings(mode, settings)) {
      return true;
    }
    settings = buildDefaultPerModeSettings(mode);
    return false;
  }

  sanitizePerModeSettings(mode, settings);
  return true;
}

void writePerModeSettings(DeviceEnum mode, const PerModeSettingsRecord& settings) {
  if (!isConcreteModeForPerSettings(mode)) {
    return;
  }
  PerModeSettingsRecord sanitized = settings;
  sanitizePerModeSettings(mode, sanitized);
  writePersistedRecordAB(
    getPerModeSettingsRecordBase(mode),
    PER_MODE_SETTINGS_RECORD_SIZE,
    PERSISTED_BLOCK_MAGIC_PER_MODE,
    sanitized
  );
}

void applyPerModeSettings(const PerModeSettingsRecord& settings) {
  deadzone_percent = settings.deadzone_percent;
  menu_deadzone_percent = settings.deadzone_percent;
  socdMode = (socdMode_t)settings.socd;
  menu_socdMode = (socdMode_t)settings.socd;
  dpad_mode = settings.dpad_mode;
  menu_dpad_mode = settings.dpad_mode;
  stick_invert = settings.stick_invert;
  menu_stick_invert = settings.stick_invert;
  rumble_level = settings.rumble_level;
  menu_rumble_level = settings.rumble_level;
  trigger_mode = settings.trigger_mode;
  menu_trigger_mode = settings.trigger_mode;
  spinner_speed = settings.spinner_speed;
  button_map_mode = settings.button_map_mode;
  menu_button_map = settings.button_map_mode;
  n64_z_mode = settings.n64_z_mode;
  menu_n64_z_mode = settings.n64_z_mode;
  n64_cstick_mode = settings.n64_cstick_mode;
  menu_n64_cstick_mode = settings.n64_cstick_mode;
  wii_analog_range = settings.wii_analog_range;
  menu_wii_analog_range = settings.wii_analog_range;
  n64_analog_range = settings.n64_analog_range;
  menu_n64_analog_range = settings.n64_analog_range;
  nso_special = (settings.nso_special != 0);
  menu_nso_special = nso_special;
  menu_powerpad_mode = settings.powerpad_mode;
  menu_guncon_offset_x = settings.guncon_offset_x;
  menu_guncon_offset_y = settings.guncon_offset_y;
  menu_jogcon_mode = settings.jogcon_mode;
  menu_jogcon_force = settings.jogcon_force;
  menu_wheel_sensitivity = settings.wheel_sensitivity;
  menu_jogcon_digital_map = settings.jogcon_digital_map;
  menu_jogcon_wheel_axis = settings.jogcon_wheel_axis;

  analogCalibration.lx.raw_min = settings.analog_cal_lx_min;
  analogCalibration.lx.raw_max = settings.analog_cal_lx_max;
  analogCalibration.ly.raw_min = settings.analog_cal_ly_min;
  analogCalibration.ly.raw_max = settings.analog_cal_ly_max;
  analogCalibration.rx.raw_min = settings.analog_cal_rx_min;
  analogCalibration.rx.raw_max = settings.analog_cal_rx_max;
  analogCalibration.ry.raw_min = settings.analog_cal_ry_min;
  analogCalibration.ry.raw_max = settings.analog_cal_ry_max;
  analogCalibration.enabled = (settings.analog_cal_enabled != 0);

}

void captureRuntimePerModeSettings(DeviceEnum mode, PerModeSettingsRecord& settings) {
  if (!readPerModeSettings(mode, settings)) {
    settings = buildDefaultPerModeSettings(mode);
  }

  settings.deadzone_percent = deadzone_percent;
  settings.socd = (uint8_t)socdMode;
  settings.dpad_mode = dpad_mode;
  settings.stick_invert = stick_invert;
  settings.rumble_level = rumble_level;
  settings.trigger_mode = trigger_mode;
  settings.spinner_speed = spinner_speed;
  settings.button_map_mode = button_map_mode;
  settings.n64_z_mode = n64_z_mode;
  settings.n64_cstick_mode = n64_cstick_mode;
  settings.wii_analog_range = wii_analog_range;
  settings.n64_analog_range = n64_analog_range;
  settings.nso_special = nso_special ? 1 : 0;
  settings.powerpad_mode = menu_powerpad_mode;
  settings.guncon_offset_x = menu_guncon_offset_x;
  settings.guncon_offset_y = menu_guncon_offset_y;
  settings.reserved_musical_buttons = 0;
  settings.jogcon_mode = menu_jogcon_mode;
  settings.jogcon_force = menu_jogcon_force;
  settings.wheel_sensitivity = menu_wheel_sensitivity;
  settings.jogcon_digital_map = menu_jogcon_digital_map;
  settings.jogcon_wheel_axis = menu_jogcon_wheel_axis;
  settings.analog_cal_lx_min = analogCalibration.lx.raw_min;
  settings.analog_cal_lx_max = analogCalibration.lx.raw_max;
  settings.analog_cal_ly_min = analogCalibration.ly.raw_min;
  settings.analog_cal_ly_max = analogCalibration.ly.raw_max;
  settings.analog_cal_rx_min = analogCalibration.rx.raw_min;
  settings.analog_cal_rx_max = analogCalibration.rx.raw_max;
  settings.analog_cal_ry_min = analogCalibration.ry.raw_min;
  settings.analog_cal_ry_max = analogCalibration.ry.raw_max;
  settings.analog_cal_enabled = analogCalibration.enabled ? 1 : 0;
  for (uint8_t i = 0; i < TURBO_BTN_COUNT; ++i) {
    settings.turbo_rates[i] = (uint8_t)turbo.getButtonRate((TurboButton)i);
  }
  for (uint8_t i = 0; i < PER_MODE_REMAP_SIZE; ++i) {
    settings.remaps[i] = active_remaps[i];
  }
  sanitizePerModeSettings(mode, settings);
}

void captureMenuPerModeSettings(DeviceEnum mode, PerModeSettingsRecord& settings) {
  if (!readPerModeSettings(mode, settings)) {
    settings = buildDefaultPerModeSettings(mode);
  }

  settings.deadzone_percent = menu_deadzone_percent;
  settings.socd = (uint8_t)menu_socdMode;
  settings.dpad_mode = menu_dpad_mode;
  settings.stick_invert = menu_stick_invert;
  settings.rumble_level = menu_rumble_level;
  settings.trigger_mode = menu_trigger_mode;
  settings.spinner_speed = spinner_speed;
  settings.button_map_mode = menu_button_map;
  settings.n64_z_mode = menu_n64_z_mode;
  settings.n64_cstick_mode = menu_n64_cstick_mode;
  settings.wii_analog_range = menu_wii_analog_range;
  settings.n64_analog_range = menu_n64_analog_range;
  settings.nso_special = menu_nso_special ? 1 : 0;
  settings.powerpad_mode = menu_powerpad_mode;
  settings.guncon_offset_x = menu_guncon_offset_x;
  settings.guncon_offset_y = menu_guncon_offset_y;
  settings.reserved_musical_buttons = 0;
  settings.jogcon_mode = menu_jogcon_mode;
  settings.jogcon_force = menu_jogcon_force;
  settings.wheel_sensitivity = menu_wheel_sensitivity;
  settings.jogcon_digital_map = menu_jogcon_digital_map;
  settings.jogcon_wheel_axis = menu_jogcon_wheel_axis;
  settings.analog_cal_lx_min = analogCalibration.lx.raw_min;
  settings.analog_cal_lx_max = analogCalibration.lx.raw_max;
  settings.analog_cal_ly_min = analogCalibration.ly.raw_min;
  settings.analog_cal_ly_max = analogCalibration.ly.raw_max;
  settings.analog_cal_rx_min = analogCalibration.rx.raw_min;
  settings.analog_cal_rx_max = analogCalibration.rx.raw_max;
  settings.analog_cal_ry_min = analogCalibration.ry.raw_min;
  settings.analog_cal_ry_max = analogCalibration.ry.raw_max;
  settings.analog_cal_enabled = analogCalibration.enabled ? 1 : 0;
  sanitizePerModeSettings(mode, settings);
}

void savePerModeSettingsFromRuntime(DeviceEnum mode) {
  if (!isConcreteModeForPerSettings(mode)) {
    return;
  }
  PerModeSettingsRecord settings{};
  captureRuntimePerModeSettings(mode, settings);
  writePerModeSettings(mode, settings);
}

void savePerModeSettingsFromMenu(DeviceEnum mode) {
  if (!isConcreteModeForPerSettings(mode)) {
    return;
  }
  PerModeSettingsRecord settings{};
  captureMenuPerModeSettings(mode, settings);
  writePerModeSettings(mode, settings);
}

bool loadAndApplyPerModeSettingsForMode(DeviceEnum mode) {
  if (!isConcreteModeForPerSettings(mode)) {
    return false;
  }

  PerModeSettingsRecord settings{};
  const bool found = readPerModeSettings(mode, settings);
  if (!found) {
    writePerModeSettings(mode, settings);
  }
  applyPerModeSettings(settings);
  return !found;
}
