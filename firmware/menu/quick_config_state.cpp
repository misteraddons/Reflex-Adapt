#include "../product_config.h"

#include <string.h>

#include "quick_config.h"
#include "../core/settings_registry.h"

#ifdef ENABLE_INPUT_PSX
#include "../input/psx/input_psx_jogcon_runtime_state.h"
#endif

namespace {

PerModeQuickSettings buildDefaultQuickSettingsForMode(DeviceEnum mode) {
  PerModeQuickSettings settings{};
  for (uint8_t i = 0; i < static_cast<uint8_t>(SettingId::Count); ++i) {
    const SettingId id = static_cast<SettingId>(i);
    if (!settingIsPerMode(id)) {
      continue;
    }
    writeRecordSettingValue(settings, id, defaultSettingValue(id, mode));
  }
  for (uint8_t i = 0; i < PER_MODE_TURBO_SIZE; ++i) {
    settings.turbo_rates[i] = TURBO_OFF;
  }
  for (uint8_t i = 0; i < PER_MODE_REMAP_SIZE; ++i) {
    settings.remaps[i] = i;
  }
  sanitizePerModeQuickSettings(mode, settings);
  return settings;
}

}  // namespace

bool QuickConfigMenu::canEditModeSpecificSettings() {
  return isConcreteModeForPerSettings(temp_input_mode);
}

bool QuickConfigMenu::isLiveInputSelection() {
  return temp_input_mode == deviceMode;
}

const TurboButtonConfig& QuickConfigMenu::getSelectedTurboConfig() {
  #ifdef ENABLE_INPUT_PSX
  if (temp_input_mode == RZORD_PSX && isLiveInputSelection()) {
    const char* typeName = controllerFrameConst(0).controller_type_name;
    if (strncmp(typeName, "JogCon", 6) == 0) {
      return getTurboButtonConfig(TURBO_MODE_PSX_JOG);
    }
    if (strcmp(typeName, "NeGcon") == 0) {
      return getTurboButtonConfig(TURBO_MODE_PSX_NEGCON);
    }
  }
  #endif
  return getTurboButtonConfig(getTurboInputModeForDeviceMode(temp_input_mode));
}

void QuickConfigMenu::loadTempQuickSettingsRecord(DeviceEnum mode,
                                                  const PerModeQuickSettings& settings) {
  temp_deadzone = settings.deadzone_percent / 5;
  temp_socd = (socdMode_t)settings.socd;
  temp_dpad_mode = settings.dpad_mode;
  temp_stick_invert = settings.stick_invert;
  temp_snes_rumbletech = 1;
  temp_rumble = settings.rumble_level;
  temp_triggers = settings.trigger_mode;
  temp_guncon_x = settings.guncon_offset_x;
  temp_guncon_y = settings.guncon_offset_y;
  temp_spinner = settings.spinner_speed;
  temp_wheel_sens = settings.wheel_sensitivity;
  temp_jogcon_digital = settings.jogcon_digital_map;
  temp_jogcon_wheel_axis = settings.jogcon_wheel_axis;
  temp_btn_map = settings.button_map_mode;
  temp_n64_z = settings.n64_z_mode;
  temp_n64_cstick = settings.n64_cstick_mode;
  temp_n64_range = sanitizeClassicAnalogRange(settings.n64_analog_range);
  temp_wii_range = sanitizeClassicAnalogRange(settings.wii_analog_range);
  temp_nso_special = settings.nso_special ? 1 : 0;
  temp_powerpad = settings.powerpad_mode;
  temp_classic_dual_merge = classicDualMergeEnabledForMode(mode) ? 1 : 0;
  temp_classic_dual_merge_p2_mask = storedClassicDualMergeP2Mask(mode);

  for (uint8_t i = 0; i < TURBO_BTN_COUNT; i++) {
    temp_rates[i] = (settings.turbo_rates[i] < TURBO_RATE_LAST) ? (TurboRate)settings.turbo_rates[i] : TURBO_OFF;
  }

  if (temp_deadzone > 6) temp_deadzone = 6;
  if (temp_socd >= SOCD_LAST_ENUM) temp_socd = SOCD_OFF;
  if (!isTempDpadModeAllowed(temp_dpad_mode)) {
    temp_dpad_mode = DPAD_MODE_DPAD;
  }
  if (temp_stick_invert > getStickInvertMax(mode)) {
    temp_stick_invert = getStickInvertMax(mode);
  }
  if (!isTempTriggerModeAllowed(temp_triggers)) {
    temp_triggers = TRIGGER_MODE_ANALOG;
  }
}

