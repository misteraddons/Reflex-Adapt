#include "../product_config.h"

#include <cstring>

#include "menu_helpers.h"
#include "quick_config.h"

namespace {
constexpr uint8_t kQuickConfigTextColumns = 21;
constexpr uint8_t kQuickConfigGlyphWidthPx = 6;
constexpr uint8_t kQuickConfigSettingValueStartColumn = 7;  // "Value: "
constexpr uint16_t kQuickConfigMarqueeTickMs = 250;

uint32_t quickConfigLastMarqueeTickMs = 0;
uint32_t quickConfigMarqueeStartMs = 0;

bool quickConfigMarqueeNeedsTick(QuickConfigState state) {
  return state == QC_MAIN_MENU || state == QC_SETTING_EDIT || state == QC_ANALOG_SUBMENU;
}

uint8_t quickConfigVisibleStartIndex(uint8_t selected, uint8_t count, uint8_t visibleRows) {
  uint8_t start_index = 0;
  if (selected >= 2) {
    start_index = selected - 2;
    if (start_index + visibleRows > count) {
      start_index = (count > visibleRows) ? count - visibleRows : 0;
    }
  }
  return start_index;
}

uint8_t quickConfigValueStartColumn(const char* name, bool submenu) {
  if (submenu) {
    return kQuickConfigTextColumns;
  }

  const uint8_t nameColumns = (uint8_t)strlen(name ? name : "");
  const uint8_t startColumn = (uint8_t)(1 + nameColumns + 2);  // selector + ": "
  return startColumn < kQuickConfigTextColumns ? startColumn : kQuickConfigTextColumns;
}

uint8_t quickConfigColumnPixel(uint8_t column) {
  const uint16_t pixel = (uint16_t)column * kQuickConfigGlyphWidthPx;
  return pixel < 128 ? (uint8_t)pixel : 127;
}

void renderQuickConfigValueMarquee(SSD1306AsciiWire& display, const char* value,
                                   uint8_t startColumn, bool selected) {
  if (startColumn >= kQuickConfigTextColumns) {
    return;
  }
  printOledMarqueeText(display, value ? value : "",
                       (uint8_t)(kQuickConfigTextColumns - startColumn),
                       selected, quickConfigMarqueeStartMs);
}

void clearQuickConfigValueField(SSD1306AsciiWire& display, uint8_t row,
                                const char* name, bool submenu) {
  const uint8_t startColumn = quickConfigValueStartColumn(name, submenu);
  if (startColumn >= kQuickConfigTextColumns) {
    return;
  }

  const uint8_t startPixel = quickConfigColumnPixel(startColumn);
  display.clear(startPixel, 127, row, row);
  display.setCursor(startPixel, row);
}

void renderQuickConfigOptionRow(SSD1306AsciiWire& display, const char* name,
                                const char* value, bool submenu, bool selected) {
  const char* safeName = name ? name : "";

  display.print(selected ? F(">") : F(" "));
  if (submenu) {
    display.print(safeName);
    display.print(F(" >"));
  } else {
    display.print(safeName);
    display.print(F(": "));
    renderQuickConfigValueMarquee(display, value, quickConfigValueStartColumn(safeName, false), selected);
  }

  display.println();
}
}

