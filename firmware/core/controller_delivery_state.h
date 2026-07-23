#pragma once

#include <cstdint>

#include "../firmware_platform_config.h"

// Delivery bookkeeping stays separate from controller data so the shared
// controller frame shape remains transport-neutral.
extern bool controller_frame_needs_delivery[MAX_USB_OUT];

inline bool controllerFrameNeedsDelivery(uint8_t index) {
  return controller_frame_needs_delivery[index];
}

inline void requestControllerFrameDelivery(uint8_t index) {
  controller_frame_needs_delivery[index] = true;
}

inline void completeControllerFrameDelivery(uint8_t index) {
  controller_frame_needs_delivery[index] = false;
}
