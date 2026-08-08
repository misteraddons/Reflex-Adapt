#include "../product_config.h"

#include <stdio.h>

#include "menu_submenu_hotkeys.h"

#include "menu.h"
#include "menu_runtime_state.h"
#include "../core/device_runtime_state.h"
#include "../core/hotkey_combo.h"
#include "../platform/buzzer.h"

namespace {

constexpr uint8_t kHotkeyRowCount = 6;
constexpr uint8_t kHotkeyValueSize = 12;

const char* const kHotkeyHoldNames[] = {
  "Press", "0.5s", "1.0s", "1.5s", "2.0s", "2.5s", "3.0s"
};

struct HotkeyRenderSnapshot {
  bool valid;
  uint8_t cursor;
  uint8_t hold;
  uint8_t menu_enabled;
  uint8_t system_enabled;
  uint8_t home_enabled;
  uint8_t capture_enabled;
  uint32_t menu_combo;
  uint32_t system_combo;
  uint32_t home_combo;
  uint32_t capture_combo;
};

HotkeyRenderSnapshot s_hotkey_render_snapshot{};

const char* compactHotkeyComboName(uint32_t combo) {
  switch (combo) {
    case INPUT_PAD_L | INPUT_START:
      return "<+ST";
    case INPUT_PAD_R | INPUT_START:
      return ">+ST";
    case INPUT_PAD_D | INPUT_START:
      return "v+ST";
    case INPUT_PAD_U | INPUT_START:
      return "^+ST";
    default:
      return nullptr;
  }
}

void formatHotkeyValue(char* buffer, size_t size, uint8_t enabled, uint32_t combo) {
  if (buffer == nullptr || size == 0) {
    return;
  }
  if (!enabled || combo == 0) {
    snprintf(buffer, size, "Off");
    return;
  }

  const char* compact = compactHotkeyComboName(combo);
  if (compact != nullptr) {
    snprintf(buffer, size, "%s", compact);
    return;
  }

  formatHotkeyCombo(buffer, size, combo, deviceMode);
}

bool hotkeyRenderSnapshotMatches(uint8_t cursor) {
  return s_hotkey_render_snapshot.valid &&
         s_hotkey_render_snapshot.cursor == cursor &&
         s_hotkey_render_snapshot.hold == menu_hotkey_hold_time &&
         s_hotkey_render_snapshot.menu_enabled == menu_menu_hotkey &&
         s_hotkey_render_snapshot.system_enabled == menu_system_menu_hotkey &&
         s_hotkey_render_snapshot.home_enabled == menu_home_hotkey &&
         s_hotkey_render_snapshot.capture_enabled == menu_capture_hotkey &&
         s_hotkey_render_snapshot.menu_combo == menu_hotkey_combo &&
         s_hotkey_render_snapshot.system_combo == menu_system_hotkey_combo &&
         s_hotkey_render_snapshot.home_combo == menu_home_hotkey_combo &&
         s_hotkey_render_snapshot.capture_combo == menu_capture_hotkey_combo;
}

void captureHotkeyRenderSnapshot(uint8_t cursor) {
  s_hotkey_render_snapshot.valid = true;
  s_hotkey_render_snapshot.cursor = cursor;
  s_hotkey_render_snapshot.hold = menu_hotkey_hold_time;
  s_hotkey_render_snapshot.menu_enabled = menu_menu_hotkey;
  s_hotkey_render_snapshot.system_enabled = menu_system_menu_hotkey;
  s_hotkey_render_snapshot.home_enabled = menu_home_hotkey;
  s_hotkey_render_snapshot.capture_enabled = menu_capture_hotkey;
  s_hotkey_render_snapshot.menu_combo = menu_hotkey_combo;
  s_hotkey_render_snapshot.system_combo = menu_system_hotkey_combo;
  s_hotkey_render_snapshot.home_combo = menu_home_hotkey_combo;
  s_hotkey_render_snapshot.capture_combo = menu_capture_hotkey_combo;
}

void invalidateHotkeyRenderSnapshot() {
  s_hotkey_render_snapshot.valid = false;
}

void renderHotkeySettingRow(uint8_t row, uint8_t cursor, const char* label, const char* value) {
  display.clear(0, 127, row + 1, row + 1);
  display.setCursor(0, row + 1);
  display.print(row == cursor ? ">" : " ");
  display.print(label);
  display.setCursor(88, row + 1);
  display.print(value);
  display.clearToEOL();
}

void renderHotkeyRows(uint8_t cursor) {
  if (hotkeyRenderSnapshotMatches(cursor)) {
    return;
  }

  if (!s_hotkey_render_snapshot.valid) {
    display.clear();
  }

  display.setFont(System5x7);
  display.set1X();
  display.setCursor(0, 0);
  display.print(F("Hotkey"));
  display.clearToEOL();

  char value[kHotkeyValueSize];
  renderHotkeySettingRow(0, cursor, "Hotkey Hold", kHotkeyHoldNames[menu_hotkey_hold_time <= 6 ? menu_hotkey_hold_time : 1]);
  formatHotkeyValue(value, sizeof(value), menu_menu_hotkey, menu_hotkey_combo);
  renderHotkeySettingRow(1, cursor, "Quick Menu", value);
  formatHotkeyValue(value, sizeof(value), menu_system_menu_hotkey, menu_system_hotkey_combo);
  renderHotkeySettingRow(2, cursor, "System Menu", value);
  formatHotkeyValue(value, sizeof(value), menu_home_hotkey, menu_home_hotkey_combo);
  renderHotkeySettingRow(3, cursor, "Home", value);
  formatHotkeyValue(value, sizeof(value), menu_capture_hotkey, menu_capture_hotkey_combo);
  renderHotkeySettingRow(4, cursor, "Capture", value);

  display.setCursor(0, 6);
  display.print(cursor == 5 ? ">" : " ");
  display.print(F("[Back]"));
  display.clearToEOL();
  display.clear(0, 127, 7, 7);
  captureHotkeyRenderSnapshot(cursor);
}

void toggleSelectedHotkey(uint8_t cursor, bool reverse) {
  switch (cursor) {
    case 0:
      if (reverse) {
        menu_hotkey_hold_time = (menu_hotkey_hold_time == 0) ? 6 : (uint8_t)(menu_hotkey_hold_time - 1);
      } else {
        menu_hotkey_hold_time = (uint8_t)((menu_hotkey_hold_time + 1) % 7);
      }
      break;
    case 1:
      menu_menu_hotkey = menu_menu_hotkey ? 0 : 1;
      break;
    case 2:
      menu_system_menu_hotkey = menu_system_menu_hotkey ? 0 : 1;
      break;
    case 3:
      menu_home_hotkey = menu_home_hotkey ? 0 : 1;
      break;
    case 4:
      menu_capture_hotkey = menu_capture_hotkey ? 0 : 1;
      break;
    default:
      break;
  }
}

void closeHotkeySubmenu(bool& active, uint8_t selected_visible, uint8_t scroll_offset, bool bottom_right) {
  active = false;
  invalidateHotkeyRenderSnapshot();
  display.clear();
  renderMenu(selected_visible, scroll_offset, bottom_right);
}

}  // namespace

