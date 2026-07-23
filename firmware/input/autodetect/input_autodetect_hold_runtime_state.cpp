#include "input_autodetect_hold_runtime_state.h"

namespace {
constexpr uint8_t kAutoDetectHoldPorts = 2;
}

uint16_t passive_assisted_hold_last_mask[kAutoDetectHoldPorts] = { 0 };
uint32_t passive_assisted_hold_stable_since[kAutoDetectHoldPorts] = { 0 };
bool passive_assisted_hold_latched[kAutoDetectHoldPorts] = { false };

uint8_t jaguar_assist_hold_last_mask[kAutoDetectHoldPorts] = { 0 };
uint32_t jaguar_assist_hold_stable_since[kAutoDetectHoldPorts] = { 0 };
bool jaguar_assist_hold_latched[kAutoDetectHoldPorts] = { false };
