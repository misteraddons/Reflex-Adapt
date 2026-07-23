#include "../product_config.h"

#include "quick_config.h"
#include "../platform/buzzer.h"
#include "../core/rumble_test_runtime.h"

namespace {
uint8_t rumbleLevelToByte(uint8_t level) {
  static const uint8_t kValues[] = {0, 85, 170, 255};
  return kValues[level & 0x03u];
}
}  // namespace

void QuickConfigMenu::setTempRumbleTech(bool enabled) {
  temp_snes_rumbletech = enabled ? 1 : 0;
  if (enabled && temp_rumble == 0) {
    temp_rumble = 3;
  }
}

void QuickConfigMenu::cycleMainMenuItemNext() {
  if (on_bottom_row) {
    bottom_index = (bottom_index + 1) % 3;
    bottom_right = bottom_index == 2;
    return;
  }

  if (visible_count == 0 || selected_index >= visible_count) {
    clampVisibleSelection();
    return;
  }

  QuickConfigItem item = visible_items[selected_index];

  if (needsSubmenu(item)) {
    enterSubmenu(item);
    return;
  }

  if (!isDirectlyEditable(item)) return;

  switch (item) {
    case QCI_INPUT_MODE:
      do {
        temp_input_mode = (DeviceEnum)(temp_input_mode + 1);
        if (temp_input_mode >= RZORD_LAST) {
          temp_input_mode = (DeviceEnum)1;
        }
      } while (should_hide_input_mode(temp_input_mode));
      loadTempQuickSettingsForMode(temp_input_mode);
      rebuildVisibleAndClampSelection();
      break;

    case QCI_OUTPUT_MODE:
      temp_output_mode = cycle_visible_output_mode(temp_output_mode, true);
      rebuildVisibleAndClampSelection();
      break;

    case QCI_WIN_OUTPUT:
      temp_win_output = (temp_win_output + 1) % 3;
      break;

    case QCI_DEADZONE:
      temp_deadzone = (temp_deadzone + 1) % 7;
      break;

    case QCI_SOCD:
      temp_socd = (socdMode_t)((temp_socd + 1) % SOCD_LAST_ENUM);
      break;

    case QCI_DPAD_BUTTONS:
      cycleTempDpadMode(true);
      break;

    case QCI_STICK_INVERT:
      temp_stick_invert = (temp_stick_invert + 1) % (getStickInvertMax(temp_input_mode) + 1);
      break;

    case QCI_RUMBLETECH:
      toggleTempRumbleTech();
      rebuildVisibleAndClampSelection();
      break;

    case QCI_RUMBLE:
      temp_rumble = (temp_rumble + 1) % 4;
      break;

    case QCI_TRIGGERS:
      cycleTempTriggerMode(true);
      break;

    case QCI_SWITCH_RTRIG_STICK:
      temp_triggers = (temp_triggers == TRIGGER_MODE_RSTICK)
                        ? TRIGGER_MODE_ANALOG
                        : TRIGGER_MODE_RSTICK;
      break;

    case QCI_SPINNER_SPEED:
      temp_spinner = (temp_spinner + 1) % 5;
      break;

    case QCI_WHEEL_SENS:
      temp_wheel_sens = (temp_wheel_sens + 1) % 3;
      break;

    case QCI_JOGCON_DIGITAL:
      temp_jogcon_digital = (temp_jogcon_digital + 1) % 4;
      break;

    case QCI_JOGCON_WHEEL_AXIS:
      temp_jogcon_wheel_axis = !temp_jogcon_wheel_axis;
      break;

    case QCI_BTN_MAP:
      temp_btn_map = !temp_btn_map;
      break;

    case QCI_N64_Z:
      temp_n64_z = !temp_n64_z;
      break;

    case QCI_N64_CSTICK:
      temp_n64_cstick = (temp_n64_cstick + 1) % 3;
      break;

    case QCI_N64_RANGE:
      cycleTempClassicAnalogRange(true);
      break;

    case QCI_NSO_SPECIAL:
      temp_nso_special = !temp_nso_special;
      break;

    case QCI_POWERPAD:
      temp_powerpad = !temp_powerpad;
      break;

    case QCI_CLASSIC_DUAL_MERGE:
      temp_classic_dual_merge = !temp_classic_dual_merge;
      break;

    default:
      break;
  }
}

