#include "analog_calibration_state.h"
#include "button_remap.h"
#include "controller_settings_state.h"
#include "controller_frame_state.h"
#include "device_runtime_state.h"
#include "firmware_support.h"
#include "settings_store_internal.h"
#include "../input/autodetect/input_autodetect_runtime.h"
#include "../input/autodetect/input_autodetect_runtime_state.h"
#include "../input/runtime/input_boot_runtime.h"
#include "../input/runtime/input_frame_runtime.h"
#include "../output/output_capabilities.h"
#include "../menu/menu_mode_state.h"
#include "../menu/menu_runtime_state.h"
#include "../menu/menu_working_state.h"
#include "../output/runtime/input_usb_identity_runtime.h"
#include "../output/runtime/output_boot_runtime.h"
#include "../output/output_runtime_state.h"
#include "../platform/webhid_runtime.h"
#include "settings_store.h"
#include "turbo.h"

#include <string.h>

namespace {

bool isPersistedAutoDetectMode(DeviceEnum mode) {
#ifdef ENABLE_INPUT_AUTODETECT
  return mode == RZORD_AUTODETECT;
#else
  (void)mode;
  return false;
#endif
}

bool isManualInputMode(DeviceEnum mode) {
  return mode > RZORD_NONE &&
         mode < RZORD_LAST &&
         !isPersistedAutoDetectMode(mode);
}

DeviceEnum getPersistedInputModeForSave(DeviceEnum selectedMode) {
  if (selectedMode == deviceMode && isPersistedAutoDetectMode(savedDeviceMode)) {
    return savedDeviceMode;
  }

  if (inputAutoDetectModeActive() &&
      selectedMode == deviceMode &&
      savedDeviceMode > RZORD_NONE &&
      savedDeviceMode < RZORD_LAST) {
    return savedDeviceMode;
  }
  return selectedMode;
}

struct QuickConfigSaveSelection {
  DeviceEnum newInputMode;
  DeviceEnum persistedInputMode;
  outputMode_t newOutputMode;
  bool needsReboot;
  PerModeSettingsRecord selectedSettings;
};

void captureRuntimeTurboRates(uint8_t* rates) {
  for (uint8_t i = 0; i < TURBO_BTN_COUNT; ++i) {
    rates[i] = (uint8_t)turbo.getButtonRate((TurboButton)i);
  }
}

GlobalSettingsRecord loadCurrentGlobalSettings() {
  GlobalSettingsRecord settings{};
  readGlobalSettings(settings);
  return settings;
}

QuickConfigSaveSelection collectModeSaveSelection(
    DeviceEnum newInputMode,
    outputMode_t newOutputMode,
    const uint8_t* tempRates,
    const PerModeSettingsRecord& selectedSettings,
    bool keepCurrentOutputOnInputChange
) {
  QuickConfigSaveSelection selection{};
  selection.newInputMode = newInputMode;
  selection.persistedInputMode = getPersistedInputModeForSave(newInputMode);
  selection.newOutputMode = canonicalizeOutputMode(newOutputMode);
  if (keepCurrentOutputOnInputChange && selection.newInputMode != deviceMode) {
    selection.newOutputMode = configuredOutputMode;
  }
  selection.needsReboot =
    (selection.newInputMode != deviceMode) ||
    (selection.newOutputMode != configuredOutputMode);
#if defined(ADAPT_OUTPUT_USB_DEVICE)
  selection.needsReboot = selection.needsReboot || (menu_usb_descriptor_reboot_required != 0);
#endif

  selection.selectedSettings = selectedSettings;
  for (uint8_t i = 0; i < TURBO_BTN_COUNT; ++i) {
    selection.selectedSettings.turbo_rates[i] =
      (tempRates[i] < TURBO_RATE_LAST) ? tempRates[i] : (uint8_t)TURBO_OFF;
  }
  sanitizePerModeSettings(selection.newInputMode, selection.selectedSettings);
  return selection;
}

void persistSelection(const QuickConfigSaveSelection& selection) {
  GlobalSettingsRecord globalSettings = loadCurrentGlobalSettings();
  globalSettings.persisted_input_mode = (uint8_t)selection.persistedInputMode;
  globalSettings.configured_output_mode = (uint8_t)selection.newOutputMode;
  sanitizeGlobalSettings(globalSettings);
  writeGlobalSettings(globalSettings);

  if (isConcreteModeForPerSettings(selection.newInputMode)) {
    writePerModeSettings(selection.newInputMode, selection.selectedSettings);
  }
}

void applySavedModeRuntimeState(
    const QuickConfigSaveSelection& selection,
    bool preserveAutoOutput
) {
  const bool preserveLiveOutputInSession =
    !selection.needsReboot &&
    selection.newOutputMode == configuredOutputMode;
  const outputMode_t resolvedOutputMode = outputMode;
  const AutoDetectState resolvedAutoDetectState = autoDetectState;
  const AutoDetectProbeStage resolvedProbeStage = autoDetectProbeStage;

  savedDeviceMode = selection.persistedInputMode;
  setInputAutoDetectModeActive(isPersistedAutoDetectMode(savedDeviceMode));
  configuredOutputMode = selection.newOutputMode;
  if (preserveLiveOutputInSession) {
    outputMode = resolvedOutputMode;
    autoDetectState = resolvedAutoDetectState;
    autoDetectProbeStage = resolvedProbeStage;
  } else {
    outputMode = sanitizeRuntimeOutputMode(configuredOutputMode);
    autoDetectState = AUTO_STATE_IDLE;
  }
  if (isManualInputMode(selection.persistedInputMode) &&
      preserveAutoOutput &&
      !preserveLiveOutputInSession) {
    auto_detect_clear_input_scratch_state();
  }
  if (!preserveAutoOutput && !preserveLiveOutputInSession) {
    auto_detect_clear_scratch_state();
  }
}

bool shouldPreserveAutoOutputForInputReboot(const QuickConfigSaveSelection& selection) {
  return selection.needsReboot &&
         selection.newOutputMode == configuredOutputMode &&
         auto_detect_preserve_known_runtime_mode_for_input_reboot();
}

#ifndef REFLEX_HOST_TEST_EEPROM_AUTH
bool isManualInputOnlyModeChange(const QuickConfigSaveSelection& selection) {
  if (selection.newInputMode == deviceMode ||
      selection.newOutputMode != configuredOutputMode ||
      !isManualInputMode(selection.persistedInputMode)) {
    return false;
  }
#if defined(ADAPT_OUTPUT_USB_DEVICE)
  if (output_is_generic_mister_hid_mode(get_effective_output_mode()) &&
      inputModeNeedsBcdDeviceReenumeration(selection.newInputMode)) {
    // MiSTer GameControllerDB keys off bcdDevice, so a live input-family
    // switch would keep the previous mapping until USB re-enumerates.
    return false;
  }
#endif
#if defined(ADAPT_OUTPUT_USB_DEVICE)
  if (menu_usb_descriptor_reboot_required != 0) {
    return false;
  }
#endif
  return true;
}

uint8_t expectedUsbSlotCountForCurrentRuntime() {
  uint8_t expected = bootUsbSlotCountForModeAndInput(
    get_effective_output_mode(),
    deviceMode,
    inputPortCount());
  if (expected == 0) {
    expected = 1;
  }
  return expected;
}

bool applyManualInputModeLive(const QuickConfigSaveSelection& selection) {
  if (!isManualInputOnlyModeChange(selection)) {
    return false;
  }

  const outputMode_t resolvedOutputMode = outputMode;
  const AutoDetectState resolvedAutoDetectState = autoDetectState;
  const AutoDetectProbeStage resolvedProbeStage = autoDetectProbeStage;

  savedDeviceMode = selection.persistedInputMode;
  deviceMode = selection.newInputMode;
  setInputAutoDetectModeActive(false);
  clearAutoDetectResult();
  clearAutoInputResolvedGrace();
  clearInputHotSwapNextDetectAllowedAt();
  configuredOutputMode = selection.newOutputMode;
  outputMode = resolvedOutputMode;
  autoDetectState = resolvedAutoDetectState;
  autoDetectProbeStage = resolvedProbeStage;
  auto_detect_clear_input_scratch_state();

  initializeInputModuleForRuntimeModeChange();
  webhid_update_device_mode(deviceMode);

  return expectedUsbSlotCountForCurrentRuntime() == max_devices;
}
#else
bool applyManualInputModeLive(const QuickConfigSaveSelection& selection) {
  (void)selection;
  return false;
}
#endif

}  // namespace

