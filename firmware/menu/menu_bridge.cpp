#include "../product_config.h"

#include "menu_bridge.h"
#include "menu_bridge_internal.h"

#include <Adafruit_TinyUSB.h>

#include "../core/firmware_support.h"
#include "../core/settings_store.h"
#include "../platform/buzzer.h"

#include "mapping_display.h"
#include "menu_catalog.h"
#include "menu_display_state.h"
#include "menu_helpers.h"
#include "menu_idle_runtime.h"
#include "menu_input.h"
#include "menu_item_handlers.h"
#include "menu_mode_state.h"
#include "menu_navigation.h"
#include "menu_runtime_state.h"
#include "menu_ui_state.h"

#include "../platform/display_runtime_state.h"
#include "../output/output_runtime_state.h"

namespace {

void clearMenuBridgeDisplay() {
#ifdef USE_I2C_DISPLAY
  display.clear();
#endif
}

void showSavedRebootMessage() {
#ifdef USE_I2C_DISPLAY
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_7x14B_tr);
  u8g2.drawStr(15, 20, "SETTINGS SAVED");
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(28, 38, "Rebooting...");
  u8g2.sendBuffer();
#endif
}

void saveSystemMenuAndReboot() {
  menuSaveAllItems();
  saveMenuModeSettings();
  buzzer.playSave();

  showSavedRebootMessage();

  uint32_t waitStart = millis();
  while (millis() - waitStart < 1500) {
    buzzer.update();
    delay(5);
  }
  reboot();
}

}  // namespace

void closeMenu() {
  isMenuOpen = false;
  mainDisplayInitialized = false;
  if (mappingDisplayActive) {
    mappingDisplayActive = false;
    deactivateMappingDisplay();
  }
  games_submenu_active = false;
  screensaver_submenu_active = false;
  sound_submenu_active = false;
  hotkeys_submenu_active = false;
  hotkeys_capture_active = false;
  kiosk_submenu_active = false;
  about_screen_active = false;
  factory_reset_stage = 0;
  bootloader_stage = 0;
  clearMenuBridgeDisplay();
}

void forceMainDisplayRefresh() {
  mainDisplayInitialized = false;
  padDisplayNeedsRedraw = true;
}