void QuickConfigMenu::cycleMainMenuItemPrev() {
  if (on_bottom_row) {
    bottom_index = (bottom_index == 0) ? 2 : bottom_index - 1;
    bottom_right = bottom_index == 2;
    return;
  }

  if (visible_count == 0 || selected_index >= visible_count) {
    clampVisibleSelection();
    return;
  }

  QuickConfigItem item = visible_items[selected_index];

  if (needsSubmenu(item)) {
    enterSubmenu(item);
    return;
  }

  if (!isDirectlyEditable(item)) return;

  switch (item) {
    case QCI_INPUT_MODE:
      do {
        if (temp_input_mode <= 1) {
          temp_input_mode = (DeviceEnum)(RZORD_LAST - 1);
        } else {
          temp_input_mode = (DeviceEnum)(temp_input_mode - 1);
        }
      } while (should_hide_input_mode(temp_input_mode));
      loadTempQuickSettingsForMode(temp_input_mode);
      rebuildVisibleAndClampSelection();
      break;

    case QCI_OUTPUT_MODE:
      temp_output_mode = cycle_visible_output_mode(temp_output_mode, false);
      rebuildVisibleAndClampSelection();
      break;

    case QCI_WIN_OUTPUT:
      temp_win_output = (temp_win_output == 0) ? 2 : temp_win_output - 1;
      break;

    case QCI_DEADZONE:
      temp_deadzone = (temp_deadzone == 0) ? 6 : temp_deadzone - 1;
      break;

    case QCI_SOCD:
      temp_socd = (temp_socd == 0)
                    ? (socdMode_t)(SOCD_LAST_ENUM - 1)
                    : (socdMode_t)(temp_socd - 1);
      break;

    case QCI_DPAD_BUTTONS:
      cycleTempDpadMode(false);
      break;

    case QCI_STICK_INVERT:
      temp_stick_invert = (temp_stick_invert == 0)
                            ? getStickInvertMax(temp_input_mode)
                            : temp_stick_invert - 1;
      break;

    case QCI_RUMBLETECH:
      toggleTempRumbleTech();
      rebuildVisibleAndClampSelection();
      break;

    case QCI_RUMBLE:
      temp_rumble = (temp_rumble == 0) ? 3 : temp_rumble - 1;
      break;

    case QCI_TRIGGERS:
      cycleTempTriggerMode(false);
      break;

    case QCI_SWITCH_RTRIG_STICK:
      temp_triggers = (temp_triggers == TRIGGER_MODE_RSTICK)
                        ? TRIGGER_MODE_ANALOG
                        : TRIGGER_MODE_RSTICK;
      break;

    case QCI_SPINNER_SPEED:
      temp_spinner = (temp_spinner == 0) ? 4 : temp_spinner - 1;
      break;

    case QCI_WHEEL_SENS:
      temp_wheel_sens = (temp_wheel_sens == 0) ? 2 : temp_wheel_sens - 1;
      break;

    case QCI_JOGCON_DIGITAL:
      temp_jogcon_digital = (temp_jogcon_digital == 0) ? 3 : temp_jogcon_digital - 1;
      break;

    case QCI_JOGCON_WHEEL_AXIS:
      temp_jogcon_wheel_axis = !temp_jogcon_wheel_axis;
      break;

    case QCI_BTN_MAP:
      temp_btn_map = !temp_btn_map;
      break;

    case QCI_N64_Z:
      temp_n64_z = !temp_n64_z;
      break;

    case QCI_N64_CSTICK:
      temp_n64_cstick = (temp_n64_cstick == 0) ? 2 : temp_n64_cstick - 1;
      break;

    case QCI_N64_RANGE:
      cycleTempClassicAnalogRange(false);
      break;

    case QCI_NSO_SPECIAL:
      temp_nso_special = !temp_nso_special;
      break;

    case QCI_POWERPAD:
      temp_powerpad = !temp_powerpad;
      break;

    case QCI_CLASSIC_DUAL_MERGE:
      temp_classic_dual_merge = !temp_classic_dual_merge;
      break;

    default:
      break;
  }
}

