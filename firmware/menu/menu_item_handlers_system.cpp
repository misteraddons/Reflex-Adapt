#include "menu_item_handlers_internal.h"

namespace menu_item_handlers_internal {

void handle_item_factory_reset(menu_item_action action) {
  switch (action) {
    case item_action_display:
      if (factory_reset_stage == 0) {
        display.print("Press to reset");
      } else if (factory_reset_stage == 1) {
        display.print("(Confirming...)");
      }
      break;
    case item_action_change:
      if (factory_reset_stage == 0) {
        factory_reset_stage = 1;
        factory_reset_selection = 0;
        drawFactoryResetPrompt(factory_reset_selection);
      } else if (factory_reset_stage == 1) {
        if (factory_reset_selection == 1) {
          factory_reset_stage = 0;
          return;
        }

        factory_reset_stage = 0;
        factoryResetSettings();
        buzzer.playFactoryReset();

        drawFactoryResetResult();
        delay(1500);
        reboot();
      }
      break;
    case item_action_change_prev:
      if (factory_reset_stage == 1) {
        factory_reset_selection = (factory_reset_selection + 1) % 2;
        drawFactoryResetPrompt(factory_reset_selection);
      }
      break;
    default:
      break;
  }
}

void handle_item_bootloader(menu_item_action action) {
  switch (action) {
    case item_action_display:
      if (bootloader_stage == 0) {
        display.print("Enter");
      } else if (bootloader_stage == 1) {
        display.print("(Confirm...)");
      }
      break;
    case item_action_change:
      if (bootloader_stage == 0) {
        bootloader_stage = 1;
        drawBootloaderPrompt();
      } else if (bootloader_stage == 1) {
        bootloader_stage = 0;
        buzzer.playModeChange();
        drawBootloaderEnteringScreen();
        delay(500);
        rp2040.rebootToBootloader();
      }
      break;
    case item_action_reset:
      if (bootloader_stage == 1) {
        bootloader_stage = 0;
        display.clear();
        forceMainDisplayRefresh();
      }
      break;
    default:
      break;
  }
}

void handle_item_display_contrast(menu_item_action action) {
  switch (action) {
    case item_action_display:
      display.print(menu_display_contrast >= 254 ? 100 : (menu_display_contrast * 100) / 255);
      display.print('%');
      break;
    case item_action_change:
      if (menu_display_contrast <= 64) {
        menu_display_contrast = 128;
      } else if (menu_display_contrast <= 128) {
        menu_display_contrast = 192;
      } else if (menu_display_contrast <= 192) {
        menu_display_contrast = 254;
      } else {
        menu_display_contrast = 64;
      }
      display.setContrast(menu_display_contrast);
      break;
    case item_action_change_prev:
      if (menu_display_contrast >= 254) {
        menu_display_contrast = 192;
      } else if (menu_display_contrast >= 192) {
        menu_display_contrast = 128;
      } else if (menu_display_contrast >= 128) {
        menu_display_contrast = 64;
      } else {
        menu_display_contrast = 254;
      }
      display.setContrast(menu_display_contrast);
      break;
    case item_action_reset:
      menu_display_contrast = display_contrast;
      display.setContrast(menu_display_contrast);
      break;
    case item_action_apply:
      display_contrast = menu_display_contrast;
      break;
    case item_action_save:
      saveSystemSettingByte(SettingId::DisplayContrast, menu_display_contrast);
      break;
  }
}

}
