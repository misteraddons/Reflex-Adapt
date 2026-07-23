#pragma once

#include <stdint.h>

#include "../../core/device_mode.h"
#include "../output_mode.h"

typedef struct {
  bool autoOutputProbeBoot;
  bool autoOutputResolvedBoot;
} auto_output_boot_state_t;

auto_output_boot_state_t restoreAutoOutputBootState();
void restoreLockedOutputModeAfterInputSetup(bool outputModeLockedAtBoot, outputMode_t bootOutputMode);
outputMode_t finalizeBootOutputMode();
uint8_t bootUsbSlotCountForModeAndInput(outputMode_t mode, DeviceEnum inputMode, uint8_t portCount);
void connectConfiguredOutputUsb();