void QuickConfigMenu::cycleSettingValue() {
  switch (editing_item) {
    case QCI_DEADZONE:
      temp_deadzone = (temp_deadzone + 1) % 7;
      break;

    case QCI_SOCD:
      temp_socd = (socdMode_t)((temp_socd + 1) % SOCD_LAST_ENUM);
      break;

    case QCI_DPAD_BUTTONS:
      cycleTempDpadMode(true);
      break;

    case QCI_WIN_OUTPUT:
      temp_win_output = (temp_win_output + 1) % 3;
      break;

    case QCI_STICK_INVERT:
      temp_stick_invert = (temp_stick_invert + 1) % (getStickInvertMax(temp_input_mode) + 1);
      break;

    case QCI_RUMBLETECH:
      toggleTempRumbleTech();
      rebuildVisibleAndClampSelection();
      break;

    case QCI_RUMBLE:
      temp_rumble = (temp_rumble + 1) % 4;
      break;

    case QCI_TRIGGERS:
      cycleTempTriggerMode(true);
      break;

    case QCI_SWITCH_RTRIG_STICK:
      temp_triggers = (temp_triggers == TRIGGER_MODE_RSTICK)
                        ? TRIGGER_MODE_ANALOG
                        : TRIGGER_MODE_RSTICK;
      break;

    case QCI_N64_CSTICK:
      temp_n64_cstick = (temp_n64_cstick + 1) % 3;
      break;

    case QCI_N64_RANGE:
      cycleTempClassicAnalogRange(true);
      break;

    case QCI_NSO_SPECIAL:
      temp_nso_special = !temp_nso_special;
      break;

    case QCI_POWERPAD:
      temp_powerpad = !temp_powerpad;
      break;

    case QCI_CLASSIC_DUAL_MERGE:
      temp_classic_dual_merge = !temp_classic_dual_merge;
      break;

    default:
      break;
  }
}

void QuickConfigMenu::cycleSettingValueBack() {
  switch (editing_item) {
    case QCI_DEADZONE:
      temp_deadzone = (temp_deadzone == 0) ? 6 : temp_deadzone - 1;
      break;

    case QCI_SOCD:
      temp_socd = (temp_socd == 0)
                    ? (socdMode_t)(SOCD_LAST_ENUM - 1)
                    : (socdMode_t)(temp_socd - 1);
      break;

    case QCI_DPAD_BUTTONS:
      cycleTempDpadMode(false);
      break;

    case QCI_WIN_OUTPUT:
      temp_win_output = (temp_win_output == 0) ? 2 : temp_win_output - 1;
      break;

    case QCI_STICK_INVERT:
      temp_stick_invert = (temp_stick_invert == 0)
                            ? getStickInvertMax(temp_input_mode)
                            : temp_stick_invert - 1;
      break;

    case QCI_RUMBLETECH:
      toggleTempRumbleTech();
      rebuildVisibleAndClampSelection();
      break;

    case QCI_RUMBLE:
      temp_rumble = (temp_rumble == 0) ? 3 : temp_rumble - 1;
      break;

    case QCI_TRIGGERS:
      cycleTempTriggerMode(false);
      break;

    case QCI_SWITCH_RTRIG_STICK:
      temp_triggers = (temp_triggers == TRIGGER_MODE_RSTICK)
                        ? TRIGGER_MODE_ANALOG
                        : TRIGGER_MODE_RSTICK;
      break;

    case QCI_N64_CSTICK:
      temp_n64_cstick = (temp_n64_cstick == 0) ? 2 : temp_n64_cstick - 1;
      break;

    case QCI_N64_RANGE:
      cycleTempClassicAnalogRange(false);
      break;

    case QCI_NSO_SPECIAL:
      temp_nso_special = !temp_nso_special;
      break;

    case QCI_POWERPAD:
      temp_powerpad = !temp_powerpad;
      break;

    case QCI_CLASSIC_DUAL_MERGE:
      temp_classic_dual_merge = !temp_classic_dual_merge;
      break;

    default:
      break;
  }
}

