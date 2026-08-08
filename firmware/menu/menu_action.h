#pragma once

#include <stdint.h>

enum menu_item_action : uint8_t {
  item_action_display,
  item_action_change,       // Next value (right/A)
  item_action_change_prev,  // Previous value (left)
  item_action_reset,
  item_action_apply,
  item_action_save,
};
