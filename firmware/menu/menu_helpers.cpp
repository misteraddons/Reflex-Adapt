#include "../product_config.h"

#include "menu_helpers.h"

#include "menu_catalog.h"

void menuResetAllItems() {
  for (uint8_t i = 0; i < MENU_TOTAL_ITEMS; ++i) {
    if (menu_items[i].item > menu_item_reserved_internal) {
      handleMenuItemAction(menu_items[i].item, item_action_reset);
    }
  }
}

void menuSaveAllItems() {
  for (uint8_t i = 0; i < MENU_TOTAL_ITEMS; ++i) {
    if (menu_items[i].item > menu_item_reserved_internal) {
      if (menu_items[i].item == menu_item_input_mode ||
          menu_items[i].item == menu_item_output_mode ||
          is_per_input_quick_setting(menu_items[i].item)) {
        continue;
      }
      handleMenuItemAction(menu_items[i].item, item_action_save);
    }
  }
}