void QuickConfigMenu::select() {
  needs_redraw = true;
  const uint8_t turboCount = getVisibleTurboCount();

  switch (state) {
    case QC_MAIN_MENU:
      if (on_bottom_row) {
        if (bottom_index == 2) {
          applyAndClose();
        } else if (bottom_index == 1) {
          state = QC_CONFIRM_DEFAULT;
          default_confirm_index = 0;
          buzzer.playMenuNav();
        } else {
          discard();
        }
        break;
      }

      if (visible_count == 0 || selected_index >= visible_count) {
        clampVisibleSelection();
        break;
      }

      enterSubmenu(visible_items[selected_index]);
      break;

    case QC_CONFIRM_DEFAULT:
      if (default_confirm_index == 1) {
        resetTempQuickSettingsToDefaults();
        on_bottom_row = true;
        bottom_index = 2;
        bottom_right = true;
      }
      state = QC_MAIN_MENU;
      break;

    case QC_TURBO_LIST:
      if (turbo_index >= turboCount) {
        state = QC_MAIN_MENU;
      } else {
        uint8_t btn_index = getTurboButtonIndexForVisibleIndex(turbo_index);
        if (btn_index != 0xFF) {
          const uint8_t rateIndex =
              (getTurboMenuIndexForRate(temp_rates[btn_index]) + 1) % TURBO_RATE_LAST;
          temp_rates[btn_index] = getTurboRateForMenuIndex(rateIndex);
        }
      }
      break;

    case QC_REMAP_LIST: {
      uint8_t btn_count = getRemapButtonCount(deviceMode);
      if (remap_index >= btn_count) {
        state = QC_MAIN_MENU;
      } else {
        const uint8_t source_slot = getRemapButtonSlot(deviceMode, remap_index);
        if (source_slot != 0xFF) {
          uint8_t target_display = getRemapButtonDisplayIndex(deviceMode, active_remaps[source_slot]);
          if (target_display == 0xFF) {
            target_display = remap_index;
          }
          target_display = (target_display + 1) % btn_count;
          active_remaps[source_slot] = getRemapButtonSlot(deviceMode, target_display);
        }
      }
      break;
    }

    case QC_ANALOG_SUBMENU:
      handleAnalogSelect();
      break;

    case QC_DUAL_MERGE_MAP:
      handleDualMergeSelect();
      break;

    case QC_RUMBLE_SUBMENU:
      handleRumbleSelect(true);
      break;

    case QC_GUNCON_ADJUST:
      handleGunconSelect();
      break;

    case QC_RANGE_TEST:
      state = return_to_analog_submenu ? QC_ANALOG_SUBMENU : QC_MAIN_MENU;
      break;

    case QC_STICK_CAL:
      if (cal_selection == 0) {
        state = return_to_analog_submenu ? QC_ANALOG_SUBMENU : QC_MAIN_MENU;
      } else if (cal_selection == 1 && cal_has_data) {
        analogCalibration.lx.raw_min = cal_lx_min;
        analogCalibration.lx.raw_max = cal_lx_max;
        analogCalibration.ly.raw_min = cal_ly_min;
        analogCalibration.ly.raw_max = cal_ly_max;
        analogCalibration.rx.raw_min = cal_rx_min;
        analogCalibration.rx.raw_max = cal_rx_max;
        analogCalibration.ry.raw_min = cal_ry_min;
        analogCalibration.ry.raw_max = cal_ry_max;
        analogCalibration.enabled = true;
        saveAnalogCalibration();
        if (classicModeHasRangeSetting(temp_input_mode)) {
          if (temp_input_mode == RZORD_N64) {
            temp_n64_range = CLASSIC_ANALOG_RANGE_CALIBRATED;
          } else {
            temp_wii_range = CLASSIC_ANALOG_RANGE_CALIBRATED;
          }
        }
        state = return_to_analog_submenu ? QC_ANALOG_SUBMENU : QC_MAIN_MENU;
      } else if (cal_selection == 2) {
        clearAnalogCalibration();
        if (classicModeHasRangeSetting(temp_input_mode)) {
          if (temp_input_mode == RZORD_N64 && temp_n64_range == CLASSIC_ANALOG_RANGE_CALIBRATED) {
            temp_n64_range = CLASSIC_ANALOG_RANGE_DEFAULT;
          } else if (temp_input_mode != RZORD_N64 && temp_wii_range == CLASSIC_ANALOG_RANGE_CALIBRATED) {
            temp_wii_range = CLASSIC_ANALOG_RANGE_DEFAULT;
          }
        }
        state = return_to_analog_submenu ? QC_ANALOG_SUBMENU : QC_MAIN_MENU;
      }
      break;

    default:
      break;
  }
}