bool handleHotkeysSubmenu(
    uint8_t& hotkeys_submenu_cursor,
    bool& hotkeys_submenu_active,
    uint8_t selected_visible,
    uint8_t scroll_offset,
    bool bottom_right,
    bool ctrlDownJust,
    bool ctrlUpJust,
    bool ctrlLeftJust,
    bool ctrlAJust,
    bool ctrlRightJust,
    bool ctrlBJust,
    bool btnNavigateJustPressed,
    bool btnChangeJustPressed) {
  if (!hotkeys_submenu_active) {
    invalidateHotkeyRenderSnapshot();
    return false;
  }

  if (hotkeys_submenu_cursor >= kHotkeyRowCount) {
    hotkeys_submenu_cursor = 0;
  }

  if (ctrlDownJust || btnNavigateJustPressed) {
    hotkeys_submenu_cursor = (uint8_t)((hotkeys_submenu_cursor + 1) % kHotkeyRowCount);
    buzzer.playMenuNav();
    renderHotkeyRows(hotkeys_submenu_cursor);
    return true;
  }

  if (ctrlUpJust) {
    hotkeys_submenu_cursor = (hotkeys_submenu_cursor == 0) ? (kHotkeyRowCount - 1) : (uint8_t)(hotkeys_submenu_cursor - 1);
    buzzer.playMenuNav();
    renderHotkeyRows(hotkeys_submenu_cursor);
    return true;
  }

  if (ctrlBJust) {
    closeHotkeySubmenu(hotkeys_submenu_active, selected_visible, scroll_offset, bottom_right);
    buzzer.playMenuNav();
    return true;
  }

  if (hotkeys_submenu_cursor == kHotkeyRowCount - 1 && (ctrlAJust || ctrlRightJust || btnChangeJustPressed)) {
    closeHotkeySubmenu(hotkeys_submenu_active, selected_visible, scroll_offset, bottom_right);
    buzzer.playMenuNav();
    return true;
  }

  if (hotkeys_submenu_cursor < kHotkeyRowCount - 1 && (ctrlAJust || ctrlRightJust || btnChangeJustPressed || ctrlLeftJust)) {
    toggleSelectedHotkey(hotkeys_submenu_cursor, ctrlLeftJust);
    buzzer.playMenuNav();
    renderHotkeyRows(hotkeys_submenu_cursor);
    return true;
  }

  renderHotkeyRows(hotkeys_submenu_cursor);
  return true;
}
