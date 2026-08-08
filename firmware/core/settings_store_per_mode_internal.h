#pragma once

#include <stdint.h>

#include "settings_store.h"

bool sanitizeTurboRatesForMode(DeviceEnum mode, uint8_t* turbo_rates);
uint8_t defaultTriggerModeForInputMode(DeviceEnum mode);
uint8_t defaultRumbleLevelForInputMode(DeviceEnum mode);
void selectPerModeRemapSlot(DeviceEnum mode);
bool loadAndApplyPerModeSettingsForMode(DeviceEnum mode);
bool loadTurboRatesForMode(DeviceEnum mode, uint8_t* turbo_rates);
bool loadRemapsForMode(DeviceEnum mode);
