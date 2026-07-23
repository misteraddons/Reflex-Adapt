#include "analog_calibration_state.h"
#include "classic_analog_range.h"
#include "controller_settings_state.h"
#include "device_runtime_state.h"
#include "settings_store.h"
#include "../menu/menu_runtime_state.h"

namespace {

void setCurrentModeRange(uint8_t range, PerModeQuickSettings& settings) {
  range = sanitizeClassicAnalogRange(range);

  switch (deviceMode) {
    #ifdef ENABLE_INPUT_N64
    case RZORD_N64:
      settings.n64_analog_range = range;
      n64_analog_range = range;
      menu_n64_analog_range = range;
      break;
    #endif
    #ifdef ENABLE_INPUT_GAMECUBE
    case RZORD_GAMECUBE:
      settings.wii_analog_range = range;
      wii_analog_range = range;
      menu_wii_analog_range = range;
      break;
    #endif
    #ifdef ENABLE_INPUT_WII
    case RZORD_WII:
      settings.wii_analog_range = range;
      wii_analog_range = range;
      menu_wii_analog_range = range;
      break;
    #endif
    #ifdef ENABLE_INPUT_DREAMCAST
    case RZORD_DREAMCAST:
      settings.wii_analog_range = range;
      wii_analog_range = range;
      menu_wii_analog_range = range;
      break;
    #endif
    default:
      break;
  }
}

uint8_t currentModeRangeFromSettings(const PerModeQuickSettings& settings) {
  switch (deviceMode) {
    #ifdef ENABLE_INPUT_N64
    case RZORD_N64:
      return sanitizeClassicAnalogRange(settings.n64_analog_range);
    #endif
    #ifdef ENABLE_INPUT_GAMECUBE
    case RZORD_GAMECUBE:
      return sanitizeClassicAnalogRange(settings.wii_analog_range);
    #endif
    #ifdef ENABLE_INPUT_WII
    case RZORD_WII:
      return sanitizeClassicAnalogRange(settings.wii_analog_range);
    #endif
    #ifdef ENABLE_INPUT_DREAMCAST
    case RZORD_DREAMCAST:
      return sanitizeClassicAnalogRange(settings.wii_analog_range);
    #endif
    default:
      return CLASSIC_ANALOG_RANGE_DEFAULT;
  }
}

}  // namespace

AnalogCalibrationData analogCalibration = {
  .lx = { .raw_min = -128, .raw_max = 127, .raw_center = 0 },
  .ly = { .raw_min = -128, .raw_max = 127, .raw_center = 0 },
  .rx = { .raw_min = -128, .raw_max = 127, .raw_center = 0 },
  .ry = { .raw_min = -128, .raw_max = 127, .raw_center = 0 },
  .enabled = false
};

int8_t applyAnalogCalibration(int8_t raw, int8_t raw_min, int8_t raw_max) {
  if (raw_min >= 0 || raw_max <= 0) return raw;

  int32_t normalized;

  if (raw <= 0) {
    if (raw < raw_min) raw = raw_min;
    normalized = ((int32_t)raw * 127) / (-raw_min);
  } else {
    if (raw > raw_max) raw = raw_max;
    normalized = ((int32_t)raw * 127) / raw_max;
  }

  if (normalized > -4 && normalized < 4) {
    return 0;
  }

  return (int8_t)constrain(normalized, -127, 127);
}

void applyAnalogCalibrationToReport(controller_state_t& report) {
  if (!analogCalibration.enabled) return;

  if (report.HAS_ANALOG_STICK_MAIN) {
    report.LX = applyAnalogCalibration(report.LX, analogCalibration.lx.raw_min, analogCalibration.lx.raw_max);
    report.LY = applyAnalogCalibration(report.LY, analogCalibration.ly.raw_min, analogCalibration.ly.raw_max);
  }
  if (report.HAS_ANALOG_STICK_AUX) {
    report.RX = applyAnalogCalibration(report.RX, analogCalibration.rx.raw_min, analogCalibration.rx.raw_max);
    report.RY = applyAnalogCalibration(report.RY, analogCalibration.ry.raw_min, analogCalibration.ry.raw_max);
  }
}

void loadAnalogCalibration() {
  PerModeQuickSettings settings;
  readPerModeQuickSettings(deviceMode, settings);
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

void saveAnalogCalibration() {
  if (!isConcreteModeForPerSettings(deviceMode)) {
    return;
  }

  PerModeQuickSettings settings{};
  captureRuntimeQuickSettings(deviceMode, settings);
  settings.analog_cal_lx_min = analogCalibration.lx.raw_min;
  settings.analog_cal_lx_max = analogCalibration.lx.raw_max;
  settings.analog_cal_ly_min = analogCalibration.ly.raw_min;
  settings.analog_cal_ly_max = analogCalibration.ly.raw_max;
  settings.analog_cal_rx_min = analogCalibration.rx.raw_min;
  settings.analog_cal_rx_max = analogCalibration.rx.raw_max;
  settings.analog_cal_ry_min = analogCalibration.ry.raw_min;
  settings.analog_cal_ry_max = analogCalibration.ry.raw_max;
  settings.analog_cal_enabled = analogCalibration.enabled ? 1 : 0;
  if (analogCalibration.enabled && classicModeHasRangeSetting(deviceMode)) {
    setCurrentModeRange(CLASSIC_ANALOG_RANGE_CALIBRATED, settings);
  }
  writePerModeQuickSettings(deviceMode, settings);
}

void clearAnalogCalibration() {
  analogCalibration.lx.raw_min = -128;
  analogCalibration.lx.raw_max = 127;
  analogCalibration.ly.raw_min = -128;
  analogCalibration.ly.raw_max = 127;
  analogCalibration.rx.raw_min = -128;
  analogCalibration.rx.raw_max = 127;
  analogCalibration.ry.raw_min = -128;
  analogCalibration.ry.raw_max = 127;
  analogCalibration.enabled = false;

  PerModeQuickSettings settings{};
  captureRuntimeQuickSettings(deviceMode, settings);
  if (classicModeHasRangeSetting(deviceMode) &&
      currentModeRangeFromSettings(settings) == CLASSIC_ANALOG_RANGE_CALIBRATED) {
    setCurrentModeRange(CLASSIC_ANALOG_RANGE_DEFAULT, settings);
  }
  saveAnalogCalibration();
}
