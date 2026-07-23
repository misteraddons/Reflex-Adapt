#include "../product_config.h"
#include "../product_display_identity.h"

#include "menu.h"
#include "menu_main_display_internal.h"

#include "../platform/boot/boot_ui_runtime.h"

#include <cstring>

namespace {

constexpr uint8_t kScreensaverDimContrast = 1;

void applyHomeDisplayContrast() {
  display.setContrast(idleDimActive ? kScreensaverDimContrast : display_contrast);
}

void flushHomeDisplay() {
  display.flush();
}

}  // namespace

void renderMainDisplay() {
#ifndef USE_I2C_DISPLAY
  mainDisplayInitialized = false;
  padDisplayNeedsRedraw = false;
  needsU8g2Clear = false;
  return;
#else
  static DeviceEnum lastInputMode = RZORD_NONE;
  static outputMode_t lastOutputMode = OUTPUT_LAST;
  static bool lastConnected[MAX_USB_OUT] = { false };
  static bool lastShowScanning = false;
  static bool firstHardwareClear = true;
  const bool keepBootSplashActive = isBootSplashScreenVisible() && isBootAutoDetectPending();

  if (keepBootSplashActive) {
    return;
  }

  if (needsU8g2Clear || firstHardwareClear) {
    needsU8g2Clear = false;
    firstHardwareClear = false;
    beginDisplayWire();
    u8g2.begin();
    u8g2.setI2CAddress(I2C_ADDRESS * 2);
    restoreOledPanelOrientation();
    u8g2.setDrawColor(1);
    u8g2.clearBuffer();
    u8g2.sendBuffer();
    display.begin(&Adafruit128x64, I2C_ADDRESS);
    restoreOledPanelOrientation();
    display.clear();
    display.invertDisplay(false);
    display.setInvertMode(false);
    applyHomeDisplayContrast();
    flushHomeDisplay();
    mainDisplayInitialized = false;
    padDisplayNeedsRedraw = true;
  }

  bool connectionStateChanged = false;
  for (uint8_t i = 0; i < max_devices && i < MAX_USB_OUT; ++i) {
    const bool connected = controllerFrameConst(i).connected;
    if (lastConnected[i] != connected) {
      connectionStateChanged = true;
      lastConnected[i] = connected;
    }
  }

  if (isIdleTimeoutReached()) {
    if (isScreensaverDimMode()) {
      if (!idleDimActive) {
        idleDimActive = true;
        applyHomeDisplayContrast();
      }
    } else {
      renderSelectedAnimation();
      return;
    }
  }

  bool needsRedraw = !mainDisplayInitialized
                   || lastInputMode != deviceMode
                   || lastOutputMode != outputMode
                   || connectionStateChanged;

  uint8_t connectedCount = 0;
  for (uint8_t i = 0; i < max_devices && i < MAX_USB_OUT; ++i) {
    if (controllerFrameConst(i).connected) {
      connectedCount++;
    }
  }

  bool showScanning = menu_main_display_internal::shouldShowAutoDetectScanning(connectedCount);
  if (lastShowScanning != showScanning) {
    needsRedraw = true;
  }

  if (menu_main_display_internal::didPrimaryControllerNameChange()) {
    needsRedraw = true;
  }

  if (!needsRedraw) {
    if (mainDisplayInitialized && !showScanning) {
      renderHomeModeLine(false);
      menu_main_display_internal::renderModeButtonIndicator();
      if (menu_main_display_internal::shouldShowXinputMultiDiagOverlay()) {
        menu_main_display_internal::renderXinputMultiDiagOverlay();
      } else {
        menu_main_display_internal::updateRealtimeButtons();
      }
      menu_main_display_internal::renderJvsRawDebugLine();
      flushHomeDisplay();
    }
    return;
  }

  mainDisplayInitialized = true;
  lastInputMode = deviceMode;
  lastOutputMode = outputMode;
  lastShowScanning = showScanning;

  display.begin(&Adafruit128x64, I2C_ADDRESS);
  restoreOledPanelOrientation();
  u8g2.setDrawColor(1);
  display.invertDisplay(false);
  display.setInvertMode(false);
  display.clear();
  applyHomeDisplayContrast();
  padDisplayNeedsRedraw = true;
  display.setFont(System5x7);
  display.set1X();

  display.set2X();
  display.setRow(0);
  const char* title = getProductHomeDisplayTitle();
  const uint8_t titleStart = (uint8_t)((128 - (std::strlen(title) * 12)) / 2);
  display.setCol(titleStart);
  display.print(title);
  display.set1X();

  renderHomeModeLine(showScanning, true);
  menu_main_display_internal::renderModeButtonIndicator(true);

  if (showScanning) {
    menu_main_display_internal::renderAutoDetectScanningStatus();
    flushHomeDisplay();
    markBootSplashScreenConsumed();
    return;
  }

  menu_main_display_internal::renderConnectedPortNames();

  if (menu_main_display_internal::shouldShowXinputMultiDiagOverlay()) {
    menu_main_display_internal::renderXinputMultiDiagOverlay(true);
  } else {
    menu_main_display_internal::updateRealtimeButtons();
  }
  menu_main_display_internal::renderJvsRawDebugLine(true);
  flushHomeDisplay();
  markBootSplashScreenConsumed();
#endif
}
