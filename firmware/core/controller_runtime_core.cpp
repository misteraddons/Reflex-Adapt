#include "../product_config.h"
#include "controller_runtime_core.h"

#include <Arduino.h>

#include "controller_runtime_internal.h"
#include "analog_calibration_state.h"
#include "button_chord_remap.h"
#include "button_remap.h"
#include "controller_settings_state.h"
#include "device_runtime_state.h"
#include "firmware_hotkeys.h"
#include "turbo.h"
#include "../platform/latency_service_mask.h"

namespace {

enum ControllerRuntimeFeature : uint16_t {
  RUNTIME_FEATURE_BOOTLOADER_HOTKEY = 1u << 0,
  RUNTIME_FEATURE_DPAD_HOTKEY       = 1u << 1,
  RUNTIME_FEATURE_VIRTUAL_HOTKEYS   = 1u << 2,
  RUNTIME_FEATURE_BUTTON_REMAP      = 1u << 3,
  RUNTIME_FEATURE_CHORD_REMAP       = 1u << 4,
  RUNTIME_FEATURE_TURBO             = 1u << 5,
  RUNTIME_FEATURE_CLASSIC_MERGE     = 1u << 6,
  RUNTIME_FEATURE_ANALOG_CALIBRATION = 1u << 7,
  RUNTIME_FEATURE_FINAL_DEADZONE    = 1u << 8,
  RUNTIME_FEATURE_OUTPUT_BUTTON_MAP = 1u << 9,
  RUNTIME_FEATURE_JAGUAR_ROTARY     = 1u << 10,
};

uint16_t runtimeFeatureMask = 0;
bool runtimeFeatureMaskInitialized = false;
DeviceEnum runtimeFeatureMaskMode = RZORD_AUTODETECT;

uint16_t __not_in_flash_func(refreshRuntimeFeatureMask)(bool updated) {
  if (runtimeFeatureMaskInitialized && !updated &&
      runtimeFeatureMaskMode == deviceMode) {
    return runtimeFeatureMask;
  }

  uint16_t mask = 0;
  if (hotkey_bootsel != (uint32_t)-1) {
    mask |= RUNTIME_FEATURE_BOOTLOADER_HOTKEY;
  }
  if (hotkey_dpad_as_buttons != (uint32_t)-1) {
    mask |= RUNTIME_FEATURE_DPAD_HOTKEY;
  }
  if (controller_runtime_internal::virtualControllerHotkeysRequireService()) {
    mask |= RUNTIME_FEATURE_VIRTUAL_HOTKEYS;
  }
  if (hasActiveRemap()) {
    mask |= RUNTIME_FEATURE_BUTTON_REMAP;
  }
  if (hasActiveButtonChordRemap()) {
    mask |= RUNTIME_FEATURE_CHORD_REMAP;
  }
  if (turbo.hasAnyEnabled()) {
    mask |= RUNTIME_FEATURE_TURBO;
  }
  if (classic_dual_merge_enabled != 0) {
    mask |= RUNTIME_FEATURE_CLASSIC_MERGE;
  }
  if (analogCalibration.enabled) {
    mask |= RUNTIME_FEATURE_ANALOG_CALIBRATION;
  }
  if (deadzone_percent != 0) {
    mask |= RUNTIME_FEATURE_FINAL_DEADZONE;
  }
#ifdef ENABLE_INPUT_N64
  if (deviceMode == RZORD_N64) {
    mask |= RUNTIME_FEATURE_OUTPUT_BUTTON_MAP;
  }
#endif
#ifdef ENABLE_INPUT_GAMECUBE
  if (deviceMode == RZORD_GAMECUBE) {
    mask |= RUNTIME_FEATURE_OUTPUT_BUTTON_MAP;
  }
#endif
#ifdef ENABLE_INPUT_JAGUAR
  if (deviceMode == RZORD_JAGUAR) {
    mask |= RUNTIME_FEATURE_JAGUAR_ROTARY;
  }
#endif
  runtimeFeatureMask = mask;
  runtimeFeatureMaskInitialized = true;
  runtimeFeatureMaskMode = deviceMode;
  return mask;
}

bool __not_in_flash_func(runtimeFeatureEnabled)(ControllerRuntimeFeature feature) {
  return (runtimeFeatureMask & feature) != 0;
}

}  // namespace

