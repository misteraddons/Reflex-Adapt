#include "../product_config.h"

#include "quick_config.h"

#include <string.h>

#include "../input/runtime/input_frame_runtime.h"

static bool quickConfigIsLiveNeGcon(DeviceEnum mode, bool liveSelection) {
  return mode == RZORD_PSX &&
         liveSelection &&
         strcmp(controllerFrameConst(0).controller_type_name, "NeGcon") == 0;
}

void QuickConfigMenu::buildAnalogItems() {
  analog_count = 0;
  DeviceEnum mode = temp_input_mode;

  if (hasAnalogSticks(mode) && isLiveInputSelection()) {
    addAnalogItem(QCI_RANGE_TEST);
    if (!isN64(mode)) {
      addAnalogItem(QCI_STICK_CAL);
    }
  }

  if (hasClassicAnalogRange(mode) && hasAnalogSticks(mode)) {
    addAnalogItem(QCI_N64_RANGE);
  }

  if (hasAnalogSticks(mode)) {
    addAnalogItem(QCI_DEADZONE);
    addAnalogItem(QCI_STICK_INVERT);
  }

}

void QuickConfigMenu::buildVisibleItems() {
  visible_count = 0;
  buildAnalogItems();
  DeviceEnum mode = temp_input_mode;

  addVisibleItem(QCI_INPUT_MODE);
  addVisibleItem(QCI_OUTPUT_MODE);

  if (!canEditModeSpecificSettings()) {
    return;
  }

  if (hasRumble(mode)) {
    addVisibleItem(QCI_RUMBLE);
  }

  addVisibleItem(QCI_TURBO);
  if (isLiveInputSelection()) {
    addVisibleItem(QCI_REMAP);
  }

  if (isNintendoController(mode) && !isNsoSpecialActiveForSelection(mode)) {
    addVisibleItem(QCI_BTN_MAP);
  }

  if (isN64(mode) || isGameCube(mode)) {
    addVisibleItem(QCI_N64_Z);
  }
  if (isN64(mode)) {
    if (!isNsoSpecialActiveForSelection(mode)) {
      addVisibleItem(QCI_N64_CSTICK);
    }
  }

  #ifdef ENABLE_INPUT_VBOY
  if (mode == RZORD_VBOY) {
    addVisibleItem(QCI_N64_CSTICK);
  }
  #endif

  if (temp_output_mode == OUTPUT_SWITCHPRO &&
      input_mode_supports_nso_special(mode)) {
    addVisibleItem(QCI_NSO_SPECIAL);
  }

  #ifdef ENABLE_INPUT_NES
  if (mode == RZORD_NES) {
    addVisibleItem(QCI_POWERPAD);
  }
  #endif

  if (analog_count > 0) {
    addVisibleItem(QCI_ANALOG_MENU);
  }

  if (hasAnalogTriggers(mode) && temp_output_mode != OUTPUT_SWITCHPRO) {
    addVisibleItem(QCI_TRIGGERS);
  }
  if (supportsTempTriggerRightStick()) {
    addVisibleItem(QCI_SWITCH_RTRIG_STICK);
  }

  addVisibleItem(QCI_DPAD_BUTTONS);
  #ifndef PRODUCT_CLASSIC2USB
  addVisibleItem(QCI_SOCD);
  #endif

#ifdef PRODUCT_CLASSIC2USB
  const bool neGconLiveSelection = quickConfigIsLiveNeGcon(mode, isLiveInputSelection());
  if (!neGconLiveSelection) {
    addVisibleItem(QCI_CLASSIC_DUAL_MERGE);
  }
#endif

  if (isGuncon(mode)) {
    addVisibleItem(QCI_GUNCON_OFFSET);
  }

  #ifdef ENABLE_INPUT_PSX
  if (mode == RZORD_PSX &&
      strncmp(controllerFrameConst(0).controller_type_name, "JogCon", 6) == 0) {
    addVisibleItem(QCI_WHEEL_SENS);
    addVisibleItem(QCI_JOGCON_DIGITAL);
    addVisibleItem(QCI_JOGCON_WHEEL_AXIS);
  }
  #endif

  if (hasSpinner(mode)) {
    addVisibleItem(QCI_SPINNER_SPEED);
  }
}