void QuickConfigMenu::render(SSD1306AsciiWire& display) {
  if (state == QC_RANGE_TEST) {
    renderRangeTest(display);
    return;
  }

  if (state == QC_STICK_CAL) {
    renderStickCal(display);
    return;
  }

  if (state == QC_REMAP_SELECT) {
    remapMenu.render(deviceMode);
    return;
  }

  if (!needs_redraw && quickConfigMarqueeNeedsTick(state)) {
    const uint32_t now = millis();
    if ((uint32_t)(now - quickConfigLastMarqueeTickMs) >= kQuickConfigMarqueeTickMs) {
      quickConfigLastMarqueeTickMs = now;
      if (renderMarqueeTick(display)) {
        return;
      }
    }
  }

  if (!needs_redraw) return;
  needs_redraw = false;
  const uint32_t now = millis();
  quickConfigLastMarqueeTickMs = now;
  quickConfigMarqueeStartMs = now;

  display.clear();
  display.setFont(System5x7);
  display.set1X();

  switch (state) {
    case QC_MAIN_MENU:
      renderMainMenu(display);
      break;

    case QC_CONFIRM_DEFAULT:
      renderDefaultConfirm(display);
      break;

    case QC_TURBO_LIST:
      renderTurboList(display);
      break;

    case QC_TURBO_RATE:
      renderTurboRate(display);
      break;

    case QC_SETTING_EDIT:
      renderSettingEdit(display);
      break;

    case QC_ANALOG_SUBMENU:
      renderAnalogSubmenu(display);
      break;

    case QC_DUAL_MERGE_MAP:
      renderDualMergeMap(display);
      break;

    case QC_RUMBLE_SUBMENU:
      renderRumbleSubmenu(display);
      break;

    case QC_GUNCON_ADJUST:
      renderGunconAdjust(display);
      break;

    case QC_REMAP_LIST:
      renderRemapList(display);
      break;

    default:
      break;
  }
}

bool QuickConfigMenu::renderMarqueeTick(SSD1306AsciiWire& display) {
  display.setFont(System5x7);
  display.set1X();

  switch (state) {
    case QC_MAIN_MENU: {
      if (on_bottom_row || selected_index >= visible_count) {
        return false;
      }

      const uint8_t visible_rows = 5;
      const uint8_t start_index = quickConfigVisibleStartIndex(selected_index, visible_count, visible_rows);
      if (selected_index < start_index || selected_index >= start_index + visible_rows) {
        return false;
      }

      const uint8_t row = (uint8_t)(1 + selected_index - start_index);
      const QuickConfigItem item = visible_items[selected_index];
      clearQuickConfigValueField(display, row, getItemName(item), isSubmenu(item));
      renderQuickConfigValueMarquee(
        display,
        getItemValue(item),
        quickConfigValueStartColumn(getItemName(item), isSubmenu(item)),
        true
      );
      return true;
    }

    case QC_SETTING_EDIT: {
      const uint8_t startPixel = quickConfigColumnPixel(kQuickConfigSettingValueStartColumn);
      display.clear(startPixel, 127, 2, 2);
      display.setCursor(startPixel, 2);
      renderQuickConfigValueMarquee(
        display,
        getItemValue(editing_item),
        kQuickConfigSettingValueStartColumn,
        true
      );
      return true;
    }

    case QC_ANALOG_SUBMENU: {
      buildAnalogItems();
      if (analog_index >= analog_count) {
        return false;
      }

      const uint8_t visible_rows = 6;
      const uint8_t start_index = quickConfigVisibleStartIndex(analog_index, analog_count, visible_rows);
      if (analog_index < start_index || analog_index >= start_index + visible_rows) {
        return false;
      }

      const uint8_t row = (uint8_t)(1 + analog_index - start_index);
      const QuickConfigItem item = analog_items[analog_index];
      clearQuickConfigValueField(display, row, getItemName(item), isSubmenu(item));
      renderQuickConfigValueMarquee(
        display,
        getItemValue(item),
        quickConfigValueStartColumn(getItemName(item), isSubmenu(item)),
        true
      );
      return true;
    }

    default:
      return false;
  }
}

