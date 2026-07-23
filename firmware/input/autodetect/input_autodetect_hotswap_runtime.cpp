#include "../../product_config.h"
#include "../../firmware_platform_config.h"

#include <Arduino.h>
#include "hardware/watchdog.h"

#include "../../core/device_runtime_state.h"
#include "../../core/controller_frame_state.h"
#include "../../core/firmware_support.h"
#include "../../core/runtime/runtime_debug_cdc_state.h"
#include "../../menu/menu_bridge.h"
#include "../../platform/buzzer.h"
#include "../../output/output_capabilities.h"
#include "../../platform/display_runtime_state.h"
#include "../../output/output_runtime_state.h"
#include "../../output/runtime/output_boot_runtime.h"
#include "../../output/runtime/input_runtime_output_bridge.h"
#include "input_autodetect_runtime_internal.h"
#include "input_autodetect_benchmark.h"
#include "input_autodetect_runtime_state.h"
#include "../mixed/input_mixed_runtime_state.h"
#include "../runtime/input_adapter_runtime.h"
#include "../runtime/input_boot_runtime.h"
#include "../runtime/input_frame_runtime.h"

#ifdef ENABLE_INPUT_AUTODETECT
namespace {

constexpr uint8_t kAutoInputResolveSourceLiveHotSwap = 4;
constexpr uint8_t kAutoInputResolveSourceAssistedHotSwap = 5;
constexpr uint8_t kHotSwapTraceThrottleSlots = 64;
constexpr uint32_t kHotSwapBlockTraceIntervalMs = 250;

uint8_t disconnectedNoInputQuickMisses = 0;
uint32_t lastHotSwapBlockTraceMs[kHotSwapTraceThrottleSlots] = {};
uint32_t hotSwapTimingBaseMs = 0;
uint32_t hotSwapScanStartMs = 0;
uint32_t hotSwapDetectEndMs = 0;
const char* hotSwapTimingSource = "idle";

void resetDisconnectedNoInputQuickMisses() {
  disconnectedNoInputQuickMisses = 0;
}

void markHotSwapTimingBase(const char* source) {
  hotSwapTimingBaseMs = millis();
  hotSwapTimingSource = source;
  hotSwapScanStartMs = 0;
  hotSwapDetectEndMs = 0;
}

void markHotSwapScanStart(const char* source) {
  hotSwapScanStartMs = millis();
  if (hotSwapTimingBaseMs == 0) {
    hotSwapTimingBaseMs = hotSwapScanStartMs;
    hotSwapTimingSource = source;
  }
}

void markHotSwapDetectEnd() {
  hotSwapDetectEndMs = millis();
}

void resetHotSwapTiming() {
  hotSwapTimingBaseMs = 0;
  hotSwapScanStartMs = 0;
  hotSwapDetectEndMs = 0;
  hotSwapTimingSource = "idle";
}

void printHotSwapTimingLine(DeviceEnum newMode, bool assistedMode) {
  #if defined(ENABLE_RUNTIME_SERIAL_DEBUG_CDC) && defined(ENABLE_INPUT_AUTODETECT_HOTSWAP_CDC_TRACE)
  if (!runtimeDebugCdcEnabled) {
    resetHotSwapTiming();
    return;
  }
  const uint32_t now = millis();
  const uint32_t scanStartMs = hotSwapScanStartMs != 0 ? hotSwapScanStartMs : now;
  const uint32_t baseMs = hotSwapTimingBaseMs != 0 ? hotSwapTimingBaseMs : scanStartMs;
  const uint32_t detectEndMs = hotSwapDetectEndMs != 0 ? hotSwapDetectEndMs : now;
  runtimeDebugCdc.print(F("ADHOTPLUG MODE="));
  runtimeDebugCdc.print((int)newMode);
  runtimeDebugCdc.print(F(" RESULT="));
  runtimeDebugCdc.print(autoDetectResultName(autoDetectResult));
  runtimeDebugCdc.print(F(" SOURCE="));
  runtimeDebugCdc.print(hotSwapTimingSource);
  runtimeDebugCdc.print(F(" PORT="));
  runtimeDebugCdc.print((int)input_autodetect_last_port);
  runtimeDebugCdc.print(F(" ASSIST="));
  runtimeDebugCdc.print(assistedMode ? 1 : 0);
  runtimeDebugCdc.print(F(" TOTAL_MS="));
  runtimeDebugCdc.print((uint32_t)(now - scanStartMs));
  runtimeDebugCdc.print(F(" EDGE_TO_SCAN_MS="));
  runtimeDebugCdc.print((uint32_t)(scanStartMs - baseMs));
  runtimeDebugCdc.print(F(" SCAN_MS="));
  runtimeDebugCdc.print((uint32_t)(detectEndMs - scanStartMs));
  runtimeDebugCdc.print(F(" INIT_MS="));
  runtimeDebugCdc.print((uint32_t)(now - detectEndMs));
  runtimeDebugCdc.println();
  resetHotSwapTiming();
  #else
  (void)newMode;
  (void)assistedMode;
  #endif
}

void printHotSwapDisconnectLine(DeviceEnum disconnectedMode, bool restoredToAuto) {
  #if defined(ENABLE_RUNTIME_SERIAL_DEBUG_CDC) && defined(ENABLE_INPUT_AUTODETECT_HOTSWAP_CDC_TRACE)
  if (!runtimeDebugCdcEnabled) {
    return;
  }
  runtimeDebugCdc.print(F("ADHOTUNPLUG MODE="));
  runtimeDebugCdc.print((int)disconnectedMode);
  runtimeDebugCdc.print(F(" AUTO_HOME="));
  runtimeDebugCdc.print(restoredToAuto ? 1 : 0);
  runtimeDebugCdc.println();
  #else
  (void)disconnectedMode;
  (void)restoredToAuto;
  #endif
}

void markHotSwapBlockTrace(AutoDetectBenchmarkEvent event,
                           uint8_t aux = 0,
                           uint16_t value = 0) {
  const uint8_t slot = (uint8_t)event;
  if (slot >= kHotSwapTraceThrottleSlots) {
    autoDetectBenchmarkMark(event, aux, value);
    return;
  }

  const uint32_t now = millis();
  if (lastHotSwapBlockTraceMs[slot] != 0 &&
      (uint32_t)(now - lastHotSwapBlockTraceMs[slot]) < kHotSwapBlockTraceIntervalMs) {
    return;
  }
  lastHotSwapBlockTraceMs[slot] = now;
  autoDetectBenchmarkMark(event, aux, value);
}

void showHotSwapTransitionMessage(const char* modeName) {
  #ifdef USE_I2C_DISPLAY
  Wire.setSDA(OLED_I2C_SDA);
  Wire.setSCL(OLED_I2C_SCL);
  Wire.begin();
  display.clear();
  display.set2X();
  display.println(F("Detected:"));
  display.println(modeName);
  display.set1X();
  display.println();
  display.println(F("Switching..."));
  Wire.end();
  #else
  (void)modeName;
  #endif
}

void clearAutoDetectDisconnectedFrames() {
  for (uint8_t i = 0; i < MAX_USB_OUT; ++i) {
    inputFrame(i) = controller_state_t{};
    clearInputFrameTypeName(i);
    setInputFrameConnected(i, false);
  }
  markInputPollUpdated();
}

bool inputHotSwapNeedsUsbDescriptorReenumeration() {
  const uint8_t expectedUsbSlots = bootUsbSlotCountForModeAndInput(
      get_effective_output_mode(), deviceMode, inputPortCount());
  if (connectedInputFrameCount(inputPortCount()) <= max_devices &&
      activeInputAdapterHasPhysicalConnectionForHotSwap()) {
    return false;
  }
  return expectedUsbSlots > max_devices;
}

bool psxPeripheralHotSwapChangedUsbDescriptor(outputMode_t previousMode) {
  return output_is_specialized_mister_psx_mode(outputMode) &&
         canonicalizeOutputMode(previousMode) != canonicalizeOutputMode(outputMode);
}

bool restoreGenericMisterOutputAfterPsxPeripheralDisconnect(DeviceEnum disconnectedMode) {
  if (disconnectedMode != RZORD_PSX ||
      !is_auto_output_mode_selected() ||
      !output_is_specialized_mister_psx_mode(outputMode)) {
    return false;
  }
  outputMode = OUTPUT_MISTER;
  return true;
}

bool autoHomeNeedsUsbDescriptorReenumeration() {
  const uint8_t expectedUsbSlots = bootUsbSlotCountForModeAndInput(
      get_effective_output_mode(), RZORD_AUTODETECT, inputPortCount());
  return expectedUsbSlots != max_devices;
}

void reenumerateUsbForHotSwapPlayerCountChange(bool assistedMode) {
  const bool outputPreserved = auto_detect_preserve_known_runtime_mode_for_input_reboot();
  if (is_auto_output_mode_selected() && !outputPreserved) {
    return;
  }

  autoDetectBenchmarkMark(ADBENCH_DESCRIPTOR_REENUM_REQUEST, assistedMode ? 1 : 0, deviceMode);
  (void)assistedMode;
  preserveInputModeForPlayerCountReboot(deviceMode, inputPortCount());
  reboot();
}

void reenumerateUsbForAutoHomePlayerCountChange() {
  const bool outputPreserved = auto_detect_preserve_known_runtime_mode_for_input_reboot();
  if (is_auto_output_mode_selected() && !outputPreserved) {
    return;
  }

  autoDetectBenchmarkMark(ADBENCH_DESCRIPTOR_REENUM_REQUEST, 2, RZORD_AUTODETECT);
  preserveAutoDetectHomeForReboot();
  reboot();
}

bool detectedSaturnFamilyModeRequiresRuntimeConfirmation(DeviceEnum mode) {
  return mode == RZORD_SATURN || mode == RZORD_MEGADRIVE;
}

bool detectedModeStillPresentAfterRuntimeSetup(DeviceEnum mode) {
  if (!detectedSaturnFamilyModeRequiresRuntimeConfirmation(mode)) {
    return true;
  }
  return anyInputFrameConnected(inputPortCount()) ||
         activeInputAdapterHasPhysicalConnectionForHotSwap();
}

void rejectUnconfirmedDetectedInputMode(DeviceEnum rejectedMode) {
  autoDetectBenchmarkMark(ADBENCH_MODE_REJECT, (uint8_t)rejectedMode, inputPortCount());
  deviceMode = RZORD_AUTODETECT;
  savedDeviceMode = RZORD_AUTODETECT;
  setInputAutoDetectModeActive(true);
  clearAutoDetectResult();
  clearAutoInputResolvedGrace();
  clearAutoDetectDisconnectedFrames();
  markInputHotSwapDisconnected(millis());
  resetDisconnectedNoInputQuickMisses();
  scheduleInputHotSwapDetectAfter(millis(), inputAutoDetectNoInputRetryIntervalMs());
  resetIdleTimer();
  forceMainDisplayRefresh();
}

void applyDetectedInputModeLive(DeviceEnum newMode, bool assistedMode) {
  autoDetectBenchmarkMark(ADBENCH_MODE_APPLY_START, assistedMode ? 1 : 0, newMode);
  deviceMode = newMode;
  savedDeviceMode = assistedMode ? newMode : RZORD_AUTODETECT;
  setInputAutoDetectModeActive(!assistedMode);
  recordInputAutoResolve(
      (uint8_t)newMode,
      assistedMode ? kAutoInputResolveSourceAssistedHotSwap
                   : kAutoInputResolveSourceLiveHotSwap,
      input_autodetect_last_port);
  resetDisconnectedNoInputQuickMisses();
  setAutoInputResolvedGraceUntil(millis() + autoInputResolvedGraceMs(newMode));
  clearInputHotSwapNextDetectAllowedAt();
  clearAutoDetectDisconnectedFrames();
  const outputMode_t outputModeBeforeInputSetup = get_effective_output_mode();
  initializeInputModuleForRuntimeModeChange();
  autoDetectBenchmarkMark(ADBENCH_MODE_INIT_DONE, assistedMode ? 1 : 0, newMode);
  if (!detectedModeStillPresentAfterRuntimeSetup(newMode)) {
    rejectUnconfirmedDetectedInputMode(newMode);
    return;
  }
  buzzer.playModeChange();
  autoDetectBenchmarkMark(ADBENCH_MODE_BEEP_QUEUED, assistedMode ? 1 : 0, newMode);
  printHotSwapTimingLine(newMode, assistedMode);
  if (psxPeripheralHotSwapChangedUsbDescriptor(outputModeBeforeInputSetup) ||
      inputHotSwapNeedsUsbDescriptorReenumeration()) {
    reenumerateUsbForHotSwapPlayerCountChange(assistedMode);
    return;
  }
  webhid_update_device_mode(deviceMode);
  resetIdleTimer();
  forceMainDisplayRefresh();
  autoDetectBenchmarkMark(ADBENCH_MODE_APPLY_END, assistedMode ? 1 : 0, newMode);
}

void hotSwapToPassiveMode(DeviceEnum newMode, const char* modeName) {
  showHotSwapTransitionMessage(modeName);
  applyDetectedInputModeLive(newMode, true);
}

void hotSwapToMode(DeviceEnum newMode, const char* modeName) {
  (void)modeName;
  applyDetectedInputModeLive(newMode, false);
}

bool shouldHotSwapToDetectedMode(DeviceEnum newMode) {
  return newMode != RZORD_NONE && (newMode != deviceMode || inputMixedModeActive());
}

void hotSwapToDetectedMode(DeviceEnum newMode, const char* modeName) {
  #ifdef ENABLE_INPUT_NEOGEO
  if (newMode == RZORD_NEOGEO) {
    hotSwapToPassiveMode(newMode, modeName);
    return;
  }
  #endif
  hotSwapToMode(newMode, modeName);
}

bool tryHotSwapToDetectedMode(DeviceEnum newMode) {
  if (!shouldHotSwapToDetectedMode(newMode)) {
    return false;
  }
  hotSwapToDetectedMode(newMode, inputMixedModeActive() ? "Mixed" : autoDetectResultName(autoDetectResult));
  return true;
}

bool tryFastDisconnectedNoInputCommonHotSwap() {
  autoDetectBenchmarkMark(
      ADBENCH_HOTSWAP_QUICK_SCAN_START,
      disconnectedNoInputQuickMisses,
      inputAutoDetectNoInputFullScanEveryQuickMisses());
  markHotSwapScanStart("quick");
  DeviceEnum newMode = runAutoDetectionFastCommonOnly(true);
  markHotSwapDetectEnd();
  autoDetectBenchmarkMark(
      ADBENCH_HOTSWAP_QUICK_SCAN_END,
      (uint8_t)newMode,
      (uint16_t)autoDetectResult);
  if (tryHotSwapToDetectedMode(newMode)) {
    disconnectedNoInputQuickMisses = 0;
    return true;
  }
  if (disconnectedNoInputQuickMisses < 0xFF) {
    ++disconnectedNoInputQuickMisses;
  }
  return false;
}

bool disconnectedNoInputFullScanDue() {
  return disconnectedNoInputQuickMisses >=
         inputAutoDetectNoInputFullScanEveryQuickMisses();
}

