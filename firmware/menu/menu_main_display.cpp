#include "../product_config.h"

#include "menu.h"
#include "menu_main_display_internal.h"
#include "pad_test_runtime.h"
#include "screensaver_rflx_bitmap.h"

#include "../core/rumble_test_runtime.h"
#include "../platform/boot/boot_ui_runtime.h"

namespace {

constexpr uint8_t kScreensaverDimContrast = 1;
constexpr uint8_t kHomeLogoHeight = 16;
constexpr uint8_t kHomeLogoPages = kHomeLogoHeight / 8;
constexpr uint8_t kHomeLogoStartCol =
  (128 - kRflxScreensaverLogoWidth) / 2;
constexpr uint8_t kRflxLogoSourceBytesPerRow =
  (kRflxScreensaverLogoWidth + 7) / 8;

bool homeLogoPixel(uint8_t x, uint8_t y) {
  const uint8_t sourceY = (uint8_t)(
    ((uint16_t)y * (kRflxScreensaverLogoHeight - 1) +
     ((kHomeLogoHeight - 1) / 2)) /
    (kHomeLogoHeight - 1));
  const uint16_t sourceIndex =
    (uint16_t)sourceY * kRflxLogoSourceBytesPerRow + (x / 8);
  const uint8_t sourceByte = pgm_read_byte(
    &kRflxScreensaverLogoBitmap[sourceIndex]);
  return (sourceByte & (uint8_t)(1U << (x & 7))) != 0;
}

void drawHomeRflxLogo() {
  for (uint8_t page = 0; page < kHomeLogoPages; ++page) {
    display.setCursor(kHomeLogoStartCol, page);
    for (uint8_t x = 0; x < kRflxScreensaverLogoWidth; ++x) {
      uint8_t pageBits = 0;
      for (uint8_t bit = 0; bit < 8; ++bit) {
        const uint8_t y = (uint8_t)(page * 8 + bit);
        if (homeLogoPixel(x, y)) {
          pageBits |= (uint8_t)(1U << bit);
        }
      }
      display.ssd1306WriteRamBuf(pageBits);
    }
  }
  display.flush();
}

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
  static bool lastShowNoHostController = false;
  static bool firstHardwareClear = true;
  const bool showNoHostController = noHostControllerTestActive();
  const bool keepBootSplashActive =
    !showNoHostController &&
    isBootSplashScreenVisible() && isBootAutoDetectPending();

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
    const bool enteringVisibleScreensaver =
      !idleAnimationActive && !idleDimActive;
    if (enteringVisibleScreensaver) {
      rumbleRuntimeSetHostFeedbackSuppressed(true);
    }
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
                   || lastShowNoHostController != showNoHostController
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

  if (menu_main_display_internal::didControllerNamesChange()) {
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
  lastShowNoHostController = showNoHostController;

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

  drawHomeRflxLogo();

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