void QuickConfigMenu::renderMainMenu(SSD1306AsciiWire& display) {
  const char* modeName = activeInputAdapterDescriptionOr("Mode");
  char title[32];
  snprintf(title, sizeof(title), "%s Settings", modeName);

  uint16_t title_width = strlen(title) * 6u;
  if (title_width > 128u) {
    title_width = 128u;
  }
  uint8_t center_offset = (uint8_t)((128u - title_width) / 2u);
  display.setCursor(center_offset, 0);
  display.println(title);

  uint8_t visible_rows = 5;
  uint8_t start_index = on_bottom_row ? 0 : quickConfigVisibleStartIndex(selected_index, visible_count, visible_rows);

  for (uint8_t row = 0; row < visible_rows && (start_index + row) < visible_count; row++) {
    uint8_t idx = start_index + row;
    QuickConfigItem item = visible_items[idx];
    bool is_selected = !on_bottom_row && (idx == selected_index);

    renderQuickConfigOptionRow(display, getItemName(item), getItemValue(item),
                               isSubmenu(item), is_selected);
  }

  display.setCursor(0, 6);
  display.clearToEOL();

  display.setCursor(0, 7);
  if (!on_bottom_row) {
    display.print(F("Cancel Default Save"));
  } else if (bottom_index == 0) {
    display.print(F("[Cancel] Default Save"));
  } else if (bottom_index == 1) {
    display.print(F("Cancel [Default] Save"));
  } else {
    display.print(F("Cancel Default [Save]"));
  }
}

void QuickConfigMenu::renderDefaultConfirm(SSD1306AsciiWire& display) {
  display.println(F("Reset mode defaults?"));
  display.println();
  display.println(F("Current mode only"));
  display.println(F("Save still required"));
  display.println();
  display.println(F("This cannot undo"));
  display.println(F("saved settings."));

  display.setCursor(0, 7);
  if (default_confirm_index == 0) {
    display.print(F("[Cancel] Default"));
  } else {
    display.print(F("Cancel [Default]"));
  }
}

void QuickConfigMenu::renderTurboList(SSD1306AsciiWire& display) {
  const uint8_t turboCount = getVisibleTurboCount();

  display.println(F("== Turbo Config =="));

  uint8_t visible_rows = 6;
  uint8_t start_index = 0;

  if (turbo_index < turboCount && turbo_index >= 2) {
    start_index = turbo_index - 2;
    if (start_index + visible_rows > turboCount) {
      start_index = (turboCount > visible_rows) ? turboCount - visible_rows : 0;
    }
  }

  for (uint8_t row = 0; row < visible_rows && (start_index + row) < turboCount; row++) {
    uint8_t item = start_index + row;
    bool is_selected = (item == turbo_index);

    display.print(is_selected ? F(">") : F(" "));

    uint8_t btn_index = getTurboButtonIndexForVisibleIndex(item);
    display.print(getTurboButtonNameForVisibleIndex(item));
    display.print(F(": "));
    display.println((btn_index == 0xFF) ? "?" : getTurboRateName(temp_rates[btn_index]));
  }

  display.setCursor(0, 7);
  display.print((turbo_index == turboCount) ? F(">") : F(" "));
  display.print(F("[Back]"));
}

void QuickConfigMenu::renderTurboRate(SSD1306AsciiWire& display) {
  display.print(F("= "));
  display.print(getTurboButtonNameForVisibleIndex(turbo_index));
  display.println(F(" Turbo ="));

  const uint8_t visible_rows = 6;
  uint8_t start_index = 0;
  if (turbo_rate_selection < TURBO_RATE_LAST && turbo_rate_selection >= 2) {
    start_index = turbo_rate_selection - 2;
    if (start_index + visible_rows > TURBO_RATE_LAST) {
      start_index = (TURBO_RATE_LAST > visible_rows) ? TURBO_RATE_LAST - visible_rows : 0;
    }
  }

  for (uint8_t row = 0; row < visible_rows && (start_index + row) < TURBO_RATE_LAST; row++) {
    uint8_t r = start_index + row;
    display.print(r == turbo_rate_selection ? F(">") : F(" "));
    display.print(r);
    display.print(F(": "));
    display.println(getTurboRateFullName(getTurboRateForMenuIndex(r)));
  }

  display.setCursor(0, 7);
  display.print((turbo_rate_selection >= TURBO_RATE_LAST) ? F(">") : F(" "));
  display.print(F("[Back]"));
}