  #if AUTODETECT_ENABLE_PASSIVE_ASSIST
bool tryPassiveHotSwapToDetectedMode(DeviceEnum newMode) {
  if (!shouldHotSwapToDetectedMode(newMode)) {
    return false;
  }
  const char* modeName = autoDetectResultName(autoDetectResult);
  hotSwapToPassiveMode(newMode, modeName);
  return true;
}

bool handlePassiveAssistedHotSwapQuick() {
  PassiveAssistScanResult passiveQuick = runPassiveAssistedAutoDetectScan();
  if (passiveQuick.result != AUTODETECT_NONE) {
    DeviceEnum passiveMode = applyPassiveAutoDetectResult(
        passiveQuick.result, 0, passiveQuick.port, INPUT_AUTODETECT_TIMING_HOTSWAP);
    resetIdleTimer();
    return tryPassiveHotSwapToDetectedMode(passiveMode);
  }
  if (inputAutoDetectAssistCandidatePending(passiveQuick, true)) {
    return false;
  }

  return false;
}

bool handlePassiveAssistedHotSwapFallback() {
  resetAutoDetectPins();
  delayWithButtonPolling(8);

  uint32_t passive_started = millis();
  PassiveAssistScanResult passiveResult = runPassiveOnlyAutoDetectScan();
  if (passiveResult.result != AUTODETECT_NONE) {
    DeviceEnum passiveMode = applyPassiveAutoDetectResult(
        passiveResult.result, millis() - passive_started, passiveResult.port,
        INPUT_AUTODETECT_TIMING_HOTSWAP);

    resetIdleTimer();
    return tryPassiveHotSwapToDetectedMode(passiveMode);
  }

  return false;
}
  #else
bool handlePassiveAssistedHotSwapQuick() { return false; }
bool handlePassiveAssistedHotSwapFallback() { return false; }
  #endif

void restoreAutoDetectHomeAfterDisconnect(DeviceEnum disconnectedMode) {
  autoDetectBenchmarkMark(ADBENCH_AUTO_HOME_RESTORE);
  const bool psxOutputDescriptorReset =
      restoreGenericMisterOutputAfterPsxPeripheralDisconnect(disconnectedMode);
  resetDisconnectedNoInputQuickMisses();
  clearAutoDetectDisconnectedFrames();
  initializeInputModuleForRuntimeModeChange();
  clearAutoDetectDisconnectedFrames();
  resetIdleTimer();
  forceMainDisplayRefresh();
  autoDetectBenchmarkMark(ADBENCH_MAIN_REFRESH);
  if (psxOutputDescriptorReset || autoHomeNeedsUsbDescriptorReenumeration()) {
    reenumerateUsbForAutoHomePlayerCountChange();
  }
}

}  // namespace

