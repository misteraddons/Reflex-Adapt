#pragma once

#include "../controller_frame_state.h"
#include "../mode_settings_crc.h"
#include "../../output/output_mode.h"

// Remaining single-translation-unit forwards needed by the implementation
// includes that still share static helpers inside firmware.cpp.

static_assert(RZORD_LAST > 1, "Define at least one INPUT mode");
static_assert(MAX_INPUT_MODES >= RZORD_LAST, "MAX_INPUT_MODES must cover all DeviceEnum values");