void QuickConfigMenu::renderSettingEdit(SSD1306AsciiWire& display) {
  char rowText[56];
  snprintf(rowText, sizeof(rowText), "= %s =", getItemName(editing_item));
  printOledMarqueeText(display, rowText, kQuickConfigTextColumns, false);
  display.println();
  display.println();
  display.print(F("Value: "));
  renderQuickConfigValueMarquee(
    display,
    getItemValue(editing_item),
    kQuickConfigSettingValueStartColumn,
    true
  );
  display.println();
  display.println();
  display.println(F("L/R = Cycle value"));
  display.println(F("A = Confirm"));

  display.setCursor(0, 7);
  display.print(F(" [Back]"));
}

void QuickConfigMenu::renderAnalogSubmenu(SSD1306AsciiWire& display) {
  buildAnalogItems();
  if (analog_index > analog_count) {
    analog_index = analog_count;
  }

  display.println(F("== Analog =="));

  uint8_t visible_rows = 6;
  uint8_t start_index = quickConfigVisibleStartIndex(analog_index, analog_count, visible_rows);

  for (uint8_t row = 0; row < visible_rows && (start_index + row) < analog_count; row++) {
    uint8_t idx = start_index + row;
    QuickConfigItem item = analog_items[idx];
    renderQuickConfigOptionRow(display, getItemName(item), getItemValue(item),
                               isSubmenu(item), idx == analog_index);
  }

  display.setCursor(0, 7);
  display.print((analog_index >= analog_count) ? F(">") : F(" "));
  display.print(F("[Back]"));
}

void QuickConfigMenu::renderDualMergeMap(SSD1306AsciiWire& display) {
  const uint8_t buttonCount = getClassicDualMergeButtonCount(temp_input_mode);
  display.println(F("== 2P Merge =="));

  const uint8_t visible_rows = 6;
  uint8_t start_index = 0;
  if (dual_merge_index < getDualMergeMenuCount() && dual_merge_index >= 2) {
    start_index = dual_merge_index - 2;
    if (start_index + visible_rows > getDualMergeMenuCount()) {
      start_index = (getDualMergeMenuCount() > visible_rows)
                      ? (uint8_t)(getDualMergeMenuCount() - visible_rows)
                      : 0;
    }
  }

  for (uint8_t row = 0; row < visible_rows && (start_index + row) < getDualMergeMenuCount(); ++row) {
    const uint8_t item = start_index + row;
    display.print(item == dual_merge_index ? F(">") : F(" "));
    if (item == 0) {
      display.print(F("Enabled: "));
      display.println(temp_classic_dual_merge ? F("On") : F("Off"));
      continue;
    }

    const uint8_t buttonIndex = item - 1;
    const uint32_t mask = getClassicDualMergeButtonMask(temp_input_mode, buttonIndex);
    display.print(getClassicDualMergeButtonName(temp_input_mode, buttonIndex));
    display.print(F(": P"));
    display.println((temp_classic_dual_merge_p2_mask & mask) ? 2 : 1);
  }

  display.setCursor(0, 7);
  display.print((dual_merge_index > buttonCount) ? F(">") : F(" "));
  display.print(F("[Back]"));
}