bool handleConnectedAutoDetectHotSwap(bool waitingForInitialResolve) {
  bool justConnected = !inputHotSwapWasConnected();
  if (justConnected) {
    autoDetectBenchmarkMark(ADBENCH_HOTSWAP_CONNECTED_EDGE, waitingForInitialResolve ? 1 : 0);
    markHotSwapTimingBase("edge");
  }
  resetDisconnectedNoInputQuickMisses();
  markInputHotSwapConnected();
  clearAutoInputResolvedGrace();

  if (!waitingForInitialResolve) {
    if (inputHotSwapNeedsUsbDescriptorReenumeration()) {
      reenumerateUsbForHotSwapPlayerCountChange(false);
      return true;
    }
    clearInputHotSwapNextDetectAllowedAt();
    return false;
  }

  if (!waitingForInitialResolve && inputAutoDetectSuspendedForIdleUi()) {
    markHotSwapBlockTrace(ADBENCH_HOTSWAP_SKIP_IDLE_UI, 0, 0);
    return false;
  }

  if (justConnected || inputHotSwapDetectDue(millis())) {
    autoDetectBenchmarkMark(
        ADBENCH_HOTSWAP_CONNECTED_SCAN_START,
        justConnected ? 1 : 0,
        waitingForInitialResolve ? 1 : 0);
    markHotSwapScanStart(justConnected ? "edge" : "connected");
    DeviceEnum newMode = runAutoDetection(true);
    markHotSwapDetectEnd();
    autoDetectBenchmarkMark(
        ADBENCH_HOTSWAP_CONNECTED_SCAN_END,
        (uint8_t)newMode,
        (uint16_t)autoDetectResult);
    scheduleUnresolvedAutoDetectRetry(millis());
    return tryHotSwapToDetectedMode(newMode);
  }

  markHotSwapBlockTrace(
      ADBENCH_HOTSWAP_WAIT_DUE,
      waitingForInitialResolve ? 1 : 0,
      1);

  return false;
}

