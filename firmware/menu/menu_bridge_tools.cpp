#include "../product_config.h"

#include "menu_bridge_internal.h"

#include "../core/button_remap.h"
#include "../core/controller_settings_state.h"
#include "../core/settings_store.h"
#include "../platform/buzzer.h"
#include "../platform/display_runtime_state.h"

#include "mapping_display.h"
#include "menu_analog_test.h"
#include "menu_helpers.h"
#include "menu_pad_test.h"
#include "menu_pin_debug.h"
#include "menu_runtime_state.h"
#include "menu_ui_state.h"

namespace {

void redrawMenuAfterOverlayExit(uint8_t selected_visible, uint8_t scroll_offset, bool bottom_right) {
  display.clear();
  renderMenu(selected_visible, scroll_offset, bottom_right);
}

void renderButtonRemap(bool btnChangeJustPressed, bool btnNavigateJustPressed) {
  if (btnNavigateJustPressed) {
    remapMenu.navigate();
    buzzer.playMenuNav();
  }

  if (btnChangeJustPressed) {
    if (remapMenu.isEditMode()) {
      uint8_t pressed = remapMenu.getFirstPressedButton();
      if (pressed != 0xFF) {
        remapMenu.setDestination(pressed);
        buzzer.playMenuNav();
      }
    } else {
      uint8_t result = remapMenu.select();
      if (result == 1) {
        saveButtonRemaps();
        buzzer.playSave();
        buttonRemapActive = false;
        remapMenu.close();
        return;
      } else if (result == 2) {
        saveButtonRemaps();
        buzzer.playMenuNav();
      } else if (result == 3) {
        loadRemapsFromEEPROM();
        buzzer.playMenuNav();
        buttonRemapActive = false;
        remapMenu.close();
        return;
      } else {
        buzzer.playMenuNav();
      }
    }
  }

  if (remapMenu.isEditMode()) {
    uint8_t pressed = remapMenu.getFirstPressedButton();
    if (pressed != 0xFF) {
      remapMenu.setDestination(pressed);
      buzzer.playMenuNav();
    }
  }

  remapMenu.render(deviceMode);
}

}  // namespace

bool handleMenuToolScreens(bool btnChangeJustPressed, bool btnNavigateJustPressed,
                           const MenuControllerInput& ctrl,
                           uint8_t selected_visible, uint8_t scroll_offset, bool bottom_right) {
  if (about_screen_active) {
    if (btnChangeJustPressed || btnNavigateJustPressed || ctrl.aJust || ctrl.bJust) {
      about_screen_active = false;
      buzzer.playMenuNav();
      redrawMenuAfterOverlayExit(selected_visible, scroll_offset, bottom_right);
    }
    return true;
  }

  if (buttonRemapActive) {
    renderButtonRemap(btnChangeJustPressed, btnNavigateJustPressed);
    if (!buttonRemapActive) {
      redrawMenuAfterOverlayExit(selected_visible, scroll_offset, bottom_right);
    }
    return true;
  }

  if (padTestActive) {
    renderPadTest(btnChangeJustPressed);
    if (!padTestActive) {
      redrawMenuAfterOverlayExit(selected_visible, scroll_offset, bottom_right);
    }
    return true;
  }

  if (pinDebugActive) {
    renderPinDebug(btnChangeJustPressed, btnNavigateJustPressed);
    if (!pinDebugActive) {
      redrawMenuAfterOverlayExit(selected_visible, scroll_offset, bottom_right);
    }
    return true;
  }

  if (analogTestActive) {
    renderAnalogTest(btnChangeJustPressed);
    if (!analogTestActive) {
      redrawMenuAfterOverlayExit(selected_visible, scroll_offset, bottom_right);
    }
    return true;
  }

  if (mappingDisplayActive) {
    renderMappingDisplay();

    if (btnChangeJustPressed) {
      mappingDisplayActive = false;
      deactivateMappingDisplay();
      buzzer.playMenuNav();
      redrawMenuAfterOverlayExit(selected_visible, scroll_offset, bottom_right);
    }
    return true;
  }

  return false;
}