void persistConfiguredInputMode(DeviceEnum mode) {
  GlobalSettingsRecord globalSettings = loadCurrentGlobalSettings();
  globalSettings.persisted_input_mode = (uint8_t)mode;
  globalSettings.configured_output_mode = (uint8_t)canonicalizeOutputMode(configuredOutputMode);
  sanitizeGlobalSettings(globalSettings);
  writeGlobalSettings(globalSettings);
}

void saveModeSettings(DeviceEnum newInputMode, outputMode_t newOutputMode, const uint8_t* tempRates,
                      const PerModeSettingsRecord& selectedSettings) {
  QuickConfigSaveSelection selection = collectModeSaveSelection(
    newInputMode,
    newOutputMode,
    tempRates,
    selectedSettings,
    true
  );

  persistSelection(selection);
  if (applyManualInputModeLive(selection)) {
    return;
  }

  const bool preserveAutoOutput =
    shouldPreserveAutoOutputForInputReboot(selection);
  applySavedModeRuntimeState(selection, preserveAutoOutput);

  if (selection.needsReboot) {
    delay(100);
    reboot();
  }
}

bool saveMenuModeSettings() {
  uint8_t turboRates[TURBO_BTN_COUNT] = {};
  PerModeSettingsRecord selectedSettings{};

  if (menu_input == deviceMode) {
    captureRuntimeTurboRates(turboRates);
  } else {
    loadStoredTurboRatesForMode(menu_input, turboRates);
  }
  captureMenuPerModeSettings(menu_input, selectedSettings);

  QuickConfigSaveSelection selection = collectModeSaveSelection(
    menu_input,
    menu_output,
    turboRates,
    selectedSettings,
    false
  );
  const bool preserveAutoOutput =
    shouldPreserveAutoOutputForInputReboot(selection);
  persistSelection(selection);
  applySavedModeRuntimeState(selection, preserveAutoOutput);
  return selection.needsReboot;
}
