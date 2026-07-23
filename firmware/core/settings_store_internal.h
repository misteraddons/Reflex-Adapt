#pragma once

#include <stdint.h>

#include "device_mode.h"

void saveTurboSettingsForMode(DeviceEnum mode, const uint8_t* rates);
void loadStoredTurboRatesForMode(DeviceEnum mode, uint8_t* turbo_rates);
