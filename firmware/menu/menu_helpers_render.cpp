#include "../product_config.h"

#include <cstring>

#include "menu_helpers.h"

#include "../core/device_runtime_state.h"
#include "../platform/display_runtime_state.h"
#include "menu_catalog.h"
#include "menu_mode_state.h"

#ifdef USE_I2C_DISPLAY
constexpr uint8_t kOledTextColumns = 21;
constexpr uint16_t kOledMarqueeStepMs = 250;
constexpr uint8_t kOledMarqueeInitialHoldSteps = 6;

void printOledMarqueeText(SSD1306AsciiWire& target, const char* text,
                          uint8_t max_chars, bool selected,
                          uint32_t marquee_start_ms) {
  if (!text || max_chars == 0) {
    return;
  }
  if (max_chars > kOledTextColumns) {
    max_chars = kOledTextColumns;
  }

  const size_t len = strlen(text);
  if (len <= max_chars) {
    target.print(text);
    return;
  }

  uint8_t offset = 0;
  uint8_t chars_to_print = max_chars;
  if (selected) {
    const uint8_t overflow = (uint8_t)(len - max_chars);
    const uint8_t hold_steps = kOledMarqueeInitialHoldSteps;
    const uint8_t period = (uint8_t)(overflow + 1 + hold_steps * 2);
    const uint32_t now = millis();
    const uint32_t elapsed_ms = marquee_start_ms
      ? (uint32_t)(now - marquee_start_ms)
      : now;
    const uint8_t phase = (uint8_t)((elapsed_ms / kOledMarqueeStepMs) % period);
    if (phase < hold_steps) {
      offset = 0;
    } else if (phase < hold_steps + overflow) {
      offset = phase - hold_steps;
    } else {
      offset = overflow;
    }
  } else if (max_chars > 1) {
    chars_to_print = max_chars - 1;
  }

  for (uint8_t i = 0; i < chars_to_print && text[offset + i]; ++i) {
    target.print(text[offset + i]);
  }
  if (!selected && max_chars > 1) {
    target.print('>');
  }
}
#else
void printOledMarqueeText(SSD1306AsciiWire& target, const char* text,
                          uint8_t max_chars, bool selected,
                          uint32_t marquee_start_ms) {
  (void)target;
  (void)text;
  (void)max_chars;
  (void)selected;
  (void)marquee_start_ms;
}
#endif

void clearRow(uint8_t row) {
#ifdef USE_I2C_DISPLAY
  display.clear(0, 127, row, row);
#else
  (void)row;
#endif
}

void printMenuItem(uint8_t actual_index, uint8_t row, bool selected) {
#ifndef USE_I2C_DISPLAY
  (void)actual_index;
  (void)row;
  (void)selected;
  return;
#else
  display.setRow(row);
  display.setCol(0);
  clearRow(row);

  if (selected)
    display.print('>');
  else
    display.print(' ');

  const menu_line_t* item = &menu_items[actual_index];

  switch (item->item) {
    case menu_item_exit:
    case menu_item_save_and_reboot:
      display.print(item->text);
      break;
    case menu_item_screensaver:
    case menu_item_games:
    case menu_item_buzzer:
    case menu_item_hotkeys:
    case menu_item_kiosk_mode:
      display.print(item->text);
      display.print(" >");
      break;
    default:
      display.print(item->text);
      display.print(": ");
      if (menu_input != deviceMode && is_mode_specific_setting(item->item)) {
        display.print("---");
      } else {
        handleMenuItemAction(item->item, item_action_display);
      }
      break;
  }
#endif
}

void renderBottomRow(bool on_bottom_row, bool right_selected) {
#ifndef USE_I2C_DISPLAY
  (void)on_bottom_row;
  (void)right_selected;
  return;
#else
  display.setRow(7);
  display.setCol(0);
  clearRow(7);

  if (on_bottom_row && !right_selected) {
    display.print(F("[Cancel]"));
  } else {
    display.print(F(" Cancel "));
  }

  display.setCol(78);
  if (on_bottom_row && right_selected) {
    display.print(F("[Save]"));
  } else {
    display.print(F(" Save "));
  }
#endif
}

void renderScrollIndicator(uint8_t scroll_offset) {
#ifndef USE_I2C_DISPLAY
  (void)scroll_offset;
  return;
#else
  display.setRow(0);
  display.setCol(120);
  if (scroll_offset > 0)
    display.print('^');
  else
    display.print(' ');
#endif
}

void renderMenu(uint8_t selected_visible, uint8_t scroll_offset, bool bottom_right_selected) {
#ifndef USE_I2C_DISPLAY
  (void)selected_visible;
  (void)scroll_offset;
  (void)bottom_right_selected;
  return;
#else
  display.setFont(System5x7);
  display.set1X();

  uint8_t visible_count = getVisibleItemCount();
  bool on_bottom_row = (selected_visible >= visible_count);

  display.setRow(0);
  display.setCol(0);
  clearRow(0);
  display.print("   -- Settings --");

  for (uint8_t row = 0; row < MENU_ITEM_ROWS; ++row) {
    uint8_t visible_idx = scroll_offset + row;
    if (visible_idx < visible_count) {
      uint8_t actual_idx = getActualIndex(visible_idx);
      printMenuItem(actual_idx, row + 1, visible_idx == selected_visible);
    } else {
      clearRow(row + 1);
    }
  }

  renderScrollIndicator(scroll_offset);
  renderBottomRow(on_bottom_row, bottom_right_selected);
#endif
}