void QuickConfigMenu::loadTempQuickSettingsForMode(DeviceEnum mode) {
  PerModeQuickSettings settings;
  readPerModeQuickSettings(mode, settings);
  loadTempQuickSettingsRecord(mode, settings);
}

void QuickConfigMenu::resetTempQuickSettingsToDefaults() {
  const PerModeQuickSettings settings = buildDefaultQuickSettingsForMode(temp_input_mode);
  loadTempQuickSettingsRecord(temp_input_mode, settings);
  if (isSnesMode(temp_input_mode)) {
    temp_snes_rumbletech = 1;
  }
  temp_classic_dual_merge = 0;
  temp_classic_dual_merge_p2_mask = defaultClassicDualMergeP2Mask(temp_input_mode);
  if (isLiveInputSelection()) {
    for (uint8_t i = 0; i < REMAP_MAX_BUTTONS && i < PER_MODE_REMAP_SIZE; ++i) {
      active_remaps[i] = settings.remaps[i];
    }
    clearAnalogCalibration();
  }
  rebuildVisibleAndClampSelection();
}

bool QuickConfigMenu::isNsoSpecialActiveForSelection(DeviceEnum mode) {
  return temp_output_mode == OUTPUT_SWITCHPRO &&
         input_mode_supports_nso_special(mode) &&
         (temp_nso_special != 0 || isN64(mode));
}

void QuickConfigMenu::getSnapshot(QuickConfigSnapshot& snapshot) {
  snapshot.state = (uint8_t)state;
  snapshot.cursor = selected_index;
  snapshot.top = selected_index > 2 ? (uint8_t)(selected_index - 2) : 0;
  snapshot.item = 0xFF;
  snapshot.count = visible_count;
  snapshot.flags = 0;

  if (on_bottom_row) snapshot.flags |= 0x01;
  if (bottom_right) snapshot.flags |= 0x02;
  if (should_save) snapshot.flags |= 0x04;

  switch (state) {
    case QC_TURBO_LIST:
      snapshot.cursor = turbo_index;
      break;
    case QC_REMAP_LIST:
    case QC_REMAP_SELECT:
      snapshot.cursor = remap_index;
      break;
    case QC_ANALOG_SUBMENU:
      snapshot.cursor = analog_index;
      snapshot.count = analog_count;
      if (analog_index < analog_count) {
        snapshot.item = (uint8_t)analog_items[analog_index];
      }
      break;
    case QC_DUAL_MERGE_MAP:
      snapshot.cursor = dual_merge_index;
      snapshot.count = getDualMergeMenuCount();
      snapshot.item = (uint8_t)QCI_CLASSIC_DUAL_MERGE;
      break;
    case QC_RUMBLE_SUBMENU:
      snapshot.cursor = rumble_index;
      snapshot.count = QCI_RUMBLE_COUNT;
      snapshot.item = (uint8_t)QCI_RUMBLE;
      break;
    default:
      break;
  }

  if (snapshot.item == 0xFF && selected_index < visible_count) {
    snapshot.item = (uint8_t)visible_items[selected_index];
  }
}

