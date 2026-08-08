#include "menu_ui_state.h"

bool sound_submenu_active = false;
uint8_t sound_submenu_cursor = 0;
bool games_submenu_active = false;
uint8_t games_submenu_cursor = 0;
bool screensaver_submenu_active = false;
uint8_t screensaver_submenu_cursor = 0;
bool hotkeys_submenu_active = false;
uint8_t hotkeys_submenu_cursor = 0;
bool hotkeys_capture_active = false;
bool kiosk_submenu_active = false;
uint8_t kiosk_submenu_selection = 0;
bool padTestActive = false;
bool padTestInitialized = false;
bool analogTestActive = false;
bool analogTestInitialized = false;
uint8_t menu_navigation_state = 255;
uint8_t menu_selected_visible = 0;
uint8_t menu_scroll_offset = 0;
bool menu_bottom_right_selected = false;
bool buttonRemapActive = false;
bool mappingDisplayActive = false;
bool pinDebugActive = false;
bool pinDebugInitialized = false;
uint8_t factory_reset_stage = 0;
uint8_t factory_reset_selection = 0;
uint8_t bootloader_stage = 0;
bool about_screen_active = false;
bool system_menu_only = true;
uint8_t screensaver_animation = 0;
bool isMenuOpen = false;
bool isQuickConfigOpen = false;
bool mainDisplayInitialized = false;
bool idleAnimationActive = false;
bool idleDimActive = false;

bool menuOwnsControllerInput() {
  return isMenuOpen ||
         isQuickConfigOpen ||
         buttonRemapActive ||
         mappingDisplayActive ||
         padTestActive ||
         analogTestActive ||
         pinDebugActive ||
         factory_reset_stage != 0 ||
         bootloader_stage != 0 ||
         about_screen_active;
}
