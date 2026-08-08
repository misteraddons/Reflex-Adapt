#pragma once

#include <stdint.h>

#include "../../core/device_mode.h"

constexpr uint8_t INPUT_MIXED_PORT_COUNT = 2;

void clearInputMixedModeState();
void configureInputMixedAutoDetectModes(DeviceEnum port0Mode, DeviceEnum port1Mode);

bool inputMixedModeActive();
DeviceEnum inputMixedPortMode(uint8_t port);
bool inputMixedPortModeSupported(uint8_t port, DeviceEnum mode);
bool inputMixedModesNeedSupervisor(DeviceEnum port0Mode, DeviceEnum port1Mode);

