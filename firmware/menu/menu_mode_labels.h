#pragma once

#include "../core/device_mode.h"
#include "../output/output_runtime_state.h"

const char* getInputShortName(DeviceEnum mode);
const char* getInputCompactName(DeviceEnum mode);
const char* getOutputShortName(outputMode_t mode);
const char* getOutputCompactName(outputMode_t mode);
const char* getVirtualOutputPadName(DeviceEnum inputMode, outputMode_t mode);
outputMode_t getAutoDetectedOutputMode(AutoDetectState state);
