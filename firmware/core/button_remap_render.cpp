#include "button_remap.h"

void ButtonRemapMenu::render(DeviceEnum mode) {
  if (!needs_redraw) return;
  needs_redraw = false;

  display.clear();
  display.setFont(System5x7);

  renderButtonList(mode);
  renderPadDisplay(mode);
  renderBottomRow();
}

void ButtonRemapMenu::renderButtonList(DeviceEnum mode) {
  uint8_t visible_count = min((uint8_t)LIST_ROWS, button_count);

  for (uint8_t row = 0; row < visible_count; row++) {
    uint8_t btn_index = scroll_offset + row;
    if (btn_index >= button_count) break;

    display.setCursor(0, row);

    bool is_selected = (btn_index == selected_index) && (state == REMAP_NAVIGATE);
    bool is_editing = (btn_index == edit_source_index) && (state == REMAP_EDIT_SOURCE);

    if (is_editing) {
      display.print(F("*"));
    } else if (is_selected) {
      display.print(F(">"));
    } else {
      display.print(F(" "));
    }

    const uint8_t source_slot = getRemapButtonSlot(mode, btn_index);
    const char* src_name = getRemapButtonName(mode, btn_index);
    display.print(src_name);
    display.print(F("->"));

    const uint8_t target_slot = (source_slot != 0xFF) ? temp_remap[source_slot] : btn_index;
    const char* dst_name = getRemapButtonNameForSlot(mode, target_slot);
    display.print(dst_name);

    if (source_slot != 0xFF && target_slot != source_slot) {
      display.print(F("!"));
    }
  }

  if (state == REMAP_EDIT_SOURCE) {
    display.setCursor(0, 6);
    display.print(F("Press btn.."));
  }
}

void ButtonRemapMenu::renderPadDisplay(DeviceEnum mode) {
  display.setFont(System5x7);

  display.setCursor(PAD_START_COL, 0);
  display.print("Remap");

  uint8_t remap_count = 0;
  for (uint8_t i = 0; i < button_count; i++) {
    const uint8_t source_slot = getRemapButtonSlot(mode, i);
    if (source_slot != 0xFF && temp_remap[source_slot] != source_slot) remap_count++;
  }

  display.setCursor(PAD_START_COL, 1);
  display.print(remap_count);
  display.print(" changed");

  if (state == REMAP_EDIT_SOURCE) {
    display.setCursor(PAD_START_COL, 3);
    display.print("From:");
    display.setCursor(PAD_START_COL, 4);
    display.print(getRemapButtonName(mode, edit_source_index));
    display.setCursor(PAD_START_COL, 5);
    display.print("Press");
    display.setCursor(PAD_START_COL, 6);
    display.print("new btn");
  } else if (selected_index < button_count) {
    display.setCursor(PAD_START_COL, 3);
    const char* src = getRemapButtonName(mode, selected_index);
    const uint8_t source_slot = getRemapButtonSlot(mode, selected_index);
    const uint8_t target_slot = (source_slot != 0xFF) ? temp_remap[source_slot] : selected_index;
    const char* dst = getRemapButtonNameForSlot(mode, target_slot);
    display.print(src);
    display.print("->");
    display.print(dst);
  }
}

void ButtonRemapMenu::renderBottomRow() {
  display.setCursor(0, BOTTOM_ROW);

  bool discard_selected = (selected_index == button_count) && (state == REMAP_NAVIGATE);
  if (discard_selected) {
    display.print(F(">"));
  } else {
    display.print(F(" "));
  }
  display.print(F("Cancel"));

  display.setCursor(48, BOTTOM_ROW);
  bool clr_selected = (selected_index == button_count + 1) && (state == REMAP_NAVIGATE);
  if (clr_selected) {
    display.print(F(">"));
  } else {
    display.print(F(" "));
  }
  display.print(F("Clear"));

  display.setCursor(90, BOTTOM_ROW);
  bool save_selected = (selected_index == button_count + 2) && (state == REMAP_NAVIGATE);
  if (save_selected) {
    display.print(F(">"));
  } else {
    display.print(F(" "));
  }
  display.print(F("Save"));
}
