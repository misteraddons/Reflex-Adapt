#include "../../product_config.h"
#include "../../firmware_platform_config.h"

#include <Arduino.h>
#include "hardware/watchdog.h"

#include "../../core/device_runtime_state.h"
#include "../../menu/menu_ui_state.h"
#include "../../output/runtime/input_runtime_output_bridge.h"
#include "input_autodetect_runtime.h"
#include "input_autodetect_runtime_state.h"

namespace {

constexpr uint32_t kAutoInputScratchModeMask = 0xFFu;
constexpr uint32_t kAutoInputScratchSourceShift = 8u;
constexpr uint32_t kAutoInputScratchPortCountShift = 16u;
constexpr uint32_t kAutoInputScratchDetectedPortShift = 24u;
constexpr uint8_t kAutoInputResolveSourceOutputReenum = 2;
constexpr uint8_t kAutoInputResolveSourceScratchRestore = 3;

bool isAutoDetectModeValue(DeviceEnum mode) {
#ifdef ENABLE_INPUT_AUTODETECT
  return mode == RZORD_AUTODETECT;
#else
  (void)mode;
  return false;
#endif
}

uint32_t makeAutoInputScratchValue(DeviceEnum mode,
                                   uint8_t source,
                                   uint8_t port_count,
                                   uint8_t detected_port = 0xFF) {
  const uint8_t packedDetectedPort =
      detected_port < 2 ? (uint8_t)(detected_port + 1u) : 0;
  return ((uint32_t)mode & kAutoInputScratchModeMask) |
         ((uint32_t)source << kAutoInputScratchSourceShift) |
         ((uint32_t)port_count << kAutoInputScratchPortCountShift) |
         ((uint32_t)packedDetectedPort << kAutoInputScratchDetectedPortShift);
}

}  // namespace

void scheduleResolvedAutoDetectRetry(uint32_t now) {
  scheduleInputHotSwapDetectAfter(now, inputAutoDetectRetryIntervalMs());
}

void scheduleUnresolvedAutoDetectRetry(uint32_t now) {
  scheduleInputHotSwapDetectAfter(now, inputAutoDetectUnresolvedRetryIntervalMs());
}

void preserveDetectedInputModeForReboot(DeviceEnum mode) {
  preserveInputModeForPlayerCountReboot(mode, 0);
}

void preserveInputModeForPlayerCountReboot(DeviceEnum mode, uint8_t port_count) {
  bool mode_valid = (mode > RZORD_NONE) && (mode < RZORD_LAST) && !isAutoDetectModeValue(mode);
  if (mode_valid) {
    if (port_count > MAX_USB_OUT) {
      port_count = MAX_USB_OUT;
    }
    watchdog_hw->scratch[AUTO_INPUT_MODE_SCRATCH_MAGIC] = AUTO_INPUT_MODE_MAGIC;
    watchdog_hw->scratch[AUTO_INPUT_MODE_SCRATCH_MODE] =
      makeAutoInputScratchValue(
        mode,
        kAutoInputResolveSourceOutputReenum,
        port_count,
        input_auto_resolve_port);
    recordInputAutoResolve(
      (uint8_t)mode,
      kAutoInputResolveSourceOutputReenum,
      input_auto_resolve_port);
  } else {
    watchdog_hw->scratch[AUTO_INPUT_MODE_SCRATCH_MAGIC] = 0;
    watchdog_hw->scratch[AUTO_INPUT_MODE_SCRATCH_MODE] = 0;
  }
}

void preserveAutoDetectHomeForReboot() {
  watchdog_hw->scratch[AUTO_INPUT_MODE_SCRATCH_MAGIC] = AUTO_INPUT_MODE_MAGIC;
  watchdog_hw->scratch[AUTO_INPUT_MODE_SCRATCH_MODE] =
    makeAutoInputScratchValue(RZORD_AUTODETECT, kAutoInputResolveSourceAutoHomeReenum, 0);
  recordInputAutoResolve((uint8_t)RZORD_AUTODETECT, kAutoInputResolveSourceAutoHomeReenum);
}

uint32_t autoInputResolvedGraceMs(DeviceEnum mode) {
  #ifdef ENABLE_INPUT_PSX
  if (mode == RZORD_PSX) {
    return 2000;
  }
  #endif
  #ifdef ENABLE_INPUT_PSX_DANCE
  if (mode == RZORD_PSX_DANCE) {
    return 2000;
  }
  #endif
  #ifdef ENABLE_INPUT_PSX_JOG
  if (mode == RZORD_PSX_JOG) {
    return 2000;
  }
  #endif
  return 2000;
}

void prepareAutoDetectInputModeAtBoot() {
  #ifdef ENABLE_INPUT_AUTODETECT
  if (deviceMode == RZORD_AUTODETECT) {
    // Hard reset should only resolve the USB host before the home screen.
    // Input probes stay in the runtime hot-swap path so boot lands on Home
    // immediately and plugged-in controllers are handled after UI startup.
    setInputAutoDetectModeActive(true);
    clearInputAutoDetectBootPending();
    clearAutoDetectResult();
    clearAutoInputResolvedGrace();
    initializeInputHotSwapDisconnected(millis());
  }
  #endif
}

bool restoreAutoDetectModeForDisconnect() {
#ifdef ENABLE_INPUT_AUTODETECT
  if (inputHotSwapRevertedToAutoWhileDisconnected() || deviceMode == RZORD_AUTODETECT) {
    return false;
  }

  deviceMode = RZORD_AUTODETECT;
  clearAutoDetectResult();
  webhid_update_device_mode(deviceMode);
  markInputHotSwapRevertedToAutoWhileDisconnected();
  return true;
#else
  return false;
#endif
}

bool inputAutoDetectSuspendedForIdleUi() {
  return idleAnimationActive || idleDimActive;
}

bool shouldDeferAutoDetectHotSwap(uint32_t now, bool waitingForInitialResolve,
                                  uint32_t disconnect_delay_ms) {
  if (!inputHotSwapDisconnectDelayElapsed(now, disconnect_delay_ms)) {
    return true;
  }

  if (!waitingForInitialResolve && inputAutoResolvedGraceActive(now)) {
    return true;
  }

  return false;
}
