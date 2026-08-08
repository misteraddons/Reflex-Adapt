#include "../product_config.h"

#include "menu_navigation.h"

#include "../platform/buzzer.h"
#include "menu_helpers.h"
#include "menu_mode_state.h"
#include "menu_ui_state.h"
#include "../output/output_runtime_state.h"

bool handleMenuValueChange(
    menu_item_enum current_item,
    menu_item_action action,
    uint8_t& selected_visible,
    uint8_t& scroll_offset,
    bool bottom_right,
    uint8_t item_rows
) {
  if (menu_input != deviceMode && is_mode_specific_setting(current_item)) {
    buzzer.playError();
    return true;
  }

  handleMenuItemAction(current_item, action);

  if (current_item == menu_item_factory_reset && factory_reset_stage == 1) {
    return true;
  }
  if (current_item == menu_item_bootloader && bootloader_stage == 1) {
    return true;
  }
  if (current_item == menu_item_about && about_screen_active) {
    return true;
  }

  if (current_item == menu_item_output_mode || current_item == menu_item_input_mode) {
    buzzer.playModeChange();
  } else if (current_item != menu_item_factory_reset &&
             current_item != menu_item_bootloader &&
             current_item != menu_item_pad_test &&
             current_item != menu_item_about) {
    buzzer.playMenuNav();
  }

  if (current_item == menu_item_output_mode || current_item == menu_item_input_mode) {
    uint8_t new_visible_count = getVisibleItemCount();
    if (selected_visible >= new_visible_count)
      selected_visible = new_visible_count - 1;
    if (scroll_offset + item_rows > new_visible_count)
      scroll_offset = (new_visible_count > item_rows) ? new_visible_count - item_rows : 0;
  }

  renderMenu(selected_visible, scroll_offset, bottom_right);
  return false;
}
