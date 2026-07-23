#include "menu_item_handlers_internal.h"

namespace menu_item_handlers_internal {

void handle_item_guncon_offset(menu_item_action action) {
  switch (action) {
    case item_action_display:
      display.print("X:");
      display.print(menu_guncon_offset_x);
      display.print(" Y:");
      display.print(menu_guncon_offset_y);
      break;
    case item_action_change:
    case item_action_change_prev:
      menu_guncon_offset_x = 0;
      menu_guncon_offset_y = 0;
      break;
    case item_action_reset: {
      PerModeQuickSettings settings;
      readPerModeQuickSettings(deviceMode, settings);
      menu_guncon_offset_x = settings.guncon_offset_x;
      menu_guncon_offset_y = settings.guncon_offset_y;
      break;
    }
    case item_action_apply:
      break;
    case item_action_save:
      break;
  }
}

void handle_item_jogcon_force(menu_item_action action) {
  static const uint8_t force_values[] = { 1, 3, 7, 15 };
  static const char* force_names[] = { "Low", "Med-L", "Med-H", "High" };

  uint8_t idx = 0;
  for (uint8_t i = 0; i < 4; i++) {
    if (force_values[i] == menu_jogcon_force) {
      idx = i;
      break;
    }
  }

  switch (action) {
    case item_action_display:
      display.print(force_names[idx]);
      break;
    case item_action_change:
      idx = (idx + 1) % 4;
      menu_jogcon_force = force_values[idx];
      break;
    case item_action_change_prev:
      idx = (idx + 3) % 4;
      menu_jogcon_force = force_values[idx];
      break;
    case item_action_reset: {
      PerModeQuickSettings settings;
      readPerModeQuickSettings(deviceMode, settings);
      menu_jogcon_force = settings.jogcon_force;
      break;
    }
    case item_action_apply:
      break;
    case item_action_save:
      break;
  }
}

void handle_item_wheel_sensitivity(menu_item_action action) {
  static const char* sens_names[] = { "Fine", "Normal", "Coarse" };

  switch (action) {
    case item_action_display:
      display.print(sens_names[menu_wheel_sensitivity]);
      break;
    case item_action_change:
      menu_wheel_sensitivity = (menu_wheel_sensitivity + 1) % 3;
      break;
    case item_action_change_prev:
      menu_wheel_sensitivity = (menu_wheel_sensitivity + 2) % 3;
      break;
    case item_action_reset: {
      PerModeQuickSettings settings;
      readPerModeQuickSettings(deviceMode, settings);
      menu_wheel_sensitivity = settings.wheel_sensitivity;
      break;
    }
    case item_action_apply:
      break;
    case item_action_save:
      break;
  }
}

}
