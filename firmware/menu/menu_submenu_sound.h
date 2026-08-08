#pragma once

#include <stdint.h>

// Returns true if the sound submenu consumed input and drew its own frame.
bool handleSoundSubmenu(
    uint8_t& sound_submenu_cursor,
    bool& sound_submenu_active,
    uint8_t& menu_buzzer_mode,
    uint16_t& menu_sound_events,
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
