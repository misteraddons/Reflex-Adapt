#pragma once

#include <stdint.h>

// Returns true if the games submenu consumed input and drew its own frame.
bool handleGamesSubmenu(
    uint8_t& games_submenu_cursor,
    bool& games_submenu_active,
    uint8_t selected_visible,
    uint8_t scroll_offset,
    bool bottom_right,
    bool ctrlDownJust,
    bool ctrlUpJust,
    bool ctrlAJust,
    bool ctrlRightJust,
    bool ctrlBJust,
    bool btnNavigateJustPressed,
    bool btnChangeJustPressed);