void showMenu(bool btnChangeJustPressed, bool btnNavigateJustPressed) {
  const uint8_t menu_invalid = 255;
  const uint8_t menu_reboot = 254;

  uint8_t& menu_state = menu_navigation_state;
  uint8_t& selected_visible = menu_selected_visible;
  uint8_t& scroll_offset = menu_scroll_offset;
  bool& bottom_right = menu_bottom_right_selected;

  MenuControllerInput ctrl = readMenuControllerInput();

  bool ctrlBack = ctrl.bJust;
  bool navDown = btnNavigateJustPressed || ctrl.downJust;
  bool navUp = ctrl.upJust;
  bool navRight = ctrl.rightJust;
  bool navLeft = ctrl.leftJust;
  bool navActivate = btnChangeJustPressed || ctrl.aJust;

  handleIdleUiActivity(
    btnChangeJustPressed || btnNavigateJustPressed ||
    ctrl.upJust || ctrl.downJust || ctrl.leftJust || ctrl.rightJust ||
    ctrl.aJust || ctrl.bJust || ctrl.startJust
  );

  if (handleMenuToolScreens(btnChangeJustPressed, btnNavigateJustPressed, ctrl,
      selected_visible, scroll_offset, bottom_right)) {
    return;
  }

  if (handleMenuSubmenus(btnChangeJustPressed, btnNavigateJustPressed, ctrl,
      selected_visible, scroll_offset, bottom_right)) {
    return;
  }

  if (menu_state == menu_reboot) {
    if (btnNavigateJustPressed) {
      tud_disconnect();
      clearMenuBridgeDisplay();
      delay(1000);
      reboot();
    }
    return;
  }

  if (menu_state == menu_invalid) {
    menu_state = 0;
    selected_visible = 0;
    scroll_offset = 0;
    bottom_right = false;
    menuResetAllItems();
    clearMenuBridgeDisplay();
    renderMenu(selected_visible, scroll_offset, bottom_right);
    return;
  }

  uint8_t visible_count = getVisibleItemCount();
  const uint8_t item_rows = 6;
  bool on_bottom_row = (selected_visible >= visible_count);

  bool in_confirmation = (factory_reset_stage == 1) || (bootloader_stage == 1);

  if (ctrl.startJust && !in_confirmation) {
    saveSystemMenuAndReboot();
    return;
  }

  if (navDown && !in_confirmation) {
    buzzer.playMenuNav();
    if (on_bottom_row) {
      if (!bottom_right) {
        bottom_right = true;
      } else {
        bottom_right = false;
        selected_visible = 0;
        scroll_offset = 0;
      }
    } else {
      selected_visible++;
      if (selected_visible >= visible_count) {
        bottom_right = false;
      } else {
        const uint8_t scroll_margin = 2;
        if (selected_visible >= scroll_offset + item_rows - scroll_margin) {
          uint8_t new_offset = selected_visible - item_rows + scroll_margin + 1;
          uint8_t max_offset = (visible_count > item_rows) ? visible_count - item_rows : 0;
          scroll_offset = (new_offset < max_offset) ? new_offset : max_offset;
        }
      }
    }
    renderMenu(selected_visible, scroll_offset, bottom_right);
  }

  if (navUp && !in_confirmation) {
    buzzer.playMenuNav();
    if (on_bottom_row) {
      selected_visible = visible_count - 1;
      if (visible_count > item_rows) {
        scroll_offset = visible_count - item_rows;
      }
      bottom_right = false;
    } else if (selected_visible > 0) {
      selected_visible--;
      const uint8_t scroll_margin = 2;
      if (selected_visible < scroll_offset + scroll_margin && scroll_offset > 0) {
        scroll_offset = (selected_visible > scroll_margin) ? selected_visible - scroll_margin : 0;
      }
    } else {
      selected_visible = visible_count;
      bottom_right = true;
    }
    renderMenu(selected_visible, scroll_offset, bottom_right);
  }

  if (on_bottom_row && (navLeft || navRight)) {
    buzzer.playMenuNav();
    if (navRight && !bottom_right) {
      bottom_right = true;
    } else if (navLeft && bottom_right) {
      bottom_right = false;
    }
    renderMenu(selected_visible, scroll_offset, bottom_right);
    return;
  }

  if (ctrlBack && in_confirmation) {
    buzzer.playMenuNav();
    factory_reset_stage = 0;
    bootloader_stage = 0;
    clearMenuBridgeDisplay();
    renderMenu(selected_visible, scroll_offset, bottom_right);
    return;
  }

  if (factory_reset_stage == 1) {
    if (btnNavigateJustPressed) {
      buzzer.playMenuNav();
      handleMenuItemAction(menu_item_factory_reset, item_action_change_prev);
    }
    if (navActivate) {
      handleMenuItemAction(menu_item_factory_reset, item_action_change);
      if (factory_reset_stage == 0) {
        clearMenuBridgeDisplay();
        renderMenu(selected_visible, scroll_offset, bottom_right);
      }
    }
    return;
  }

  if (navActivate) {
    if (on_bottom_row) {
      if (!bottom_right) {
        buzzer.playMenuNav();
        menuResetAllItems();
        menu_state = menu_invalid;
        closeMenu();
        return;
      } else {
        saveSystemMenuAndReboot();
        return;
      }
    }

    uint8_t actual_idx = getActualIndex(selected_visible);
    menu_item_enum current_item = menu_items[actual_idx].item;
    if (handleMenuValueChange(current_item, item_action_change, selected_visible, scroll_offset, bottom_right, item_rows)) {
      return;
    }
  }

  if (navRight && !on_bottom_row) {
    uint8_t actual_idx = getActualIndex(selected_visible);
    menu_item_enum current_item = menu_items[actual_idx].item;
    if (handleMenuValueChange(current_item, item_action_change, selected_visible, scroll_offset, bottom_right, item_rows)) {
      return;
    }
  }

  if (navLeft && !on_bottom_row) {
    uint8_t actual_idx = getActualIndex(selected_visible);
    menu_item_enum current_item = menu_items[actual_idx].item;
    if (handleMenuValueChange(current_item, item_action_change_prev, selected_visible, scroll_offset, bottom_right, item_rows)) {
      return;
    }
  }
}
