#pragma once

#include <stdint.h>

bool handleKioskSubmenu(
    uint8_t& kiosk_submenu_selection,
    bool& kiosk_submenu_active,
    uint8_t selected_visible,
    uint8_t scroll_offset,
    bool bottom_right,
    bool ctrlDownJust,
    bool ctrlUpJust,
    bool ctrlLeftJust,
    bool ctrlAJust,
    bool ctrlRightJust,
    bool ctrlBJust,
    bool btnNavigateJustPressed,
    bool btnChangeJustPressed);
