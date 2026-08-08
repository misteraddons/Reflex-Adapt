#include "input_intellivision_runtime_state.h"

namespace {
constexpr uint8_t kIntellivisionPorts = 2;
}

uint8_t intellivision_last_state[kIntellivisionPorts] = { 0 };
