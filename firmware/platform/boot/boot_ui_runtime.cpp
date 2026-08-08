#include "../../product_config.h"

#include "boot_ui_runtime.h"
#include "reflex_boot_logo_bitmap.h"

#include <Arduino.h>

#include "../../core/controller_frame_state.h"
#include "../../core/controller_settings_state.h"
#include "../../core/device_runtime_state.h"
#include "../../input/autodetect/input_autodetect_benchmark.h"
#include "../../input/autodetect/input_autodetect_runtime_state.h"
#include "../../input/runtime/input_frame_runtime.h"
#include "../../output/autodetect/output_autodetect_runtime.h"
#include "../../output/output_runtime_state.h"
#include "../buzzer.h"
#include "../display_runtime_state.h"

namespace {
constexpr uint16_t kBootJingleDelayMs = 1000;

bool bootSplashVisible = false;
bool bootUsbDebugInfoSuppressed = false;
bool bootAutoDetectSplashSuppressed = false;

}  // namespace

bool isBootAutoDetectPending() {
  const bool outputAutoPending =
    configuredOutputMode == OUTPUT_AUTO &&
    !auto_detect_is_final_state();

  return outputAutoPending;
}

void showBootSplashScreen() {
  autoDetectBenchmarkMark(ADBENCH_BOOT_SPLASH);
  if (bootAutoDetectSplashSuppressed) {
    bootAutoDetectSplashSuppressed = false;
    bootSplashVisible = false;
    return;
  }

  #ifdef USE_I2C_DISPLAY
    beginDisplayWire();
    u8g2.begin();
    u8g2.setI2CAddress(I2C_ADDRESS * 2);
    restoreOledPanelOrientation();
    u8g2.setDrawColor(1);
    u8g2.clearBuffer();
    u8g2.drawXBMP(0, 0, kReflexBootLogoWidth, kReflexBootLogoHeight, kReflexBootLogoBitmap);
    u8g2.sendBuffer();

    display.beginNoClear(&Adafruit128x64, I2C_ADDRESS);
    restoreOledPanelOrientation();
    display.invertDisplay(false);
    display.setInvertMode(false);
    display.setContrast(display_contrast);
    display.setFont(System5x7);
    bootSplashVisible = true;
    Wire.end();
  #endif
}

bool isBootSplashScreenVisible() {
  return bootSplashVisible;
}

void markBootSplashScreenConsumed() {
  bootSplashVisible = false;
}

void suppressBootUsbDebugInfoOnce() {
  bootUsbDebugInfoSuppressed = true;
}

void suppressBootAutoDetectSplashOnce() {
  bootAutoDetectSplashSuppressed = true;
}

void showBootTraceMarker(const char* marker) {
#if defined(ENABLE_BOOTTRACE_OLED) && defined(USE_I2C_DISPLAY)
  bootSplashVisible = false;
  beginDisplayWire();
  display.begin(&Adafruit128x64, I2C_ADDRESS);
  display.setContrast(255);
  display.setFont(System5x7);
  display.clear();
  display.set1X();
  display.set2X();
  display.setCol(40);
  display.print(F("BOOT"));
  display.set1X();
  display.setRow(3);
  display.setCol(0);
  display.print(marker);
  display.flush();
  Wire.end();
#else
  (void)marker;
#endif
}

void maybePlayResolvedBootJingle(bool autoOutputProbeBoot, bool autoOutputResolvedBoot) {
  if (canonicalizeOutputMode(get_effective_output_mode()) == OUTPUT_PS5) {
    return;
  }

  const bool inputModeResolvedAtBoot = (deviceMode != RZORD_AUTODETECT);
  const bool outputModeResolvedAtBoot =
    (configuredOutputMode != OUTPUT_AUTO) ||
    (autoDetectState != AUTO_STATE_IDLE) ||
    autoOutputResolvedBoot;
  if (inputModeResolvedAtBoot && outputModeResolvedAtBoot && !autoOutputProbeBoot) {
    buzzer.playBootDelayed(kBootJingleDelayMs);
  }
}

void showBootUsbDebugInfo() {
  #ifdef USE_I2C_DISPLAY
  if (bootUsbDebugInfoSuppressed) {
    bootUsbDebugInfoSuppressed = false;
    return;
  }
  if (isBootSplashScreenVisible() || isBootAutoDetectPending()) {
    return;
  }

  beginDisplayWire();
  restoreOledPanelOrientation();
  display.setRow(6);
  display.setCol(0);
  display.print(F("USB:"));
  display.print(max_devices);
  display.print(F("/"));
  display.print(hid_begin_success_count);
  display.print(F(" T:"));
  display.print(inputPortCount());
  display.flush();
  Wire.end();
  #endif
}
