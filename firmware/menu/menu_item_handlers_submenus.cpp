#include "menu_item_handlers_internal.h"

namespace menu_item_handlers_internal {

namespace {

uint8_t loadMenuBuzzerMode() {
  return (uint8_t)loadSettingValue(SettingId::BuzzerMode, RZORD_NONE);
}

void applyMenuBuzzerMode(uint8_t mode) {
  buzzer_enabled = mode > 0;
  buzzer.setEnabled(buzzer_enabled);
  buzzer.setVolume((mode == 1) ? 10 : 100);
}

}  // namespace

void handle_item_screensaver(menu_item_action action) {
  switch (action) {
    case item_action_display:
      display.print(">");
      break;
    case item_action_change:
    case item_action_change_prev:
      screensaver_submenu_active = true;
      screensaver_submenu_cursor = 0;
      break;
    case item_action_reset:
    case item_action_apply:
    case item_action_save:
      break;
  }
}

void handle_item_games(menu_item_action action) {
  switch (action) {
    case item_action_display:
      display.print(">");
      break;
    case item_action_change:
    case item_action_change_prev:
      games_submenu_active = true;
      games_submenu_cursor = 0;
      break;
    case item_action_reset:
    case item_action_apply:
    case item_action_save:
      break;
  }
}

void handle_item_buzzer(menu_item_action action) {
  switch (action) {
    case item_action_display:
      display.print(">");
      break;
    case item_action_change:
    case item_action_change_prev:
      sound_submenu_active = true;
      sound_submenu_cursor = 0;
      menu_buzzer_mode = loadMenuBuzzerMode();
      menu_sound_events = buzzer.getEventMask();
      break;
    case item_action_reset:
      menu_buzzer_mode = loadMenuBuzzerMode();
      menu_sound_events = buzzer.getEventMask();
      break;
    case item_action_apply:
      applyMenuBuzzerMode(menu_buzzer_mode);
      buzzer.setEventMask(menu_sound_events);
      break;
    case item_action_save:
      saveSoundSettings(menu_buzzer_mode, menu_sound_events);
      break;
  }
}

void handle_item_hotkeys(menu_item_action action) {
  switch (action) {
    case item_action_display:
      display.print(">");
      break;
    case item_action_change:
    case item_action_change_prev:
      hotkeys_submenu_active = true;
      hotkeys_submenu_cursor = 0;
      break;
    case item_action_reset:
    case item_action_apply:
    case item_action_save:
      break;
  }
}

void handle_item_kiosk_mode(menu_item_action action) {
  switch (action) {
    case item_action_display:
      display.print(menu_kiosk_mode ? "On" : "Off");
      break;
    case item_action_change:
    case item_action_change_prev:
      kiosk_submenu_active = true;
      kiosk_submenu_selection = menu_kiosk_mode ? 0 : 1;
      break;
    case item_action_reset:
    case item_action_apply:
    case item_action_save:
      break;
  }
}

void handle_item_about(menu_item_action action) {
  switch (action) {
    case item_action_display:
      if (!about_screen_active) {
        display.print("Press to view");
      } else {
        display.print("(Viewing...)");
      }
      break;
    case item_action_change:
    case item_action_change_prev:
      if (!about_screen_active) {
        about_screen_active = true;
        renderAboutScreen();
      } else {
        about_screen_active = false;
        display.clear();
        forceMainDisplayRefresh();
      }
      break;
    default:
      break;
  }
}

}