bool QuickConfigMenu::saveShortcut() {
  needs_redraw = true;

  switch (state) {
    case QC_MAIN_MENU:
      applyAndClose();
      return true;

    case QC_STICK_CAL:
      if (!cal_has_data) {
        return false;
      }
      cal_selection = 1;
      select();
      return true;

    default:
      return false;
  }
}

void QuickConfigMenu::handleAnalogSelect() {
  buildAnalogItems();
  if (analog_index >= analog_count) {
    state = QC_MAIN_MENU;
    needs_redraw = true;
    return;
  }

  QuickConfigItem item = analog_items[analog_index];
  switch (item) {
    case QCI_RANGE_TEST:
    case QCI_STICK_CAL:
      enterSubmenu(item);
      return_to_analog_submenu = true;
      return;

    case QCI_DEADZONE:
      temp_deadzone = (temp_deadzone + 1) % 7;
      break;

    case QCI_STICK_INVERT:
      temp_stick_invert = (temp_stick_invert + 1) % (getStickInvertMax(temp_input_mode) + 1);
      break;

    case QCI_N64_RANGE:
      cycleTempClassicAnalogRange(true);
      break;

    default:
      break;
  }

  needs_redraw = true;
}

void QuickConfigMenu::handleAnalogSelectBack() {
  buildAnalogItems();
  if (analog_index >= analog_count) {
    state = QC_MAIN_MENU;
    needs_redraw = true;
    return;
  }

  QuickConfigItem item = analog_items[analog_index];
  switch (item) {
    case QCI_RANGE_TEST:
    case QCI_STICK_CAL:
      enterSubmenu(item);
      return_to_analog_submenu = true;
      return;

    case QCI_DEADZONE:
      temp_deadzone = (temp_deadzone == 0) ? 6 : temp_deadzone - 1;
      break;

    case QCI_STICK_INVERT:
      temp_stick_invert = (temp_stick_invert == 0)
                            ? getStickInvertMax(temp_input_mode)
                            : temp_stick_invert - 1;
      break;

    case QCI_N64_RANGE:
      cycleTempClassicAnalogRange(false);
      break;

    default:
      break;
  }

  needs_redraw = true;
}

