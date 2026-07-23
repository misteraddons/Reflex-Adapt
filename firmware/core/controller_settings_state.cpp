#include "controller_settings_state.h"
#include "classic_analog_range.h"

uint8_t deadzone_percent = 0;
uint8_t display_contrast = 128;
uint8_t stick_invert = 0;
uint8_t screensaver_mode = 4;
uint8_t spinner_speed = 2;
uint8_t button_map_mode = 1;
uint8_t n64_z_mode = 0;
uint8_t n64_analog_range = CLASSIC_ANALOG_RANGE_DEFAULT;
uint8_t n64_cstick_mode = 0;
uint8_t wii_analog_range = CLASSIC_ANALOG_RANGE_DEFAULT;
const uint8_t spinner_speed_mult[5] = { 1, 2, 4, 8, 16 };
const uint8_t driving_speed_mult[5] = { 2, 4, 8, 12, 16 };
const char* const spinner_speed_labels[5] = { "0.25x", "0.5x", "1x", "2x", "4x" };
uint8_t rumble_left[MAX_USB_OUT] = { 0 };
uint8_t rumble_right[MAX_USB_OUT] = { 0 };
uint8_t rumble_level = 3;
uint8_t snes_rumbletech_enabled = 0;
uint8_t trigger_mode = 0;
uint8_t buzzer_enabled = 1;
uint8_t led_mode = 0;
uint8_t led_brightness = 128;
uint8_t dpad_mode = 0;
bool nso_special = false;
uint8_t classic_dual_merge_enabled = 0;
