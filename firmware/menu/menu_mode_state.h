#pragma once

#include "../core/device_mode.h"
#include "../output/output_mode.h"

extern DeviceEnum menu_input;
extern outputMode_t menu_output;

bool input_has_analog();
bool input_has_right_stick();
bool input_has_rumble();
bool is_passive_controller_mode(DeviceEnum mode);
bool input_has_analog_triggers();
bool output_has_rumble();
bool should_hide_output_mode(outputMode_t mode);
outputMode_t cycle_visible_output_mode(outputMode_t current, bool forward);
DeviceEnum getMenuSavedInputMode(DeviceEnum selectedMode);