uint8_t quickConfigItemSettingIds(QuickConfigItem item, DeviceEnum mode,
                                  SettingId* ids, uint8_t maxIds) {
  uint8_t count = 0;
  auto add = [&](SettingId id) {
    if (ids != nullptr && count < maxIds) {
      ids[count] = id;
    }
    ++count;
  };

  switch (item) {
    case QCI_INPUT_MODE: add(SettingId::PersistedInputMode); break;
    case QCI_OUTPUT_MODE: add(SettingId::ConfiguredOutputMode); break;
    case QCI_WIN_OUTPUT: add(SettingId::WinOutput); break;
    case QCI_DEADZONE: add(SettingId::Deadzone); break;
    case QCI_DPAD_BUTTONS: add(SettingId::DpadMode); break;
    case QCI_SOCD: add(SettingId::Socd); break;
    case QCI_STICK_INVERT: add(SettingId::StickInvert); break;
    case QCI_RUMBLETECH: add(SettingId::SnesRumbleTech); break;
    case QCI_RUMBLE: add(SettingId::RumbleLevel); break;
    case QCI_TRIGGERS:
    case QCI_SWITCH_RTRIG_STICK:
      add(SettingId::TriggerMode);
      break;
    case QCI_BTN_MAP: add(SettingId::ButtonMapMode); break;
    case QCI_N64_Z: add(SettingId::N64ZMode); break;
    case QCI_N64_CSTICK: add(SettingId::N64CStickMode); break;
    case QCI_N64_RANGE:
      #ifdef ENABLE_INPUT_N64
      if (mode == RZORD_N64) {
        add(SettingId::N64AnalogRange);
      } else
      #endif
      {
        add(SettingId::WiiAnalogRange);
      }
      break;
    case QCI_NSO_SPECIAL: add(SettingId::NsoSpecial); break;
    case QCI_POWERPAD: add(SettingId::PowerpadMode); break;
    case QCI_CLASSIC_DUAL_MERGE: add(SettingId::ClassicDualMerge); break;
    case QCI_GUNCON_OFFSET:
      add(SettingId::GunconOffsetX);
      add(SettingId::GunconOffsetY);
      break;
    case QCI_WHEEL_SENS: add(SettingId::JogconRange); break;
    case QCI_JOGCON_DIGITAL: add(SettingId::JogconDigitalMap); break;
    case QCI_JOGCON_WHEEL_AXIS: add(SettingId::JogconWheelAxis); break;
    case QCI_SPINNER_SPEED: add(SettingId::SpinnerSpeed); break;
    case QCI_STICK_CAL:
      add(SettingId::AnalogCalLXMin);
      add(SettingId::AnalogCalLXMax);
      add(SettingId::AnalogCalLYMin);
      add(SettingId::AnalogCalLYMax);
      add(SettingId::AnalogCalRXMin);
      add(SettingId::AnalogCalRXMax);
      add(SettingId::AnalogCalRYMin);
      add(SettingId::AnalogCalRYMax);
      add(SettingId::AnalogCalEnabled);
      break;
    default:
      break;
  }

  return count;
}

uint8_t QuickConfigMenu::debugBuildVisibleItems(DeviceEnum inputMode, outputMode_t outputMode,
                                                uint8_t winOutput, QuickConfigItem* items,
                                                uint8_t maxItems) {
  state = QC_MAIN_MENU;
  selected_index = 0;
  on_bottom_row = false;
  bottom_right = false;
  bottom_index = 0;
  temp_input_mode = inputMode;
  temp_output_mode = canonicalizeOutputMode(outputMode);
  temp_win_output = (winOutput > 2) ? 0 : winOutput;

  if (isConcreteModeForPerSettings(temp_input_mode)) {
    loadTempQuickSettingsForMode(temp_input_mode);
  } else {
    temp_deadzone = 0;
    temp_socd = SOCD_OFF;
    temp_dpad_mode = 0;
    temp_stick_invert = 0;
    temp_snes_rumbletech = snes_rumbletech_enabled ? 1 : 0;
    temp_rumble = 3;
    temp_triggers = TRIGGER_MODE_ANALOG;
    temp_spinner = defaultSpinnerSpeedForInputMode(temp_input_mode);
    temp_btn_map = defaultButtonMapModeForInputMode(temp_input_mode);
    temp_n64_z = defaultZButtonModeForInputMode(temp_input_mode);
    temp_n64_cstick = 0;
    temp_n64_range = CLASSIC_ANALOG_RANGE_DEFAULT;
    temp_wii_range = CLASSIC_ANALOG_RANGE_DEFAULT;
    temp_nso_special = 0;
    temp_powerpad = 0;
    temp_classic_dual_merge = menu_classic_dual_merge;
  }

  buildVisibleItems();

  uint8_t count = 0;
  for (uint8_t i = 0; i < visible_count && count < maxItems; ++i) {
    items[count++] = visible_items[i];
  }

  if (analog_count > 0) {
    for (uint8_t i = 0; i < analog_count && count < maxItems; ++i) {
      items[count++] = analog_items[i];
    }
  }

  return count;
}

