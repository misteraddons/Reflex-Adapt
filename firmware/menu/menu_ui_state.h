#pragma once

#include <stdint.h>

extern bool sound_submenu_active;
extern uint8_t sound_submenu_cursor;
extern bool games_submenu_active;
extern uint8_t games_submenu_cursor;
extern bool screensaver_submenu_active;
extern uint8_t screensaver_submenu_cursor;
extern bool hotkeys_submenu_active;
extern uint8_t hotkeys_submenu_cursor;
extern bool hotkeys_capture_active;
extern bool kiosk_submenu_active;
extern uint8_t kiosk_submenu_selection;
extern bool padTestActive;
extern bool padTestInitialized;
extern bool analogTestActive;
extern bool analogTestInitialized;
extern uint8_t menu_navigation_state;
extern uint8_t menu_selected_visible;
extern uint8_t menu_scroll_offset;
extern bool menu_bottom_right_selected;
extern bool buttonRemapActive;
extern bool mappingDisplayActive;
extern bool pinDebugActive;
extern bool pinDebugInitialized;
extern uint8_t factory_reset_stage;
extern uint8_t factory_reset_selection;
extern uint8_t bootloader_stage;
extern bool about_screen_active;
extern bool system_menu_only;
extern uint8_t screensaver_animation;
extern bool isMenuOpen;
extern bool isQuickConfigOpen;
extern bool mainDisplayInitialized;
extern bool idleAnimationActive;
extern bool idleDimActive;

bool menuOwnsControllerInput();