void QuickConfigMenu::renderRumbleSubmenu(SSD1306AsciiWire& display) {
  const char* levelNames[] = {"Off", "Low", "Med", "High"};
  const uint8_t motors = rumbleMotorCount();
  display.println(F("== Rumble =="));

  for (uint8_t i = 0; i < QCI_RUMBLE_BACK; ++i) {
    RumbleMenuItem item = (RumbleMenuItem)i;
    if (!isRumbleMenuItemVisible(item)) {
      continue;
    }
    display.print(i == rumble_index ? F(">") : F(" "));
    switch (item) {
      case QCI_RUMBLE_LEVEL:
        display.print(F("Level: "));
        display.println(levelNames[temp_rumble & 0x03u]);
        break;
      case QCI_RUMBLE_TEST_HEAVY:
        display.println(F("Test Heavy"));
        break;
      case QCI_RUMBLE_TEST_LIGHT:
        display.println(F("Test Light"));
        break;
      case QCI_RUMBLE_TEST_BOTH:
        display.println(motors <= 1 ? F("Test Motor") : F("Test Both"));
        break;
      default:
        break;
    }
  }

  display.setCursor(0, 7);
  display.print((rumble_index == QCI_RUMBLE_BACK) ? F(">") : F(" "));
  display.print(F("[Back]"));
}

void QuickConfigMenu::renderGunconAdjust(SSD1306AsciiWire& display) {
  display.println(F("== GunCon Alignment =="));
  display.println();

  display.print(guncon_edit_axis == 0 ? F("> ") : F("  "));
  display.print(F("X Alignment: "));
  display.println(temp_guncon_x);

  display.print(guncon_edit_axis == 1 ? F("> ") : F("  "));
  display.print(F("Y Alignment: "));
  display.println(temp_guncon_y);

  display.println();
  display.println(F("L/R = Adjust"));

  display.setCursor(0, 7);
  display.print(guncon_edit_axis == 2 ? F(">") : F(" "));
  display.print(F("[Done]"));
}

bool QuickConfigMenu::isWheelController() {
  #ifdef ENABLE_INPUT_PSX
  if (deviceMode == RZORD_PSX) {
    const char* type = controllerFrameConst(0).controller_type_name;
    if (strncmp(type, "JogCon", 6) == 0 || strcmp(type, "NeGcon") == 0) {
      return true;
    }
  }
  #endif
  #ifdef ENABLE_INPUT_PADDLE
  if (deviceMode == RZORD_PADDLE) {
    return true;
  }
  #endif
  return false;
}