const char* QuickConfigMenu::getItemName(QuickConfigItem item) {
  switch (item) {
    case QCI_INPUT_MODE: return "Input";
    case QCI_OUTPUT_MODE: return "Output";
    case QCI_WIN_OUTPUT: return "Win Mode";
    case QCI_ANALOG_MENU: return "Analog";
    case QCI_DEADZONE: return "Deadzone";
    case QCI_RANGE_TEST: return isN64(temp_input_mode) ? "Raw Stick" : "Range Test";
    case QCI_DPAD_BUTTONS: return "DPad";
    case QCI_SOCD: return "SOCD";
    case QCI_STICK_INVERT: return "Stick Inv";
    case QCI_RUMBLETECH: return "RumbleTech";
    case QCI_RUMBLE: return "Rumble";
    case QCI_TRIGGERS: return "Triggers";
    case QCI_SWITCH_RTRIG_STICK: return "RTrig=RStick";
    case QCI_REMAP: return "Button Remap";
    case QCI_BTN_MAP: return "Button Map";
    case QCI_N64_Z:
      return isGameCube(temp_input_mode) ? "GC Z Btn" : "N64 Z Btn";
    case QCI_N64_CSTICK:
      #ifdef ENABLE_INPUT_VBOY
      if (temp_input_mode == RZORD_VBOY) return "Right DPad";
      #endif
      return "N64 C Buttons";
    case QCI_N64_RANGE: return "Stick Range";
    case QCI_NSO_SPECIAL: return "NSO Mode";
    case QCI_POWERPAD: return "Power Pad";
    case QCI_CLASSIC_DUAL_MERGE: return "2P Merge";
    case QCI_GUNCON_OFFSET: return "GunCon Align";
    case QCI_TURBO: return "Turbo";
    case QCI_WHEEL_SENS: return "Wheel Sens";
    case QCI_JOGCON_DIGITAL: return "Dig Output";
    case QCI_JOGCON_WHEEL_AXIS: return "Wheel Axis";
    case QCI_SPINNER_SPEED: return "Spin Speed";
    case QCI_STICK_CAL: return "Stick Cal";
    case QCI_DISCARD_EXIT: return "[Cancel]";
    case QCI_EXIT: return "[Save]";
    default: return "?";
  }
}

const char* QuickConfigMenu::getItemValue(QuickConfigItem item) {
  static char buf[16];
  switch (item) {
    case QCI_INPUT_MODE:
      return getInputModeName(temp_input_mode);
    case QCI_OUTPUT_MODE:
      return get_mode_name(temp_output_mode);
    case QCI_WIN_OUTPUT:
      switch (temp_win_output) {
        case 1: return "XInput";
        case 2: return "Keyboard";
        default: return "DInput";
      }
    case QCI_DEADZONE:
      snprintf(buf, sizeof(buf), "%d%%", temp_deadzone * 5);
      return buf;
    case QCI_RANGE_TEST:
      return ">";
    case QCI_DPAD_BUTTONS: {
      static const char* const dpad_mode_names[] = { "DPad", "Left Stick", "Right Stick", "Buttons" };
      const uint8_t idx = isTempDpadModeAllowed(temp_dpad_mode)
                             ? temp_dpad_mode
                             : DPAD_MODE_DPAD;
      return dpad_mode_names[idx];
    }
    case QCI_SOCD:
      switch (temp_socd) {
        case SOCD_OFF: return "Off";
        case SOCD_NEUTRAL: return "Neutral";
        case SOCD_SECOND: return "2nd Win";
        case SOCD_FIRST: return "1st Win";
        default: return "?";
      }
    case QCI_STICK_INVERT:
      if (temp_stick_invert > getStickInvertMax(temp_input_mode)) return "Off";
      switch (temp_stick_invert) {
        case 0: return "Off";
        case 1: return quickConfigIsLiveNeGcon(temp_input_mode, isLiveInputSelection()) ? "Twist" : "Left";
        case 2: return "Right";
        case 3: return "Both";
        default: return "?";
      }
    case QCI_RUMBLETECH:
      return temp_snes_rumbletech ? "Yes" : "No";
    case QCI_RUMBLE:
      switch (temp_rumble) {
        case 0: return "Off";
        case 1: return "Low";
        case 2: return "Med";
        case 3: return "High";
        default: return "?";
      }
    case QCI_TRIGGERS:
      switch (temp_triggers) {
        case TRIGGER_MODE_DIGITAL: return "Digital";
        case TRIGGER_MODE_BOTH: return "Both";
        default: return "Analog";
      }
    case QCI_SWITCH_RTRIG_STICK:
      return temp_triggers == TRIGGER_MODE_RSTICK ? "On" : "Off";
    case QCI_REMAP:
    case QCI_ANALOG_MENU:
      return ">";
    case QCI_BTN_MAP:
      return temp_btn_map ? "Position" : "Name";
    case QCI_N64_Z:
      return isGameCube(temp_input_mode)
               ? (temp_n64_z ? "Back" : "R1")
               : (temp_n64_z ? "L2" : "L1");
    case QCI_N64_CSTICK:
      switch (temp_n64_cstick) {
        case 1: return "Buttons";
        case 2: return "Stick";
        default: return "Auto";
      }
    case QCI_N64_RANGE: {
      static const char* const range_labels[] = { "Raw", "Norm", "Cal", "Learn" };
      return range_labels[sanitizeClassicAnalogRange(getTempClassicAnalogRange())];
    }
    case QCI_NSO_SPECIAL:
      return temp_nso_special ? "On" : "Off";
    case QCI_POWERPAD:
      return temp_powerpad ? "On" : "Off";
    case QCI_CLASSIC_DUAL_MERGE:
      return ">";
    case QCI_GUNCON_OFFSET:
      snprintf(buf, sizeof(buf), "%+d,%+d", temp_guncon_x, temp_guncon_y);
      return buf;
    case QCI_TURBO:
      return ">";
    case QCI_WHEEL_SENS: {
      static const char* sens_labels[] = { "Fine", "Normal", "Coarse" };
      return sens_labels[temp_wheel_sens];
    }
    case QCI_JOGCON_DIGITAL: {
      static const char* dig_labels[] = { "L3/R3", "Left/Right", "L/R", "Up/Down" };
      return dig_labels[temp_jogcon_digital];
    }
    case QCI_JOGCON_WHEEL_AXIS:
      return temp_jogcon_wheel_axis ? "Y" : "X";
    case QCI_SPINNER_SPEED:
      return spinner_speed_labels[temp_spinner];
    case QCI_STICK_CAL:
      return ">";
    default:
      return "";
  }
}

