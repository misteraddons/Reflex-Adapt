#pragma once

#include <stdint.h>

// Input-owned polling/runtime helpers that bridge active adapter polling into
// the controller pipeline and platform UI.

void resetInputPollSchedule();
bool pollInputModuleIfDue(bool* updated = nullptr);
bool pollInputModuleIfDue(uint32_t& last_poll_at_us, bool* updated = nullptr);
bool runInputRuntimeCycle(bool* polled = nullptr, bool* updated = nullptr);
