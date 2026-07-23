#pragma once

#include "device_mode.h"

// Shared mutable input-mode runtime state. Keeping this separate from the enum
// header lets callers opt into the live mode globals explicitly.
extern DeviceEnum deviceMode;
extern DeviceEnum savedDeviceMode;