uint8_t QuickConfigMenu::getDualMergeMenuCount() {
  return (uint8_t)(getClassicDualMergeButtonCount(temp_input_mode) + 1u);
}

void QuickConfigMenu::toggleDualMergeCurrentItem() {
  const uint8_t buttonCount = getClassicDualMergeButtonCount(temp_input_mode);
  if (dual_merge_index == 0) {
    temp_classic_dual_merge = !temp_classic_dual_merge;
    return;
  }
  if (dual_merge_index > buttonCount) {
    state = QC_MAIN_MENU;
    return;
  }

  const uint32_t mask = getClassicDualMergeButtonMask(temp_input_mode, dual_merge_index - 1);
  if (mask != 0) {
    temp_classic_dual_merge_p2_mask ^= mask;
    temp_classic_dual_merge_p2_mask =
      sanitizeClassicDualMergeP2Mask(temp_input_mode, temp_classic_dual_merge_p2_mask);
  }
}

void QuickConfigMenu::handleDualMergeSelect() {
  toggleDualMergeCurrentItem();
  needs_redraw = true;
}

void QuickConfigMenu::handleDualMergeSelectBack() {
  toggleDualMergeCurrentItem();
  needs_redraw = true;
}

void QuickConfigMenu::startTempRumbleTest(bool heavy, bool light) {
  const uint8_t level = rumbleLevelToByte(temp_rumble);
  if (level == 0) {
    return;
  }
  const uint8_t left = heavy ? level : 0;
  const uint8_t right = light ? level : 0;
  rumbleTestStart(left, right, 3000);
}

void QuickConfigMenu::handleRumbleSelect(bool forward) {
  switch ((RumbleMenuItem)rumble_index) {
    case QCI_RUMBLE_LEVEL:
      temp_rumble = forward ? (uint8_t)((temp_rumble + 1) % 4)
                            : (uint8_t)((temp_rumble == 0) ? 3 : temp_rumble - 1);
      break;

    case QCI_RUMBLE_TEST_HEAVY:
      startTempRumbleTest(true, false);
      break;

    case QCI_RUMBLE_TEST_LIGHT:
      startTempRumbleTest(false, true);
      break;

    case QCI_RUMBLE_TEST_BOTH:
      startTempRumbleTest(true, true);
      break;

    case QCI_RUMBLE_BACK:
      state = QC_MAIN_MENU;
      break;

    default:
      break;
  }

  needs_redraw = true;
}

void QuickConfigMenu::handleGunconSelect() {
  guncon_edit_axis++;
  if (guncon_edit_axis > 2) {
    guncon_edit_axis = 0;
    state = QC_MAIN_MENU;
  }
  needs_redraw = true;
}

void QuickConfigMenu::handleGunconNavigate() {
  if (guncon_edit_axis == 0) {
    temp_guncon_x = (temp_guncon_x + 1);
    if (temp_guncon_x > GUNCON_ALIGNMENT_MAX) temp_guncon_x = GUNCON_ALIGNMENT_MIN;
  } else if (guncon_edit_axis == 1) {
    temp_guncon_y = (temp_guncon_y + 1);
    if (temp_guncon_y > GUNCON_ALIGNMENT_MAX) temp_guncon_y = GUNCON_ALIGNMENT_MIN;
  }

  needs_redraw = true;
}

void QuickConfigMenu::handleGunconNavigateBack() {
  if (guncon_edit_axis == 0) {
    temp_guncon_x = (temp_guncon_x - 1);
    if (temp_guncon_x < GUNCON_ALIGNMENT_MIN) temp_guncon_x = GUNCON_ALIGNMENT_MAX;
  } else if (guncon_edit_axis == 1) {
    temp_guncon_y = (temp_guncon_y - 1);
    if (temp_guncon_y < GUNCON_ALIGNMENT_MIN) temp_guncon_y = GUNCON_ALIGNMENT_MAX;
  }

  needs_redraw = true;
}
