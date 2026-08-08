#include "../product_config.h"

#include "menu_bridge_internal.h"

#include "menu_runtime_state.h"
#include "menu_submenu_games.h"
#include "menu_submenu_hotkeys.h"
#include "menu_submenu_kiosk.h"
#include "menu_submenu_screensaver.h"
#include "menu_submenu_sound.h"
#include "menu_ui_state.h"
#include "menu_working_state.h"

bool handleMenuSubmenus(bool btnChangeJustPressed, bool btnNavigateJustPressed,
                        const MenuControllerInput& ctrl,
                        uint8_t selected_visible, uint8_t scroll_offset, bool bottom_right) {
  if (handleSoundSubmenu(sound_submenu_cursor, sound_submenu_active, menu_buzzer_mode, menu_sound_events,
      selected_visible, scroll_offset, bottom_right,
      ctrl.downJust, ctrl.upJust, ctrl.aJust, ctrl.rightJust, ctrl.bJust,
      btnNavigateJustPressed, btnChangeJustPressed)) {
    return true;
  }

  if (handleGamesSubmenu(games_submenu_cursor, games_submenu_active,
      selected_visible, scroll_offset, bottom_right,
      ctrl.downJust, ctrl.upJust, ctrl.aJust, ctrl.rightJust, ctrl.bJust,
      btnNavigateJustPressed, btnChangeJustPressed)) {
    return true;
  }

  if (handleScreensaverSubmenu(screensaver_submenu_cursor, screensaver_submenu_active,
      menu_screensaver, menu_screensaver_anim,
      selected_visible, scroll_offset, bottom_right,
      ctrl.downJust, ctrl.upJust, ctrl.leftJust, ctrl.aJust, ctrl.rightJust, ctrl.bJust,
      ctrl.startJust,
      btnNavigateJustPressed, btnChangeJustPressed)) {
    return true;
  }

  if (handleHotkeysSubmenu(hotkeys_submenu_cursor, hotkeys_submenu_active,
      selected_visible, scroll_offset, bottom_right,
      ctrl.downJust, ctrl.upJust, ctrl.leftJust, ctrl.aJust, ctrl.rightJust, ctrl.bJust,
      btnNavigateJustPressed, btnChangeJustPressed)) {
    return true;
  }

  if (handleKioskSubmenu(kiosk_submenu_selection, kiosk_submenu_active,
      selected_visible, scroll_offset, bottom_right,
      ctrl.downJust, ctrl.upJust, ctrl.leftJust, ctrl.aJust, ctrl.rightJust, ctrl.bJust,
      btnNavigateJustPressed, btnChangeJustPressed)) {
    return true;
  }

  return false;
}