void QuickConfigMenu::open() {
  state = QC_MAIN_MENU;
  selected_index = 0;
  turbo_index = 0;
  remap_index = 0;
  analog_index = 0;
  dual_merge_index = 0;
  on_bottom_row = false;
  bottom_right = false;
  bottom_index = 0;
  default_confirm_index = 0;
  needs_redraw = true;
  should_save = false;

  for (uint8_t i = 0; i < TURBO_BTN_COUNT; i++) {
    temp_rates[i] = turbo.getButtonRate((TurboButton)i);
  }
  temp_input_mode = deviceMode;
  temp_output_mode = canonicalizeOutputMode(configuredOutputMode);
  temp_win_output = (menu_win_output > 2) ? 0 : menu_win_output;
  temp_deadzone = deadzone_percent / 5;
  temp_socd = socdMode;
  temp_dpad_mode = dpad_mode;
  temp_stick_invert = stick_invert;
  temp_snes_rumbletech = 1;
  temp_rumble = rumble_level;
  temp_triggers = trigger_mode;
  temp_guncon_x = menu_guncon_offset_x;
  temp_guncon_y = menu_guncon_offset_y;
  temp_spinner = spinner_speed;
  temp_wheel_sens = menu_wheel_sensitivity;
  temp_jogcon_digital = menu_jogcon_digital_map;
  temp_jogcon_wheel_axis = menu_jogcon_wheel_axis;
  temp_btn_map = button_map_mode;
  temp_n64_z = n64_z_mode;
  temp_n64_cstick = n64_cstick_mode;
  temp_n64_range = sanitizeClassicAnalogRange(n64_analog_range);
  temp_wii_range = sanitizeClassicAnalogRange(wii_analog_range);
  temp_nso_special = nso_special ? 1 : 0;
  temp_powerpad = menu_powerpad_mode;
  temp_classic_dual_merge = classicDualMergeEnabledForMode(temp_input_mode) ? 1 : 0;
  temp_classic_dual_merge_p2_mask = storedClassicDualMergeP2Mask(temp_input_mode);
  if (temp_deadzone > 6) temp_deadzone = 6;
  if (temp_socd >= SOCD_LAST_ENUM) temp_socd = SOCD_OFF;
  if (!isTempDpadModeAllowed(temp_dpad_mode)) {
    temp_dpad_mode = DPAD_MODE_DPAD;
  }
  if (temp_stick_invert > getStickInvertMax(temp_input_mode)) {
    temp_stick_invert = getStickInvertMax(temp_input_mode);
  }
  if (!isTempTriggerModeAllowed(temp_triggers)) {
    temp_triggers = TRIGGER_MODE_ANALOG;
  }

  buildVisibleItems();
  clampVisibleSelection();
}

void QuickConfigMenu::applyAndClose() {
  should_save = true;
  const uint8_t newWinOutput = (temp_win_output > 2) ? 0 : temp_win_output;
  const bool windowsOutputDescriptorChanged =
    newWinOutput != menu_win_output &&
    configuredOutputMode == OUTPUT_AUTO &&
    autoDetectState == AUTO_STATE_WINDOWS;
  if (newWinOutput != menu_win_output) {
    auto_detect_clear_scratch_state();
  }
  menu_win_output = newWinOutput;
  saveSystemSettingByte(SettingId::WinOutput, menu_win_output);
  // Retain the persisted field for EEPROM compatibility, but RumbleTech
  // support is automatic and no longer user-configurable.
  menu_snes_rumbletech = 1;
  snes_rumbletech_enabled = 1;
  if (windowsOutputDescriptorChanged) {
    menu_usb_descriptor_reboot_required = 1;
  }
#ifdef PRODUCT_CLASSIC2USB
  if (isLiveInputSelection() &&
      (temp_classic_dual_merge ? 1 : 0) != menu_classic_dual_merge) {
    menu_usb_descriptor_reboot_required = 1;
  }
#endif
#ifdef PRODUCT_CLASSIC2USB
  saveClassicDualMergeEnabledForMode(temp_input_mode, temp_classic_dual_merge != 0);
  saveClassicDualMergeP2MaskForMode(temp_input_mode, temp_classic_dual_merge_p2_mask);
  if (isLiveInputSelection()) {
    menu_classic_dual_merge = temp_classic_dual_merge ? 1 : 0;
    classic_dual_merge_enabled = menu_classic_dual_merge;
    classic_dual_merge_p2_mask = sanitizeClassicDualMergeP2Mask(
      temp_input_mode,
      temp_classic_dual_merge_p2_mask
    );
  }
#else
  menu_classic_dual_merge = temp_classic_dual_merge ? 1 : 0;
  classic_dual_merge_enabled = menu_classic_dual_merge;
  saveSystemSettingByte(SettingId::ClassicDualMerge, menu_classic_dual_merge);
#endif

  if (isLiveInputSelection()) {
    for (uint8_t i = 0; i < TURBO_BTN_COUNT; i++) {
      turbo.setButtonRate((TurboButton)i, temp_rates[i]);
    }

    deadzone_percent = temp_deadzone * 5;
    menu_deadzone_percent = deadzone_percent;
    socdMode = temp_socd;
    menu_socdMode = temp_socd;
    dpad_mode = temp_dpad_mode;
    menu_dpad_mode = temp_dpad_mode;
    stick_invert = temp_stick_invert;
    menu_stick_invert = temp_stick_invert;
    rumble_level = temp_rumble;
    menu_rumble_level = temp_rumble;
    trigger_mode = temp_triggers;
    menu_trigger_mode = temp_triggers;
    menu_guncon_offset_x = temp_guncon_x;
    menu_guncon_offset_y = temp_guncon_y;
    spinner_speed = temp_spinner;
    menu_wheel_sensitivity = temp_wheel_sens;
    menu_jogcon_digital_map = temp_jogcon_digital;
    menu_jogcon_wheel_axis = temp_jogcon_wheel_axis;
    button_map_mode = temp_btn_map;
    menu_button_map = temp_btn_map;
    n64_z_mode = temp_n64_z;
    menu_n64_z_mode = temp_n64_z;
    n64_cstick_mode = temp_n64_cstick;
    menu_n64_cstick_mode = temp_n64_cstick;
    n64_analog_range = temp_n64_range;
    menu_n64_analog_range = temp_n64_range;
    wii_analog_range = temp_wii_range;
    menu_wii_analog_range = temp_wii_range;
    nso_special = (temp_nso_special != 0);
    menu_nso_special = nso_special;
    menu_powerpad_mode = temp_powerpad;
    #ifdef ENABLE_INPUT_PSX
    jogcon_wheelCenterSet = false;
    #endif
  }

  close();
}

