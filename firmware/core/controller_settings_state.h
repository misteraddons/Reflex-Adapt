#pragma once

#include <stdint.h>

#include "../firmware_platform_config.h"
#include "gamecube_switch_left_mode.h"

enum AnalogMouseMode : uint8_t {
  ANALOG_MOUSE_OFF = 0,
  ANALOG_MOUSE_LEFT = 1,
  ANALOG_MOUSE_RIGHT = 2,
};
constexpr uint8_t ANALOG_MOUSE_SPEED_MAX = 16;
constexpr uint8_t ANALOG_MOUSE_SPEED_DEFAULT = 4;
constexpr uint8_t ANALOG_MOUSE_SPEED_UNITY = 8;

extern uint8_t deadzone_percent;
extern uint8_t display_contrast;
extern uint8_t stick_invert;
extern uint8_t screensaver_mode;
extern uint8_t spinner_speed;
extern uint8_t button_map_mode;
extern uint8_t gamecube_l_switch_mode;
extern uint8_t analog_mouse_mode;
extern uint8_t analog_mouse_speed;
extern uint8_t n64_z_mode;
extern uint8_t n64_analog_range;
extern uint8_t n64_cstick_mode;
extern uint8_t wii_analog_range;
extern const uint8_t spinner_speed_mult[5];
extern const uint8_t driving_speed_mult[5];
extern const char* const spinner_speed_labels[5];
extern uint8_t rumble_left[MAX_USB_OUT];
extern uint8_t rumble_right[MAX_USB_OUT];
extern uint8_t rumble_level;
constexpr uint8_t RUMBLE_MODE_OFF = 0;
constexpr uint8_t RUMBLE_MODE_VARIABLE = 1;
constexpr uint8_t RUMBLE_MODE_MAX = 2;
extern uint8_t snes_rumbletech_enabled;
extern uint8_t trigger_mode;
constexpr uint8_t TRIGGER_MODE_ANALOG = 0;
constexpr uint8_t TRIGGER_MODE_DIGITAL = 1;
constexpr uint8_t TRIGGER_MODE_RSTICK = 2;
constexpr uint8_t TRIGGER_MODE_BOTH = 3;
constexpr uint8_t TRIGGER_DIGITAL_THRESHOLD = 100;
extern uint8_t buzzer_enabled;
extern uint8_t led_mode;
extern uint8_t led_brightness;
extern uint8_t dpad_mode;
extern bool nso_special;
extern uint8_t classic_dual_merge_enabled;
