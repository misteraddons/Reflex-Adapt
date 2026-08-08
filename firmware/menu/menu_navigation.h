#pragma once

#include <stdint.h>

#include "menu_action.h"
#include "menu_item_defs.h"

bool handleMenuValueChange(
    menu_item_enum current_item,
    menu_item_action action,
    uint8_t& selected_visible,
    uint8_t& scroll_offset,
    bool bottom_right,
    uint8_t item_rows);
