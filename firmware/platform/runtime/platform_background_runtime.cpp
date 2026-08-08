#include "../../product_config.h"

#include "platform_background_runtime.h"

#include "../buzzer.h"
#include "../latency_test.h"
#include "../rgb_led.h"
#include "../../core/rumble_test_runtime.h"
#include "../../output/output_runtime_state.h"

void runPlatformBackgroundTasks() {
  static bool ps5QuietWasActive = false;
  if (is_ps5_timing_quiet_mode_active()) {
    // P5General/auth is sensitive to unrelated timing work. Keep PWM/PIO LED
    // and latency bookkeeping out of the PS5 console loop.
    if (!ps5QuietWasActive) {
      buzzer.stop();
      ps5QuietWasActive = true;
    }
    return;
  }

  ps5QuietWasActive = false;
  buzzer.update();
  latencyTest.update();
  rumbleTestUpdate();

  #ifdef USE_WS2812
  rgbLed.update();
  #endif
}