void QuickConfigMenu::renderRangeTest(SSD1306AsciiWire& display) {
  const controller_state_t& frame = controllerFrameConst(0);
  int16_t lx = frame.LX;
  int16_t ly = frame.LY;
  int16_t rx = frame.RX;
  int16_t ry = frame.RY;
  ClassicAnalogRangeSnapshot rangeSnapshot{};
  const bool hasRangeSnapshot = getClassicAnalogRangeSnapshot(deviceMode, 0, rangeSnapshot);
  const bool showRightStick =
    frame.connected ? frame.HAS_ANALOG_STICK_AUX : input_has_right_stick();
  const int16_t raw_lx = (hasRangeSnapshot && rangeSnapshot.valid[CLASSIC_ANALOG_AXIS_LX])
                           ? rangeSnapshot.raw[CLASSIC_ANALOG_AXIS_LX]
                           : lx;
  const int16_t raw_ly = (hasRangeSnapshot && rangeSnapshot.valid[CLASSIC_ANALOG_AXIS_LY])
                           ? rangeSnapshot.raw[CLASSIC_ANALOG_AXIS_LY]
                           : ly;
  const int16_t raw_rx = (hasRangeSnapshot && rangeSnapshot.valid[CLASSIC_ANALOG_AXIS_RX])
                           ? rangeSnapshot.raw[CLASSIC_ANALOG_AXIS_RX]
                           : rx;
  const int16_t raw_ry = (hasRangeSnapshot && rangeSnapshot.valid[CLASSIC_ANALOG_AXIS_RY])
                           ? rangeSnapshot.raw[CLASSIC_ANALOG_AXIS_RY]
                           : ry;
  const int16_t cal_lx = (hasRangeSnapshot && rangeSnapshot.valid[CLASSIC_ANALOG_AXIS_LX])
                           ? rangeSnapshot.calibrated[CLASSIC_ANALOG_AXIS_LX]
                           : lx;
  const int16_t cal_ly = (hasRangeSnapshot && rangeSnapshot.valid[CLASSIC_ANALOG_AXIS_LY])
                           ? rangeSnapshot.calibrated[CLASSIC_ANALOG_AXIS_LY]
                           : ly;
  const int16_t cal_rx = (hasRangeSnapshot && rangeSnapshot.valid[CLASSIC_ANALOG_AXIS_RX])
                           ? rangeSnapshot.calibrated[CLASSIC_ANALOG_AXIS_RX]
                           : rx;
  const int16_t cal_ry = (hasRangeSnapshot && rangeSnapshot.valid[CLASSIC_ANALOG_AXIS_RY])
                           ? rangeSnapshot.calibrated[CLASSIC_ANALOG_AXIS_RY]
                           : ry;

  bool values_changed = (raw_lx != range_last_lx || raw_ly != range_last_ly ||
                         raw_rx != range_last_rx || raw_ry != range_last_ry);
  bool is_wheel = isWheelController();
  #ifdef ENABLE_INPUT_N64
  const bool is_n64_raw_test = deviceMode == RZORD_N64;
  #else
  const bool is_n64_raw_test = false;
  #endif

  if (is_n64_raw_test && hasRangeSnapshot) {
    const int8_t raw_x8 = (int8_t)raw_lx;
    const int8_t raw_y8 = (int8_t)raw_ly;
    if (!range_n64_has_data) {
      range_n64_min_x = raw_x8;
      range_n64_max_x = raw_x8;
      range_n64_min_y = raw_y8;
      range_n64_max_y = raw_y8;
      range_n64_has_data = true;
      values_changed = true;
    } else {
      if (raw_x8 < range_n64_min_x) { range_n64_min_x = raw_x8; values_changed = true; }
      if (raw_x8 > range_n64_max_x) { range_n64_max_x = raw_x8; values_changed = true; }
      if (raw_y8 < range_n64_min_y) { range_n64_min_y = raw_y8; values_changed = true; }
      if (raw_y8 > range_n64_max_y) { range_n64_max_y = raw_y8; values_changed = true; }
    }
  }

  if (is_n64_raw_test) {
    if (needs_redraw || values_changed) {
      display.clear();
      display.setFont(System5x7);
      display.set1X();

      char buf[18];
      display.println(F("= N64 Raw Stick ="));

      display.print(F("X/Y: "));
      snprintf(buf, sizeof(buf), "%4d,%4d", raw_lx, raw_ly);
      display.println(buf);

      display.print(F("-X/+X: "));
      snprintf(buf, sizeof(buf), "%3d/%3d",
               range_n64_min_x < 0 ? -range_n64_min_x : 0,
               range_n64_max_x > 0 ? range_n64_max_x : 0);
      display.println(buf);

      display.print(F("-Y/+Y: "));
      snprintf(buf, sizeof(buf), "%3d/%3d",
               range_n64_min_y < 0 ? -range_n64_min_y : 0,
               range_n64_max_y > 0 ? range_n64_max_y : 0);
      display.println(buf);

      display.println();
      display.println(F("Move stick edges"));

      display.setCursor(0, 7);
      display.print(range_test_exit_selected ? F(">") : F(" "));
      display.print(F("[Back]"));

      needs_redraw = false;
    }

    range_last_lx = raw_lx;
    range_last_ly = raw_ly;
    range_last_rx = raw_rx;
    range_last_ry = raw_ry;
    return;
  }

  if (needs_redraw) {
    display.clear();
    display.setFont(System5x7);
    display.set1X();

    if (is_wheel) {
      const char* type = frame.controller_type_name;
      display.print(F("== "));
      display.print(type);
      display.println(F(" Test =="));
      display.println();

      char buf[16];
      bool is_negcon = (strcmp(type, "NeGcon") == 0);
      #ifdef ENABLE_INPUT_PADDLE
      bool is_paddle = (deviceMode == RZORD_PADDLE);
      #else
      bool is_paddle = false;
      #endif

      if (is_negcon) {
        display.print(F("Twist: "));
        snprintf(buf, sizeof(buf), "%6d", lx);
        display.println(buf);

        display.print(F("I/II:  "));
        snprintf(buf, sizeof(buf), "%3d/%3d", ly, rx);
        display.println(buf);

        display.print(F("L:     "));
        snprintf(buf, sizeof(buf), "%6d", ry);
        display.println(buf);
      } else if (is_paddle) {
        display.print(F("Pdl A: "));
        snprintf(buf, sizeof(buf), "%6d", lx);
        display.println(buf);

        display.print(F("Pdl B: "));
        snprintf(buf, sizeof(buf), "%6d", ly);
        display.println(buf);

        display.println();
      } else {
        display.print(F("Wheel: "));
        snprintf(buf, sizeof(buf), "%6d", lx);
        display.println(buf);

        display.println();
        display.println();
      }
    } else {
      display.println(F("== Analog Test =="));
      display.println();

      char buf[16];
      display.print(F("Raw Left: "));
      snprintf(buf, sizeof(buf), "%4d,%4d", raw_lx, raw_ly);
      display.println(buf);

      display.print(F("Cal Left: "));
      snprintf(buf, sizeof(buf), "%4d,%4d", cal_lx, cal_ly);
      display.println(buf);

      if (showRightStick) {
        display.print(F("Raw Right:"));
        snprintf(buf, sizeof(buf), "%4d,%4d", raw_rx, raw_ry);
        display.println(buf);

        display.print(F("Cal Right:"));
        snprintf(buf, sizeof(buf), "%4d,%4d", cal_rx, cal_ry);
        display.println(buf);
      }
    }

    display.setCursor(0, 7);
    display.print(range_test_exit_selected ? F(">") : F(" "));
    display.print(F("[Back]"));

    needs_redraw = false;
  } else if (values_changed) {
    display.setFont(System5x7);
    display.set1X();

    char buf[16];
    if (is_wheel) {
      const char* type = frame.controller_type_name;
      bool is_negcon = (strcmp(type, "NeGcon") == 0);
      #ifdef ENABLE_INPUT_PADDLE
      bool is_paddle = (deviceMode == RZORD_PADDLE);
      #else
      bool is_paddle = false;
      #endif

      if (is_negcon) {
        display.setCursor(42, 2);
        snprintf(buf, sizeof(buf), "%6d", lx);
        display.print(buf);

        display.setCursor(42, 3);
        snprintf(buf, sizeof(buf), "%3d/%3d", ly, rx);
        display.print(buf);

        display.setCursor(42, 4);
        snprintf(buf, sizeof(buf), "%6d", ry);
        display.print(buf);
      } else if (is_paddle) {
        display.setCursor(42, 2);
        snprintf(buf, sizeof(buf), "%6d", lx);
        display.print(buf);

        display.setCursor(42, 3);
        snprintf(buf, sizeof(buf), "%6d", ly);
        display.print(buf);
      } else {
        display.setCursor(42, 2);
        snprintf(buf, sizeof(buf), "%6d", lx);
        display.print(buf);
      }
    } else {
      display.setCursor(60, 2);
      snprintf(buf, sizeof(buf), "%4d,%4d", raw_lx, raw_ly);
      display.print(buf);

      display.setCursor(60, 3);
      snprintf(buf, sizeof(buf), "%4d,%4d", cal_lx, cal_ly);
      display.print(buf);

      if (showRightStick) {
        display.setCursor(60, 4);
        snprintf(buf, sizeof(buf), "%4d,%4d", raw_rx, raw_ry);
        display.print(buf);

        display.setCursor(60, 5);
        snprintf(buf, sizeof(buf), "%4d,%4d", cal_rx, cal_ry);
        display.print(buf);
      }
    }
  }

  range_last_lx = raw_lx;
  range_last_ly = raw_ly;
  range_last_rx = raw_rx;
  range_last_ry = raw_ry;
}