bool QuickConfigMenu::isDirectlyEditable(QuickConfigItem item) {
  switch (item) {
    case QCI_INPUT_MODE:
    case QCI_OUTPUT_MODE:
    case QCI_WIN_OUTPUT:
    case QCI_DEADZONE:
    case QCI_DPAD_BUTTONS:
    case QCI_SOCD:
    case QCI_STICK_INVERT:
    case QCI_RUMBLETECH:
    case QCI_TRIGGERS:
    case QCI_SWITCH_RTRIG_STICK:
    case QCI_SPINNER_SPEED:
    case QCI_WHEEL_SENS:
    case QCI_JOGCON_DIGITAL:
    case QCI_JOGCON_WHEEL_AXIS:
    case QCI_BTN_MAP:
    case QCI_N64_Z:
    case QCI_N64_CSTICK:
    case QCI_N64_RANGE:
    case QCI_NSO_SPECIAL:
    case QCI_POWERPAD:
      return true;
    default:
      return false;
  }
}

bool QuickConfigMenu::needsSubmenu(QuickConfigItem item) {
  switch (item) {
    case QCI_TURBO:
    case QCI_REMAP:
    case QCI_ANALOG_MENU:
    case QCI_RANGE_TEST:
    case QCI_STICK_CAL:
    case QCI_GUNCON_OFFSET:
    case QCI_CLASSIC_DUAL_MERGE:
    case QCI_RUMBLE:
      return true;
    default:
      return false;
  }
}

void QuickConfigMenu::enterSubmenu(QuickConfigItem item) {
  return_to_analog_submenu = false;

  switch (item) {
    case QCI_TURBO:
      state = QC_TURBO_LIST;
      turbo_index = 0;
      break;
    case QCI_REMAP:
      state = QC_REMAP_LIST;
      remap_index = 0;
      break;
    case QCI_ANALOG_MENU:
      buildAnalogItems();
      analog_index = 0;
      state = QC_ANALOG_SUBMENU;
      break;
    case QCI_RANGE_TEST:
      state = QC_RANGE_TEST;
      range_test_exit_selected = true;
      range_n64_min_x = 0;
      range_n64_max_x = 0;
      range_n64_min_y = 0;
      range_n64_max_y = 0;
      range_n64_has_data = false;
      needs_redraw = true;
      break;
    case QCI_STICK_CAL:
      state = QC_STICK_CAL;
      cal_lx_min = 0;
      cal_lx_max = 0;
      cal_ly_min = 0;
      cal_ly_max = 0;
      cal_rx_min = 0;
      cal_rx_max = 0;
      cal_ry_min = 0;
      cal_ry_max = 0;
      cal_selection = 0;
      cal_has_data = false;
      needs_redraw = true;
      break;
    case QCI_GUNCON_OFFSET:
      state = QC_GUNCON_ADJUST;
      guncon_edit_axis = 0;
      break;
    case QCI_CLASSIC_DUAL_MERGE:
      state = QC_DUAL_MERGE_MAP;
      dual_merge_index = 0;
      break;
    case QCI_RUMBLE:
      state = QC_RUMBLE_SUBMENU;
      rumble_index = 0;
      break;
    default:
      break;
  }
}
