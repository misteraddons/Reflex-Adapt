#pragma once

#include <stdint.h>

bool handleHotkeysSubmenu(
    uint8_t& hotkeys_submenu_cursor,
    bool& hotkeys_submenu_active,
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
