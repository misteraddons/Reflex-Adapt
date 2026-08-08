#include "button_remap.h"

ButtonRemapMenu::ButtonRemapMenu()
  : state(REMAP_CLOSED),
    selected_index(0),
    scroll_offset(0),
    button_count(8),
    needs_redraw(true),
    edit_source_index(0),
    active_mode(RZORD_NONE) {
  for (uint8_t i = 0; i < REMAP_MAX_BUTTONS; i++) {
    temp_remap[i] = i;
  }
}

bool ButtonRemapMenu::isOpen() {
  return state != REMAP_CLOSED;
}

bool ButtonRemapMenu::isEditMode() {
  return state == REMAP_EDIT_SOURCE;
}

void ButtonRemapMenu::open(DeviceEnum mode) {
  state = REMAP_NAVIGATE;
  selected_index = 0;
  scroll_offset = 0;
  active_mode = mode;
  button_count = getRemapButtonCount(mode);
  needs_redraw = true;

  for (uint8_t i = 0; i < REMAP_MAX_BUTTONS; i++) {
    temp_remap[i] = active_remaps[i];
  }
}

void ButtonRemapMenu::close() {
  state = REMAP_CLOSED;
  needs_redraw = true;
}

void ButtonRemapMenu::loadRemaps(const uint8_t* data) {
  for (uint8_t i = 0; i < REMAP_MAX_BUTTONS; i++) {
    temp_remap[i] = data[i] < REMAP_MAX_BUTTONS ? data[i] : i;
  }
  needs_redraw = true;
}

void ButtonRemapMenu::getRemaps(uint8_t* data) {
  for (uint8_t i = 0; i < REMAP_MAX_BUTTONS; i++) {
    data[i] = temp_remap[i];
  }
}

void ButtonRemapMenu::clearRemaps() {
  for (uint8_t i = 0; i < REMAP_MAX_BUTTONS; i++) {
    temp_remap[i] = i;
  }
  needs_redraw = true;
}

void ButtonRemapMenu::navigate() {
  if (state == REMAP_EDIT_SOURCE) {
    state = REMAP_NAVIGATE;
    needs_redraw = true;
    return;
  }

  needs_redraw = true;
  selected_index++;

  uint8_t total_items = button_count + 3;
  if (selected_index >= total_items) {
    selected_index = 0;
  }

  if (selected_index < button_count) {
    const uint8_t scroll_margin = 2;
    if (selected_index < scroll_offset + scroll_margin && scroll_offset > 0) {
      scroll_offset = (selected_index > scroll_margin) ? selected_index - scroll_margin : 0;
    } else if (selected_index >= scroll_offset + LIST_ROWS - scroll_margin) {
      uint8_t new_offset = selected_index - LIST_ROWS + scroll_margin + 1;
      uint8_t max_offset = (button_count > LIST_ROWS) ? button_count - LIST_ROWS : 0;
      scroll_offset = (new_offset < max_offset) ? new_offset : max_offset;
    }
  }
}

uint8_t ButtonRemapMenu::select() {
  needs_redraw = true;

  if (state == REMAP_EDIT_SOURCE) {
    state = REMAP_NAVIGATE;
    return 0;
  }

  if (selected_index < button_count) {
    edit_source_index = selected_index;
    state = REMAP_EDIT_SOURCE;
    return 0;
  } else if (selected_index == button_count) {
    return 3;
  } else if (selected_index == button_count + 1) {
    clearRemaps();
    for (uint8_t i = 0; i < REMAP_MAX_BUTTONS; i++) {
      active_remaps[i] = temp_remap[i];
    }
    return 2;
  } else {
    for (uint8_t i = 0; i < REMAP_MAX_BUTTONS; i++) {
      active_remaps[i] = temp_remap[i];
    }
    return 1;
  }
}

void ButtonRemapMenu::setDestination(uint8_t button_index) {
  if (state != REMAP_EDIT_SOURCE) return;
  if (getRemapButtonDisplayIndex(active_mode, button_index) == 0xFF) return;

  const uint8_t source_slot = getRemapButtonSlot(active_mode, edit_source_index);
  if (source_slot == 0xFF) return;

  temp_remap[source_slot] = button_index;
  state = REMAP_NAVIGATE;
  needs_redraw = true;
}