void QuickConfigMenu::getTempRates(uint8_t* rates) {
  for (uint8_t i = 0; i < TURBO_BTN_COUNT; i++) {
    rates[i] = temp_rates[i];
  }
}

void QuickConfigMenu::getTempQuickSettings(DeviceEnum mode, PerModeQuickSettings& settings) {
  readPerModeQuickSettings(mode, settings);
  settings.deadzone_percent = temp_deadzone * 5;
  settings.socd = (uint8_t)temp_socd;
  settings.dpad_mode = temp_dpad_mode;
  settings.stick_invert = temp_stick_invert;
  settings.rumble_level = temp_rumble;
  settings.trigger_mode = temp_triggers;
  settings.spinner_speed = temp_spinner;
  settings.button_map_mode = temp_btn_map;
  settings.n64_z_mode = temp_n64_z;
  settings.n64_cstick_mode = temp_n64_cstick;
  settings.n64_analog_range = temp_n64_range;
  settings.wii_analog_range = temp_wii_range;
  settings.powerpad_mode = temp_powerpad;
  settings.nso_special = temp_nso_special ? 1 : 0;
  settings.guncon_offset_x = temp_guncon_x;
  settings.guncon_offset_y = temp_guncon_y;
  settings.reserved_musical_buttons = 0;
  settings.wheel_sensitivity = temp_wheel_sens;
  settings.jogcon_digital_map = temp_jogcon_digital;
  settings.jogcon_wheel_axis = temp_jogcon_wheel_axis;
  if (isLiveInputSelection()) {
    settings.analog_cal_lx_min = analogCalibration.lx.raw_min;
    settings.analog_cal_lx_max = analogCalibration.lx.raw_max;
    settings.analog_cal_ly_min = analogCalibration.ly.raw_min;
    settings.analog_cal_ly_max = analogCalibration.ly.raw_max;
    settings.analog_cal_rx_min = analogCalibration.rx.raw_min;
    settings.analog_cal_rx_max = analogCalibration.rx.raw_max;
    settings.analog_cal_ry_min = analogCalibration.ry.raw_min;
    settings.analog_cal_ry_max = analogCalibration.ry.raw_max;
    settings.analog_cal_enabled = analogCalibration.enabled ? 1 : 0;
  }
  for (uint8_t i = 0; i < TURBO_BTN_COUNT; ++i) {
    settings.turbo_rates[i] = (uint8_t)temp_rates[i];
  }
  sanitizePerModeQuickSettings(mode, settings);
}
