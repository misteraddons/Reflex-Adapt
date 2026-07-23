#include "input_port_scan_runtime_state.h"

namespace {
constexpr uint8_t kInputPortScanTimestampSlots = 6;
}

uint32_t input_empty_port_last_tested_at = 0;
uint32_t input_empty_port_tested_port_timestamp[kInputPortScanTimestampSlots] = { 0 };
