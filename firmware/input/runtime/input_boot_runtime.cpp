#include "../../product_config.h"

#include "input_boot_runtime.h"

#include <Arduino.h>

#if defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
  #include "hardware/watchdog.h"
#endif

#include "../../core/device_runtime_state.h"
#include "../../core/firmware_support.h"
#include "../../core/settings_store.h"
#include "../../firmware_platform_config.h"
#include "../../platform/display_runtime_state.h"
#include "../../platform/boot/boot_ui_runtime.h"
#include "../autodetect/input_autodetect_benchmark.h"
#include "../../output/auth/auth_status.h"
#include "../../output/output_capabilities.h"
#include "../../output/output_runtime_state.h"
#include "../factory/input_adapter_factory.h"
#include "input_adapter_runtime.h"
#include "input_frame_runtime.h"
#include "input_runtime_bridge.h"
#include "../autodetect/input_autodetect_runtime.h"

namespace {

bool isFastSetupInputMode() {
  #if defined(ENABLE_INPUT_PSX)
  if (deviceMode == RZORD_PSX) {
    return true;
  }
  if (deviceMode == RZORD_PSX_JOG ||
      deviceMode == RZORD_PSX_DANCE) {
    return true;
  }
  #endif
  return false;
}

uint32_t getPostSetupInputSettleMs() {
  if (isFastSetupInputMode()) {
    return 5;
  }
  return 50;
}

void updateBootModuleDescriptionDisplay() {
  #ifdef USE_I2C_DISPLAY
    if (!hasActiveInputAdapter()) {
      Wire.setSDA(OLED_I2C_SDA);
      Wire.setSCL(OLED_I2C_SCL);
      Wire.begin();
      display.clear();
      display.print(F("Input module is null"));
      panic("current input module is null");
    }
    if (isBootSplashScreenVisible()) {
      return;
    }

    Wire.setSDA(OLED_I2C_SDA);
    Wire.setSCL(OLED_I2C_SCL);
    Wire.begin();

    const char* desc = activeInputAdapterDescriptionOr("ERROR");
    uint8_t str_pixel_count = strlen(desc) * 6;
    uint8_t margin = (str_pixel_count < 128) ? ((128 - str_pixel_count) / 2) : 0;
    bool keepBootStatusLine = false;
    #ifdef ENABLE_INPUT_AUTODETECT
    keepBootStatusLine = (deviceMode == RZORD_AUTODETECT);
    #endif
    bool descLooksInvalid = (strcmp(desc, "ERROR") == 0) || (strcmp(desc, "MODE ERROR") == 0);
    if (!keepBootStatusLine && !descLooksInvalid) {
      display.clear(0, 127, 2, 2);
      display.setCol(margin);
      display.setRow(2);
      display.println(desc);
    }
    Wire.end();
  #endif
}

uint32_t getInitialPollingDuration() {
  uint32_t initialPolling = 200;
  #if defined(ENABLE_INPUT_PSX)
  if (deviceMode == RZORD_PSX) {
    initialPolling = 25;
  } else if (deviceMode == RZORD_PSX_JOG ||
             deviceMode == RZORD_PSX_DANCE) {
    initialPolling = 25;
  }
  #endif
  #if defined(ENABLE_INPUT_USB)
  if (deviceMode == RZORD_USB)
    initialPolling = 3000;
  #endif
  #if defined(ENABLE_INPUT_GBA)
    if (deviceMode == RZORD_GBA)
      initialPolling = 3000;
  #endif
  return initialPolling;
}

void runInitialModulePolling() {
  resetInputPollSchedule();
  uint32_t pollingStartedAt = millis();
  uint32_t initialPolling = getInitialPollingDuration();
  while (millis() - pollingStartedAt < initialPolling) {
    pollInputModuleIfDue();
  }
}

void applyReservedInputPortCountForDescriptorReenum() {
  const uint8_t reservedPortCount = inputReenumReservedPortCount();
  if (reservedPortCount != 0 && reservedPortCount > inputPortCount()) {
    setInputPortCount(reservedPortCount);
  }
  clearInputReenumReservedPortCount();
}

void clearPassiveModeScratchRegisters() {
  watchdog_hw->scratch[PASSIVE_MODE_SCRATCH_MAGIC] = 0;
  watchdog_hw->scratch[PASSIVE_MODE_SCRATCH_MODE] = 0;
}

void restorePassiveInputModeFromScratchRegisters() {
  if (watchdog_hw->scratch[PASSIVE_MODE_SCRATCH_MAGIC] != PASSIVE_MODE_MAGIC) {
    return;
  }

  DeviceEnum passiveMode = (DeviceEnum)watchdog_hw->scratch[PASSIVE_MODE_SCRATCH_MODE];
  if (passiveMode > RZORD_NONE && passiveMode < RZORD_LAST) {
    deviceMode = passiveMode;
    savedDeviceMode = deviceMode;
  }

  clearPassiveModeScratchRegisters();
}

#ifdef ENABLE_INPUT_AUTODETECT
void clearAutoInputScratchRegisters() {
  watchdog_hw->scratch[AUTO_INPUT_MODE_SCRATCH_MAGIC] = 0;
  watchdog_hw->scratch[AUTO_INPUT_MODE_SCRATCH_MODE] = 0;
}

void restoreChainedAutoInputModeFromScratchRegisters() {
  if (watchdog_hw->scratch[AUTO_INPUT_MODE_SCRATCH_MAGIC] != AUTO_INPUT_MODE_MAGIC) {
    return;
  }

  const uint32_t scratchValue = watchdog_hw->scratch[AUTO_INPUT_MODE_SCRATCH_MODE];
  DeviceEnum chainedMode = (DeviceEnum)(scratchValue & 0xFFu);
  const uint8_t scratchSource = (uint8_t)((scratchValue >> 8u) & 0xFFu);
  const uint8_t scratchPortCount = min(
      (uint8_t)((scratchValue >> 16u) & 0xFFu),
      (uint8_t)MAX_USB_OUT);
  const uint8_t packedDetectedPort = (uint8_t)((scratchValue >> 24u) & 0xFFu);
  const uint8_t detectedPort =
      (packedDetectedPort >= 1u && packedDetectedPort <= 2u)
          ? (uint8_t)(packedDetectedPort - 1u)
          : 0xFF;
  setInputReenumReservedPortCount(scratchPortCount);
  if (chainedMode == RZORD_AUTODETECT &&
      scratchSource == kAutoInputResolveSourceAutoHomeReenum) {
    deviceMode = RZORD_AUTODETECT;
    savedDeviceMode = RZORD_AUTODETECT;
    setInputAutoDetectModeActive(true);
    clearInputAutoDetectBootPending();
    markInputHotSwapRevertedToAutoWhileDisconnected();
    recordInputAutoResolve((uint8_t)chainedMode, scratchSource, detectedPort);
    if (detectedPort != 0xFF) {
      input_autodetect_last_port = detectedPort;
    }
    autoDetectBenchmarkMark(ADBENCH_BOOT_AUTO_HOME_RESTORE, scratchSource);
    suppressBootAutoDetectSplashOnce();
    suppressBootUsbDebugInfoOnce();
  } else if (chainedMode > RZORD_NONE &&
      chainedMode < RZORD_LAST &&
      chainedMode != RZORD_AUTODETECT) {
    deviceMode = chainedMode;
    setInputAutoDetectModeActive(savedDeviceMode == RZORD_AUTODETECT);
    recordInputAutoResolve((uint8_t)chainedMode, 3, detectedPort);
    if (detectedPort != 0xFF) {
      input_autodetect_last_port = detectedPort;
    }
    autoDetectBenchmarkMark(ADBENCH_BOOT_INPUT_RESTORE, scratchSource, chainedMode);
    setAutoInputResolvedGraceUntil(millis() + autoInputResolvedGraceMs(chainedMode));
    suppressBootUsbDebugInfoOnce();
  }

  clearAutoInputScratchRegisters();
}
#endif

}  // namespace

void restoreInputModeFromScratchRegisters() {
  restorePassiveInputModeFromScratchRegisters();
  #ifdef ENABLE_INPUT_AUTODETECT
  restoreChainedAutoInputModeFromScratchRegisters();
  #endif
}

bool shouldSetupInputUsbBridgeForBoot(bool outputUsbReady) {
  // Classic2USB has no secondary USB-host input bridge.
  (void)outputUsbReady;
  return false;
}

void initializeInputModuleForBoot() {
  createActiveInputAdapter();
  loadPerModeSettings();
  updateBootModuleDescriptionDisplay();
  setupActiveInputAdapter();
  delay(getPostSetupInputSettleMs());
  runInitialModulePolling();
  applyReservedInputPortCountForDescriptorReenum();
  setupActiveInputAdapterStage2();
}

void initializeInputModuleForRuntimeModeChange() {
  createActiveInputAdapter();
  loadPerModeSettings();
  setupActiveInputAdapter();
  delay(getPostSetupInputSettleMs());
  runInitialModulePolling();
  applyReservedInputPortCountForDescriptorReenum();
  setupActiveInputAdapterStage2();
}

void maybeSetupInputUsbBridge(bool outputUsbReady) {
  (void)outputUsbReady;
}
