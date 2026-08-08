#include "../../product_config.h"

#include "input_mode_runtime.h"

#include <Arduino.h>

#include "../../core/device_runtime_state.h"
#include "../../core/firmware_support.h"
#include "../../core/settings_store.h"
#include "../../firmware_platform_config.h"
#include "../../menu/menu_input_mode.h"
#include "../../menu/menu_mode_labels.h"
#include "../../output/output_runtime_state.h"
#include "../../platform/buzzer.h"
#include "../../platform/display_runtime_state.h"
#include "../autodetect/input_autodetect_runtime.h"
#include "../autodetect/input_autodetect_runtime_state.h"

bool handlePendingAutoDetectRebootRelease() {
  #ifdef ENABLE_INPUT_AUTODETECT
  if (inputAutoDetectRebootPending() && !resetButton.isPressed()) {
    auto_detect_preserve_resolved_runtime_mode_for_reboot();
    reboot();
    return true;
  }
  #endif
  return false;
}

void armAutoDetectReboot() {
#ifdef ENABLE_INPUT_AUTODETECT
  deviceMode = RZORD_AUTODETECT;
  savedDeviceMode = RZORD_AUTODETECT;
  setInputAutoDetectModeActive(true);
  clearAutoDetectResult();
  persistConfiguredInputMode(savedDeviceMode);
  setPendingAutoDetectReboot(true);
#endif
}

void cycleInputMode() {
  DeviceEnum newMode = deviceMode;
  do {
    newMode = (DeviceEnum)(newMode + 1);
    if (newMode >= RZORD_LAST) {
      newMode = (DeviceEnum)1;
    }
  } while (should_hide_input_mode(newMode));

  persistConfiguredInputMode(newMode);

  #ifdef USE_I2C_DISPLAY
    Wire.setSDA(OLED_I2C_SDA);
    Wire.setSCL(OLED_I2C_SCL);
    Wire.begin();
    display.clear();
    display.set2X();
    display.println(F("Input Mode"));
    display.println();
    display.print(F("-> "));
    display.println(getInputShortName(newMode));
    display.set1X();
    display.println();
    display.println(F("Rebooting..."));
    Wire.end();
  #endif

  buzzer.playModeChange();
  waitWithBuzzerUpdates(1000);
  #ifdef ENABLE_INPUT_AUTODETECT
    if (newMode == RZORD_AUTODETECT) {
      auto_detect_preserve_known_runtime_mode_for_input_reboot();
    } else {
      if (auto_detect_preserve_known_runtime_mode_for_input_reboot()) {
        auto_detect_clear_input_scratch_state();
      } else {
        auto_detect_clear_scratch_state();
      }
    }
  #else
    auto_detect_clear_scratch_state();
  #endif
  reboot();
}
