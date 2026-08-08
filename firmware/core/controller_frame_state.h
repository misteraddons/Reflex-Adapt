#pragma once

#include "../firmware_platform_config.h"
#include "controller_state.h"

// Canonical transport-neutral controller frames produced by inputs and
// consumed by the controller core, UI, and output layers.
extern uint8_t max_devices;
extern controller_state_t controller_frames[MAX_USB_OUT];

inline controller_state_t& controllerFrame(uint8_t index) {
  return controller_frames[index];
}

inline const controller_state_t& controllerFrameConst(uint8_t index) {
  return controller_frames[index];
}