bool handleDisconnectedAutoDetectHotSwap(bool waitingForInitialResolve) {
  if (!inputHotSwapInitialized()) {
    initializeInputHotSwapDisconnected(millis());
  }

  if (inputHotSwapWasConnected()) {
    autoDetectBenchmarkMark(ADBENCH_HOTSWAP_DISCONNECTED_EDGE, waitingForInitialResolve ? 1 : 0);
    const DeviceEnum disconnectedMode = deviceMode;
    markInputHotSwapDisconnected(millis());
    clearAutoDetectResult();
    clearAutoInputResolvedGrace();
    if (savedDeviceMode == RZORD_AUTODETECT) {
      const bool restoredToAuto = restoreAutoDetectModeForDisconnect();
      printHotSwapDisconnectLine(disconnectedMode, restoredToAuto);
      resetHotSwapTiming();
      if (restoredToAuto) {
        restoreAutoDetectHomeAfterDisconnect(disconnectedMode);
        return true;
      }
    }
    return false;
  }

  // If the first disconnected edge was missed, return resolved AUTO sessions to
  // the idle AUTO home screen without a full probe. Active drivers own their
  // own presence debounce, so held button states cannot re-enter arbitration.
  const DeviceEnum disconnectedMode = deviceMode;
  bool skipDisconnectDelay = false;
  if (waitingForInitialResolve && inputHotSwapRevertedToAutoWhileDisconnected()) {
    const uint32_t now = millis();
    if (!inputHotSwapDetectDue(now)) {
      markHotSwapBlockTrace(ADBENCH_HOTSWAP_WAIT_AUTO_REVERTED, 1, 0);
      return false;
    }
    autoDetectBenchmarkMark(ADBENCH_HOTSWAP_CLEAR_AUTO_REVERTED, 1, 0);
    markInputHotSwapRevertedToAutoWhileDisconnected(false);
    skipDisconnectDelay = true;
  }

  const uint32_t disconnectDelayMs = inputAutoDetectDisconnectDelayMs();
  if (!skipDisconnectDelay &&
      shouldDeferAutoDetectHotSwap(millis(), waitingForInitialResolve,
                                   disconnectDelayMs)) {
    markHotSwapBlockTrace(
        ADBENCH_HOTSWAP_WAIT_DISCONNECT_DELAY,
        waitingForInitialResolve ? 1 : 0,
        disconnectDelayMs > 65535u ? 65535u : (uint16_t)disconnectDelayMs);
    return false;
  }

  const bool restoredToAuto = restoreAutoDetectModeForDisconnect();
  printHotSwapDisconnectLine(disconnectedMode, restoredToAuto);
  resetHotSwapTiming();
  if (restoredToAuto && savedDeviceMode == RZORD_AUTODETECT) {
    restoreAutoDetectHomeAfterDisconnect(disconnectedMode);
    return true;
  }

  if (handlePassiveAssistedHotSwapQuick()) {
    return true;
  }

  if (!waitingForInitialResolve && inputAutoDetectSuspendedForIdleUi()) {
    markHotSwapBlockTrace(ADBENCH_HOTSWAP_SKIP_IDLE_UI, 1, 0);
    return false;
  }

  if (!inputHotSwapDetectDue(millis())) {
    markHotSwapBlockTrace(
        ADBENCH_HOTSWAP_WAIT_DUE,
        waitingForInitialResolve ? 1 : 0,
        2);
    return false;
  }

  if (waitingForInitialResolve) {
    if (tryFastDisconnectedNoInputCommonHotSwap()) {
      return true;
    }

    if (!disconnectedNoInputFullScanDue()) {
      scheduleInputHotSwapDetectAfter(
          millis(), inputAutoDetectFastNoInputRetryIntervalMs());
      restoreAutoDetectSharedInputLines(disconnectedMode);
      return false;
    }

    resetDisconnectedNoInputQuickMisses();
  }

  autoDetectBenchmarkMark(
      ADBENCH_HOTSWAP_FULL_SCAN_START,
      waitingForInitialResolve ? 1 : 0,
      disconnectedNoInputQuickMisses);
  markHotSwapScanStart("full");
  DeviceEnum newMode = runAutoDetection(true);
  markHotSwapDetectEnd();
  autoDetectBenchmarkMark(
      ADBENCH_HOTSWAP_FULL_SCAN_END,
      (uint8_t)newMode,
      (uint16_t)autoDetectResult);

  if (tryHotSwapToDetectedMode(newMode)) {
    resetDisconnectedNoInputQuickMisses();
    return true;
  }

  if (handlePassiveAssistedHotSwapFallback()) {
    return true;
  }

  scheduleInputHotSwapDetectAfter(
      millis(),
      waitingForInitialResolve ? inputAutoDetectFastNoInputRetryIntervalMs()
                               : inputAutoDetectNoInputRetryIntervalMs());
  restoreAutoDetectSharedInputLines(disconnectedMode);

  return false;
}
#endif