void QuickConfigMenu::renderStickCal(SSD1306AsciiWire& display) {
  const controller_state_t& frame = controllerFrameConst(0);
  int8_t lx = (int8_t)frame.LX;
  int8_t ly = (int8_t)frame.LY;
  int8_t rx = (int8_t)frame.RX;
  int8_t ry = (int8_t)frame.RY;

  uint8_t stick_count = getStickCount(deviceMode);

  if (lx < cal_lx_min) { cal_lx_min = lx; cal_has_data = true; }
  if (lx > cal_lx_max) { cal_lx_max = lx; cal_has_data = true; }
  if (ly < cal_ly_min) { cal_ly_min = ly; cal_has_data = true; }
  if (ly > cal_ly_max) { cal_ly_max = ly; cal_has_data = true; }
  if (stick_count >= 2) {
    if (rx < cal_rx_min) { cal_rx_min = rx; cal_has_data = true; }
    if (rx > cal_rx_max) { cal_rx_max = rx; cal_has_data = true; }
    if (ry < cal_ry_min) { cal_ry_min = ry; cal_has_data = true; }
    if (ry > cal_ry_max) { cal_ry_max = ry; cal_has_data = true; }
  }

  if (needs_redraw) {
    display.clear();
    display.setFont(System5x7);
    display.set1X();
    display.println(F("= Calibrate Stick ="));
    display.println(F("Move sticks to edges"));
    needs_redraw = false;
  }

  char buf[22];
  display.setCursor(0, 2);
  snprintf(buf, sizeof(buf), "L:%4d/%4d %4d/%4d", cal_lx_min, cal_lx_max, cal_ly_min, cal_ly_max);
  display.print(buf);

  display.setCursor(0, 3);
  if (stick_count >= 2) {
    snprintf(buf, sizeof(buf), "R:%4d/%4d %4d/%4d", cal_rx_min, cal_rx_max, cal_ry_min, cal_ry_max);
    display.print(buf);
  } else {
    display.print(F("                    "));
  }

  display.setCursor(0, 7);
  display.print(cal_selection == 0 ? F(">") : F(" "));
  display.print(F("Cancel"));

  display.setCursor(48, 7);
  display.print(cal_selection == 1 ? F(">") : F(" "));
  display.print(F("Save"));

  display.setCursor(84, 7);
  display.print(cal_selection == 2 ? F(">") : F(" "));
  display.print(F("Clear"));
}