uint8_t ButtonRemapMenu::getFirstPressedButton() {
  if (MAX_USB_OUT == 0) return 0xFF;
  const controller_state_t& r = controllerFrameConst(0);

  if (r.A) return 0;
  if (r.B) return 1;
  if (r.X) return 2;
  if (r.Y) return 3;
  if (r.L1) return 4;
  if (r.R1) return 5;
  if (r.L2) return 6;
  if (r.R2) return 7;
  if (r.L3) return 8;
  if (r.R3) return 9;
  if (r.START) return 10;
  if (r.SELECT) return 11;
  if (r.HOME) return 12;
  if (r.CAPTURE) return 13;
  if (r.EXTRA & (1u << 1)) return 14;
  if (r.EXTRA & (1u << 4)) return 15;
  if (r.EXTRA & (1u << 0)) return 16;
  if (r.EXTRA & (1u << 2)) return 17;
  if (r.EXTRA & (1u << 3)) return 18;

  return 0xFF;
}

void ButtonRemapMenu::applyRemap(controller_state_t& report, const uint8_t* remap) {
  uint8_t original[REMAP_MAX_BUTTONS] = {0};
  uint8_t originalAnalog[REMAP_MAX_BUTTONS] = {0};
  original[0] = report.A;
  original[1] = report.B;
  original[2] = report.X;
  original[3] = report.Y;
  original[4] = report.L1;
  original[5] = report.R1;
  original[6] = report.L2;
  original[7] = report.R2;
  original[8] = report.L3;
  original[9] = report.R3;
  original[10] = report.START;
  original[11] = report.SELECT;
  original[12] = report.HOME;
  original[13] = report.CAPTURE;
  const bool remapJaguarExtras = deviceMode == RZORD_JAGUAR;
  if (remapJaguarExtras) {
    original[14] = (report.EXTRA & (1u << 1)) != 0;
    original[15] = (report.EXTRA & (1u << 4)) != 0;
    original[16] = (report.EXTRA & (1u << 0)) != 0;
    original[17] = (report.EXTRA & (1u << 2)) != 0;
    original[18] = (report.EXTRA & (1u << 3)) != 0;
  }
  originalAnalog[6] = report.ANALOG_L2;
  originalAnalog[7] = report.ANALOG_R2;

  report.A = 0;
  report.B = 0;
  report.X = 0;
  report.Y = 0;
  report.L1 = 0;
  report.R1 = 0;
  report.L2 = 0;
  report.R2 = 0;
  report.L3 = 0;
  report.R3 = 0;
  report.START = 0;
  report.SELECT = 0;
  report.HOME = 0;
  report.CAPTURE = 0;
  if (remapJaguarExtras) {
    report.EXTRA &= (uint16_t)~((1u << 0) | (1u << 1) | (1u << 2) |
                                (1u << 3) | (1u << 4));
  }
  report.ANALOG_L2 = 0;
  report.ANALOG_R2 = 0;

  for (uint8_t i = 0; i < REMAP_MAX_BUTTONS; i++) {
    if (original[i] || originalAnalog[i]) {
      uint8_t target = remap[i];
      if (original[i]) {
        switch (target) {
          case 0: report.A = 1; break;
          case 1: report.B = 1; break;
          case 2: report.X = 1; break;
          case 3: report.Y = 1; break;
          case 4: report.L1 = 1; break;
          case 5: report.R1 = 1; break;
          case 6: report.L2 = 1; break;
          case 7: report.R2 = 1; break;
          case 8: report.L3 = 1; break;
          case 9: report.R3 = 1; break;
          case 10: report.START = 1; break;
          case 11: report.SELECT = 1; break;
          case 12: report.HOME = 1; break;
          case 13: report.CAPTURE = 1; break;
          case 14:
            if (remapJaguarExtras) report.EXTRA |= (1u << 1);
            break;
          case 15:
            if (remapJaguarExtras) report.EXTRA |= (1u << 4);
            break;
          case 16:
            if (remapJaguarExtras) report.EXTRA |= (1u << 0);
            break;
          case 17:
            if (remapJaguarExtras) report.EXTRA |= (1u << 2);
            break;
          case 18:
            if (remapJaguarExtras) report.EXTRA |= (1u << 3);
            break;
        }
      }
      if (originalAnalog[i]) {
        if (target == 6 && originalAnalog[i] > report.ANALOG_L2) {
          report.ANALOG_L2 = originalAnalog[i];
        } else if (target == 7 && originalAnalog[i] > report.ANALOG_R2) {
          report.ANALOG_R2 = originalAnalog[i];
        }
      }
    }
  }
}
