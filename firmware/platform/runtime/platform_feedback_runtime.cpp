#include "../../product_config.h"

#include "platform_feedback_runtime.h"

#include <Arduino.h>
#include <string.h>

#include "../../core/controller_frame_state.h"
#include "../../core/device_runtime_state.h"
#include "../../core/controller_settings_state.h"
#include "../../features/feature_module.h"
#include "../../firmware_platform_config.h"
#include "../../menu/menu_bridge.h"
#include "../../menu/menu_ui_state.h"
#include "../../output/output_runtime_state.h"
#include "../display_runtime_state.h"
#include "../rgb_led.h"
#include "../webhid_runtime.h"

#if defined(ENABLE_USB_AUTH_SIDECAR) || defined(ENABLE_USB_AUTH_SIDECAR_CORE1)
#include "../../input/usb_host/input_usb_host_service.h"
#include "../../output/auth/ps_auth_dongle_runtime.h"
#endif

namespace {

uint32_t last_display_update = 0;

void recordWebHidInputStats() {
  const controller_state_t& primary = controllerFrameConst(0);
  webhid_record_input(
    (max_devices > 0) ? primary.digital_buttons : 0,
    (max_devices > 0) ? primary.LX : 0,
    (max_devices > 0) ? primary.LY : 0,
    (max_devices > 0) ? primary.RX : 0,
    (max_devices > 0) ? primary.RY : 0
  );
}

void updateRgbLedFromInput(bool updated) {
  #ifdef USE_WS2812
  if (!updated || led_mode == LED_MODE_OFF) {
    return;
  }

  for (uint8_t i = 0; i < max_devices; ++i) {
    controller_state_t& frame = controllerFrame(i);
    if (!frame.connected) {
      continue;
    }

    if (led_mode == LED_MODE_REACTIVE) {
      rgbLed.triggerButtons(frame.digital_buttons);
    }
    if (led_mode == LED_MODE_ANALOG) {
      uint8_t lx_scaled = (frame.LX + 32768) >> 8;
      uint8_t ly_scaled = (frame.LY + 32768) >> 8;
      rgbLed.setAnalog(lx_scaled, ly_scaled);
    }
    if (led_mode == LED_MODE_RUMBLE) {
      uint8_t rumble_max = (rumble_left[i] > rumble_right[i]) ? rumble_left[i] : rumble_right[i];
      rgbLed.setRumble(rumble_max);
    }
    break;
  }
  #else
  (void)updated;
  #endif
}

void renderMainDisplayIfNeeded() {
  #ifdef USE_I2C_DISPLAY
    // Keep idle animations on the normal home-display cadence. A previous
    // 1 ms animation path could saturate I2C updates enough to destabilize USB.
    constexpr uint32_t kOledFrameIntervalMs = 16;
    #ifdef ENABLE_INPUT_JVS
    if (deviceMode == RZORD_JVS && !idleAnimationActive) {
      // In JVS mode the donor firmware only touches the OLED in the safe window
      // between host() and sync(). Defer shared home-screen redraws to that
      // path so I2C writes do not steal time from UART RX mid-transaction.
      return;
    }
    #endif
    if (!isMenuOpen && !isQuickConfigOpen &&
        millis() - last_display_update >= kOledFrameIntervalMs) {
      beginDisplayWire();
      if (!featureModulesRenderOledTransient()) {
        renderMainDisplay();
      }
      Wire.end();
      last_display_update = millis();
    }
  #endif
}

void renderJvsSafeWindowDisplayIfNeededInternal() {
  #ifdef USE_I2C_DISPLAY
  #ifdef ENABLE_INPUT_JVS
  if (deviceMode != RZORD_JVS || idleAnimationActive || isMenuOpen || isQuickConfigOpen) {
    return;
  }

  constexpr uint32_t kJvsDisplayIntervalMs = 50;
  if (millis() - last_display_update < kJvsDisplayIntervalMs) {
    return;
  }

  beginDisplayWire();
  if (!featureModulesRenderOledTransient()) {
    renderMainDisplay();
  }
  Wire.end();
  last_display_update = millis();
  #endif
  #endif
}

}  // namespace

void runPlatformFeedbackServices(bool updated) {
  if (is_ps5_timing_quiet_mode_active()) {
    return;
  }

  recordWebHidInputStats();
  updateRgbLedFromInput(updated);
  renderMainDisplayIfNeeded();
}

void runJvsSafeWindowDisplayIfNeeded() {
  renderJvsSafeWindowDisplayIfNeededInternal();
}