void QuickConfigMenu::renderRemapList(SSD1306AsciiWire& display) {
  display.println(F("== Button Remap =="));

  uint8_t btn_count = getRemapButtonCount(deviceMode);
  uint8_t visible_rows = 6;
  uint8_t start_index = 0;

  if (remap_index < btn_count && remap_index >= 2) {
    start_index = remap_index - 2;
    if (start_index + visible_rows > btn_count) {
      start_index = (btn_count > visible_rows) ? btn_count - visible_rows : 0;
    }
  }

  for (uint8_t row = 0; row < visible_rows && (start_index + row) < btn_count; row++) {
    uint8_t item = start_index + row;
    bool is_selected = (item == remap_index);

    display.print(is_selected ? F(">") : F(" "));

    const uint8_t source_slot = getRemapButtonSlot(deviceMode, item);
    const char* name = getRemapButtonName(deviceMode, item);
    display.print(name);
    display.print(F("->"));
    const uint8_t target_slot = (source_slot != 0xFF) ? active_remaps[source_slot] : item;
    display.print(getRemapButtonNameForSlot(deviceMode, target_slot));
    if (source_slot != 0xFF && active_remaps[source_slot] != source_slot) {
      display.print(F("!"));
    }
    display.println();
  }

  display.setCursor(0, 7);
  display.print((remap_index == btn_count) ? F(">") : F(" "));
  display.print(F("[Back]"));
}
