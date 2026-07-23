#include "input_wii_runtime_state.h"

namespace {
constexpr uint8_t kWiiPorts = 2;
}

uint32_t wii_last_digital_state[kWiiPorts] = { 0 };
uint64_t wii_last_analog_sticks_state[kWiiPorts] = { 0 };
uint64_t wii_last_analog_buttons_state[kWiiPorts] = { 0 };