void __not_in_flash_func(processPolledInputFrame)(bool updated) {
  refreshRuntimeFeatureMask(updated);
  controller_runtime_internal::updateStickCenteringFromPoll(updated);
  controller_runtime_internal::snapshotRawInputButtonsBeforeTransforms();
  controller_runtime_internal::applyDeadzoneAndTriggerTransforms(updated);
  controller_runtime_internal::snapshotPhysicalButtonsBeforeTransforms();
  if (runtimeFeatureEnabled(RUNTIME_FEATURE_BOOTLOADER_HOTKEY)) {
    controller_runtime_internal::handleHeldBootloaderHotkey();
  }
  if (runtimeFeatureEnabled(RUNTIME_FEATURE_DPAD_HOTKEY)) {
    controller_runtime_internal::handleHeldDpadModeHotkey();
  }
  if (runtimeFeatureEnabled(RUNTIME_FEATURE_VIRTUAL_HOTKEYS) &&
      !latencyServiceDisabled(LATENCY_SERVICE_MENU_HOTKEYS)) {
    controller_runtime_internal::updateHeldControllerHotkeysBeforeTransforms();
  }
  if (runtimeFeatureEnabled(RUNTIME_FEATURE_BUTTON_REMAP) &&
      !latencyServiceDisabled(LATENCY_SERVICE_BUTTON_REMAP)) {
    controller_runtime_internal::applyButtonRemapTransforms();
  }
  if (runtimeFeatureEnabled(RUNTIME_FEATURE_CHORD_REMAP) &&
      !latencyServiceDisabled(LATENCY_SERVICE_CHORD_REMAP)) {
    controller_runtime_internal::applyButtonChordRemapTransforms();
  }
  if (runtimeFeatureEnabled(RUNTIME_FEATURE_TURBO) &&
      !latencyServiceDisabled(LATENCY_SERVICE_TURBO_TRANSFORM)) {
    controller_runtime_internal::applyTurboTransforms();
  }
  if (runtimeFeatureEnabled(RUNTIME_FEATURE_CLASSIC_MERGE)) {
    controller_runtime_internal::applyClassicDualControllerMergeTransform();
  }
  if (runtimeFeatureEnabled(RUNTIME_FEATURE_VIRTUAL_HOTKEYS) &&
      !latencyServiceDisabled(LATENCY_SERVICE_VIRTUAL_HOTKEYS)) {
    controller_runtime_internal::applyVirtualControllerHotkeyOutputs();
  }

  if (runtimeFeatureEnabled(RUNTIME_FEATURE_JAGUAR_ROTARY)) {
    controller_runtime_internal::updateJaguarRotaryStateFromInputs();
  }
#if !defined(PRODUCT_CLASSIC2USB)
  if (!latencyServiceDisabled(LATENCY_SERVICE_SOCD)) {
    controller_runtime_internal::applySocdCleaningToInputs();
  }
#endif
}

void cacheProcessedControllerOutputState() {
  controller_runtime_internal::cacheProcessedControllerOutputStateInternal();
}

void __not_in_flash_func(finalizeControllerFrameForOutput)() {
  // Output finalization may add host-only buttons or calibration. Restore the
  // captured frame after send, but keep digital buttons at the physical
  // pre-remap snapshot so remap/turbo never feeds back into itself.
  controller_runtime_internal::captureOutputFinalizeRestoreFrames();
  controller_runtime_internal::rebuildDigitalButtonsForOutputFrame(
      runtimeFeatureEnabled(RUNTIME_FEATURE_BUTTON_REMAP),
      runtimeFeatureEnabled(RUNTIME_FEATURE_CHORD_REMAP),
      runtimeFeatureEnabled(RUNTIME_FEATURE_TURBO),
      runtimeFeatureEnabled(RUNTIME_FEATURE_VIRTUAL_HOTKEYS));
  if (runtimeFeatureEnabled(RUNTIME_FEATURE_CLASSIC_MERGE)) {
    controller_runtime_internal::applyClassicDualControllerMergeTransform();
  }
  if (runtimeFeatureEnabled(RUNTIME_FEATURE_VIRTUAL_HOTKEYS) &&
      !latencyServiceDisabled(LATENCY_SERVICE_VIRTUAL_HOTKEYS)) {
    controller_runtime_internal::applyVirtualControllerHotkeyOutputs();
  }
  if (runtimeFeatureEnabled(RUNTIME_FEATURE_ANALOG_CALIBRATION)) {
    controller_runtime_internal::applyAnalogCalibrationToConnectedInputs();
  }
  if (runtimeFeatureEnabled(RUNTIME_FEATURE_FINAL_DEADZONE)) {
    controller_runtime_internal::applyFinalStickDeadzoneToConnectedInputs();
  }
  if (runtimeFeatureEnabled(RUNTIME_FEATURE_OUTPUT_BUTTON_MAP)) {
    controller_runtime_internal::applyOutputButtonModeMappingsToConnectedInputs();
  }
  if (runtimeFeatureEnabled(RUNTIME_FEATURE_JAGUAR_ROTARY)) {
    controller_runtime_internal::clearJaguarRotaryDpadBeforeUsbSend();
  }
  controller_runtime_internal::consumeMenuCapturedControllerOutput();
}

void __not_in_flash_func(restoreControllerFrameAfterOutputSend)() {
  controller_runtime_internal::restorePhysicalButtonsAfterUsbSend();
}
