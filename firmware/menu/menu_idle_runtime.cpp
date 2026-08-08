#include "menu_idle_runtime.h"

#include "../core/controller_frame_state.h"
#include "../core/controller_settings_state.h"
#include "../core/device_runtime_state.h"
#include "../input/autodetect/input_autodetect_runtime_state.h"
#include "../platform/display_runtime_state.h"
#include "menu_input.h"
#include "menu_display_state.h"
#include "menu_ui_state.h"
#include "menu_working_state.h"

#include <Arduino.h>

namespace {

bool isVisibleScreensaverActive() {
  return idleAnimationActive || idleDimActive;
}

bool shouldKeepIdleAwakeForAutoDetectScan() {
  #ifdef ENABLE_INPUT_AUTODETECT
  if (!isAutoDetectMode || deviceMode != RZORD_AUTODETECT) {
    return false;
  }

  for (uint8_t i = 0; i < max_devices && i < MAX_USB_OUT; ++i) {
    if (controllerFrameConst(i).connected) {
      return false;
    }
  }

  return true;
  #else
  return false;
  #endif
}

void restoreNormalIdleDisplayState() {
#ifdef USE_I2C_DISPLAY
  restoreOledPanelOrientation();
  u8g2.setDrawColor(1);
  display.invertDisplay(false);
  display.setInvertMode(false);
  display.setContrast(display_contrast);
#endif
}

}  // namespace

uint32_t getScreensaverTimeoutMs() {
  // menu_screensaver: 0=Off, 1-3=Dim 1/5/10m, 4-6=Anim 1/5/10m
  if (menu_screensaver == 0) return 0;

  uint8_t timeIndex = ((menu_screensaver - 1) % 3);
  switch (timeIndex) {
    case 0: return 60000;
    case 1: return 300000;
    case 2: return 600000;
    default: return 60000;
  }
}

bool isScreensaverDimMode() {
  return menu_screensaver >= 1 && menu_screensaver <= 3;
}

void resetIdleTimer() {
  lastActivityTime = millis();
  const bool wasVisibleIdle = idleAnimationActive || idleDimActive;
  if (idleAnimationActive) {
    idleAnimationActive = false;
    mainDisplayInitialized = false;
    padDisplayNeedsRedraw = true;
    needsU8g2Clear = true;
    resetAnimationState();
  }
  if (idleDimActive) {
    idleDimActive = false;
    mainDisplayInitialized = false;
    padDisplayNeedsRedraw = true;
  }
  if (wasVisibleIdle) {
    restoreNormalIdleDisplayState();
  }
}

bool handleIdleUiActivity(bool uiActivity) {
  if (!uiActivity) {
    return false;
  }

  const bool consumedWake = isVisibleScreensaverActive();
  resetIdleTimer();
  return consumedWake;
}

bool handleControllerIdleWakeActivity() {
  static bool initialized = false;
  static uint32_t lastWakeButtons[MAX_USB_OUT] = { 0 };
  static bool lastConnected[MAX_USB_OUT] = { false };

  bool newWakePress = false;
  bool connectionChanged = false;
  for (uint8_t i = 0; i < max_devices && i < MAX_USB_OUT; ++i) {
    const controller_state_t& frame = controllerFrameConst(i);
    const bool connected = frame.connected;
    const uint32_t wakeButtons = connected ? getScreensaverWakeButtons(frame) : 0;

    if (initialized && lastConnected[i] != connected) {
      connectionChanged = true;
    }
    if (initialized && (wakeButtons & ~lastWakeButtons[i]) != 0) {
      newWakePress = true;
    }

    lastConnected[i] = connected;
    lastWakeButtons[i] = wakeButtons;
  }

  if (!initialized) {
    initialized = true;
    return false;
  }

  return handleIdleUiActivity(newWakePress || connectionChanged);
}

bool isIdleTimeoutReached() {
  if (shouldKeepIdleAwakeForAutoDetectScan()) {
    return false;
  }

  uint32_t timeout = getScreensaverTimeoutMs();
  if (timeout == 0) return false;
  return (millis() - lastActivityTime) >= timeout;
}
