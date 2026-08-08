#pragma once

#include "../../firmware_platform_config.h"
#include "../../product_config.h"
#include "../../core/controller_settings_state.h"
#include "../../core/device_runtime_state.h"

inline uint8_t output_usb_player_count() {
  uint8_t count = max_devices;
  if (count == 0) {
    count = 1;
  }

#if defined(PRODUCT_CLASSIC2USB)
  // 2P Merge still reads both physical controllers, but the host should see
  // one output player so the merged frame lands on a single gamepad endpoint.
  if (classic_dual_merge_enabled) {
    count = 1;
  }
#endif

  return (count > MAX_USB_OUT) ? MAX_USB_OUT : count;
}
