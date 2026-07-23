#include "input_autodetect_passive_runtime_state.h"

namespace {
constexpr uint8_t kAutoDetectPassivePorts = 2;
}

uint16_t passive_single_step_prev_mask[kAutoDetectPassivePorts] = { 0 };
bool passive_single_step_armed[kAutoDetectPassivePorts] = { false };
