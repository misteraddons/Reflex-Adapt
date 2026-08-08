#pragma once

#include <stdint.h>

// Returns true if the screensaver submenu consumed input and drew its own frame.
bool handleScreensaverSubmenu(
    uint8_t& screensaver_submenu_cursor,
    bool& screensaver_submenu_active,
    uint8_t& menu_screensaver,
    uint8_t& menu_screensaver_anim,
    uint8_t selected_visible,
    uint8_t scroll_offset,
    bool bottom_right,
    bool ctrlDownJust,
    bool ctrlUpJust,
    bool ctrlLeftJust,
    bool ctrlAJust,
    bool ctrlRightJust,
    bool ctrlBJust,
    bool ctrlStartJust,
    bool btnNavigateJustPressed,
    bool btnChangeJustPressed);
