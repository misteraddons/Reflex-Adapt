#include "../product_config.h"

#include "menu.h"
#include "../core/settings_store.h"
#include "screensaver_animation.h"
#include "../platform/buzzer.h"

namespace {

const char* const screensaver_mode_names[] = {
  "Off", "Dim 1m", "Dim 5m", "Dim 10m", "Anim 1m", "Anim 5m", "Anim 10m"
};

constexpr uint8_t SCREENSAVER_MODE_COUNT = 7;

}  // namespace

bool handleScreensaverSubmenu(
    uint8_t& screensaver_submenu_cursor,
    bool& screensaver_submenu_active,
    uint8_t& menu_screensaver,
    uint8_t& menu_screensaver_anim,
    uint8_t selected_visible,
    uint8_t scroll_offset,
    bool bottom_right,
    bool ctrlDownJust,
    bool ctrlUpJust,
    bool ctrlLeftJust,
    bool ctrlAJust,
    bool ctrlRightJust,
    bool ctrlBJust,
    bool ctrlStartJust,
    bool btnNavigateJustPressed,
    bool btnChangeJustPressed) {
  if (!screensaver_submenu_active) return false;

  static bool screensaver_submenu_initialized = false;
  menu_screensaver_anim = sanitizeScreensaverAnimation(menu_screensaver_anim);

  uint8_t total_items = 1 + SCREENSAVER_ENABLED_ANIM_COUNT + 1;

  if (!screensaver_submenu_initialized) {
    screensaver_submenu_initialized = true;
    display.clear();
  }

  display.setFont(System5x7);
  display.set1X();
  display.setCursor(0, 0);
  display.print(F("Screensaver:"));

  const uint8_t visible_rows = 6;
  const uint8_t scroll_margin = 2;
  uint8_t scroll = 0;

  if (screensaver_submenu_cursor >= visible_rows - scroll_margin && total_items > visible_rows) {
    uint8_t new_scroll = screensaver_submenu_cursor - visible_rows + scroll_margin + 1;
    uint8_t max_scroll = total_items - visible_rows;
    scroll = (new_scroll < max_scroll) ? new_scroll : max_scroll;
  }

  for (uint8_t row = 0; row < visible_rows; row++) {
    uint8_t idx = scroll + row;
    display.setCursor(0, row + 1);

    if (idx == 0) {
      bool selected = (screensaver_submenu_cursor == 0);
      display.print(selected ? ">" : " ");
      display.print(F("Mode: "));
      display.print(screensaver_mode_names[menu_screensaver]);
      display.clearToEOL();
    } else if (idx <= SCREENSAVER_ENABLED_ANIM_COUNT) {
      uint8_t animIdx = screensaver_enabled_anim_ids[idx - 1];
      bool selected = (idx == screensaver_submenu_cursor);
      bool isCurrentAnim = (animIdx == menu_screensaver_anim);
      display.print(selected ? ">" : " ");
      display.print(screensaver_anim_names[animIdx]);
      display.clearToEOL();
      if (isCurrentAnim) {
        display.setCursor(115, row + 1);
        display.print("*");
      }
    } else if (idx == SCREENSAVER_ENABLED_ANIM_COUNT + 1) {
      bool selected = (screensaver_submenu_cursor == SCREENSAVER_ENABLED_ANIM_COUNT + 1);
      display.print(selected ? ">" : " ");
      display.print(F("[Save]"));
      display.clearToEOL();
    }
  }

  auto saveAndCloseScreensaver = [&]() {
    screensaver_mode = menu_screensaver;
    screensaver_animation = menu_screensaver_anim;
    saveScreensaverSettings(menu_screensaver, menu_screensaver_anim);
    resetAnimationState();
    buzzer.playSave();
    screensaver_submenu_active = false;
    screensaver_submenu_initialized = false;
    display.clear();
    renderMenu(selected_visible, scroll_offset, bottom_right);
  };

  if (ctrlDownJust || btnNavigateJustPressed) {
    if (screensaver_submenu_cursor < total_items - 1) {
      screensaver_submenu_cursor++;
      buzzer.playMenuNav();
    }
  }
  if (ctrlUpJust) {
    if (screensaver_submenu_cursor > 0) {
      screensaver_submenu_cursor--;
      buzzer.playMenuNav();
    }
  }

  if (ctrlStartJust) {
    saveAndCloseScreensaver();
    return true;
  }

  if (ctrlAJust || ctrlRightJust || btnChangeJustPressed) {
    if (screensaver_submenu_cursor == 0) {
      menu_screensaver = (menu_screensaver + 1) % SCREENSAVER_MODE_COUNT;
      buzzer.playMenuNav();
    } else if (screensaver_submenu_cursor <= SCREENSAVER_ENABLED_ANIM_COUNT) {
      menu_screensaver_anim = screensaver_enabled_anim_ids[screensaver_submenu_cursor - 1];
      saveAndCloseScreensaver();
    } else {
      saveAndCloseScreensaver();
    }
  }

  if (ctrlLeftJust) {
    if (screensaver_submenu_cursor == 0) {
      menu_screensaver = (menu_screensaver + SCREENSAVER_MODE_COUNT - 1) % SCREENSAVER_MODE_COUNT;
      buzzer.playMenuNav();
    }
  }

  if (ctrlBJust) {
    menu_screensaver = screensaver_mode;
    menu_screensaver_anim = screensaver_animation;
    screensaver_submenu_active = false;
    screensaver_submenu_initialized = false;
    display.clear();
    renderMenu(selected_visible, scroll_offset, bottom_right);
  }

  return true;
}
