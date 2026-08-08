#include "menu_runtime_state.h"
#include "../core/classic_analog_range.h"
#include "../core/gamecube_switch_left_mode.h"
#include "../core/controller_settings_state.h"

int8_t menu_guncon_offset_x = 0;
int8_t menu_guncon_offset_y = 0;
uint8_t menu_powerpad_mode = 0;
uint8_t menu_jogcon_mode = 0;
uint8_t menu_jogcon_force = 15;
uint8_t menu_wheel_sensitivity = 1;
uint8_t menu_jogcon_digital_map = 0;
uint8_t menu_jogcon_wheel_axis = 0;
uint8_t menu_button_map = 1;
uint8_t menu_gamecube_l_switch_mode = GAMECUBE_L_SWITCH_ZL;
uint8_t menu_analog_mouse_mode = ANALOG_MOUSE_OFF;
uint8_t menu_analog_mouse_speed = ANALOG_MOUSE_SPEED_DEFAULT;
uint8_t menu_n64_z_mode = 1;
uint8_t menu_n64_cstick_mode = 1;
uint8_t menu_wii_analog_range = CLASSIC_ANALOG_RANGE_DEFAULT;
uint8_t menu_n64_analog_range = CLASSIC_ANALOG_RANGE_DEFAULT;
uint8_t menu_menu_hotkey = 0;
uint8_t menu_system_menu_hotkey = 0;
uint8_t menu_home_hotkey = 1;
uint8_t menu_capture_hotkey = 0;
uint8_t menu_kiosk_mode = 0;
uint32_t menu_hotkey_combo = 0;
uint32_t menu_system_hotkey_combo = 0;
uint32_t menu_home_hotkey_combo = 0;
uint32_t menu_capture_hotkey_combo = 0;
uint8_t menu_win_output = 0;
uint8_t menu_mister_output = 0;
uint8_t menu_snes_rumbletech = 0;
uint8_t menu_home_screen_debug = 0;
uint8_t menu_home_button_labels = 0;
uint8_t menu_hotkey_hold_time = 1;
uint8_t menu_classic_dual_merge = 0;
uint8_t menu_usb_descriptor_reboot_required = 0;
