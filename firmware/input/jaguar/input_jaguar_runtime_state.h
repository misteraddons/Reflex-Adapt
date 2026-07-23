#pragma once

#include <stdint.h>

#include "../../firmware_platform_config.h"

extern bool jaguarRotaryActive;
extern bool jaguarRotaryActivePorts[MAX_USB_OUT];

void resetJaguarRotaryRuntimeState();
