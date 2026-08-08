#include "../product_config.h"

#include "menu.h"
#include "../core/settings_store.h"
#include "../platform/buzzer.h"

namespace {

const char* const buzzer_mode_names[] = { "Off", "Low", "High" };

void applyMenuBuzzerMode(uint8_t mode, uint16_t soundEvents) {
  buzzer.setEnabled(mode > 0);
  buzzer.setVolume((mode == 1) ? 10 : 100);
  buzzer.setEventMask(soundEvents);
}

}  // namespace

bool handleSoundSubmenu(
    uint8_t& sound_submenu_cursor,
    bool& sound_submenu_active,
    uint8_t& menu_buzzer_mode,
    uint16_t& menu_sound_events,
    uint8_t selected_visible,
    uint8_t scroll_offset,
    bool bottom_right,
    bool ctrlDownJust,
    bool ctrlUpJust,
    bool ctrlAJust,
    bool ctrlRightJust,
    bool ctrlBJust,
    bool btnNavigateJustPressed,
    bool btnChangeJustPressed) {
  if (!sound_submenu_active) return false;

  static bool sound_submenu_initialized = false;

  if (!sound_submenu_initialized) {
    sound_submenu_initialized = true;
    display.clear();
  }

  display.setFont(System5x7);
  display.set1X();
  display.setCursor(0, 0);
  display.print(F("Sound Events:"));

  const uint8_t visible_rows = 6;
  const uint8_t scroll_margin = 2;
  uint8_t scroll = 0;
  uint8_t total_items = SOUND_EVENT_COUNT + 2;

  if (sound_submenu_cursor >= visible_rows - scroll_margin && total_items > visible_rows) {
    uint8_t new_scroll = sound_submenu_cursor - visible_rows + scroll_margin + 1;
    uint8_t max_scroll = total_items - visible_rows;
    scroll = (new_scroll < max_scroll) ? new_scroll : max_scroll;
  }

  for (uint8_t row = 0; row < visible_rows; row++) {
    uint8_t idx = scroll + row;
    display.setCursor(0, row + 1);

    if (idx == 0) {
      bool selected = (sound_submenu_cursor == 0);
      display.print(selected ? ">" : " ");
      display.print(F("Mode"));
      display.clearToEOL();

      display.setCursor(98, row + 1);
      display.print(buzzer_mode_names[menu_buzzer_mode <= 2 ? menu_buzzer_mode : 0]);
    } else if (idx <= SOUND_EVENT_COUNT) {
      const uint8_t soundIdx = idx - 1;
      bool selected = (idx == sound_submenu_cursor);
      bool enabled = (menu_sound_events & sound_event_items[soundIdx].mask) != 0;

      display.print(selected ? ">" : " ");
      display.print(sound_event_items[soundIdx].name);
      display.clearToEOL();

      display.setCursor(100, row + 1);
      display.print(enabled ? "On" : "Off");
    } else if (idx == SOUND_EVENT_COUNT + 1) {
      bool selected = (sound_submenu_cursor == SOUND_EVENT_COUNT + 1);
      display.print(selected ? ">" : " ");
      display.print(F("[Back]"));
      display.clearToEOL();
    }
  }

  if (total_items > visible_rows) {
    display.setCursor(120, 7);
    if (scroll > 0) display.print("^");
    if (scroll + visible_rows < total_items) display.print("v");
  }

  if (ctrlDownJust || btnNavigateJustPressed) {
    if (sound_submenu_cursor < total_items - 1) {
      sound_submenu_cursor++;
      buzzer.playMenuNav();
    }
  }
  if (ctrlUpJust) {
    if (sound_submenu_cursor > 0) {
      sound_submenu_cursor--;
      buzzer.playMenuNav();
    }
  }

  if (ctrlAJust || ctrlRightJust || btnChangeJustPressed) {
    if (sound_submenu_cursor == 0) {
      menu_buzzer_mode = (menu_buzzer_mode + 1) % 3;
      applyMenuBuzzerMode(menu_buzzer_mode, menu_sound_events);
      buzzer.playMenuNav();
    } else if (sound_submenu_cursor <= SOUND_EVENT_COUNT) {
      menu_sound_events ^= sound_event_items[sound_submenu_cursor - 1].mask;
      applyMenuBuzzerMode(menu_buzzer_mode, menu_sound_events);
      buzzer.playMenuNav();
    } else {
      applyMenuBuzzerMode(menu_buzzer_mode, menu_sound_events);
      saveSoundSettings(menu_buzzer_mode, menu_sound_events);
      buzzer.playSave();
      sound_submenu_active = false;
      sound_submenu_initialized = false;
      display.clear();
      renderMenu(selected_visible, scroll_offset, bottom_right);
    }
  }

  if (ctrlBJust) {
    applyMenuBuzzerMode(menu_buzzer_mode, menu_sound_events);
    saveSoundSettings(menu_buzzer_mode, menu_sound_events);
    buzzer.playSave();
    sound_submenu_active = false;
    sound_submenu_initialized = false;
    display.clear();
    renderMenu(selected_visible, scroll_offset, bottom_right);
  }

  return true;
}
