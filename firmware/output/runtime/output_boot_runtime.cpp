#include "../../product_config.h"

#include "output_boot_runtime.h"

#include <Adafruit_TinyUSB.h>

#if defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
  #include "hardware/watchdog.h"
#endif

#include "../../core/controller_frame_state.h"
#include "../../core/device_runtime_state.h"
#include "../../firmware_platform_config.h"
#include "../../input/runtime/input_adapter_runtime.h"
#include "../../input/runtime/input_frame_runtime.h"
#include "../../input/autodetect/input_autodetect_benchmark.h"
#include "input_usb_identity_runtime.h"
#include "output_boot_bridge.h"
#include "../output_capabilities.h"
#include "../output_runtime_state.h"

namespace {

auto_output_boot_state_t getAutoOutputBootStateFromScratch() {
  auto_output_boot_state_t bootState = {
    .autoOutputProbeBoot =
      (configuredOutputMode == OUTPUT_AUTO) &&
      (watchdog_hw->scratch[AUTO_PROBE_SCRATCH_STAGE] != (uint32_t)AUTO_PROBE_GENERIC),
    .autoOutputResolvedBoot =
      (configuredOutputMode == OUTPUT_AUTO) &&
      (watchdog_hw->scratch[AUTO_DETECT_SCRATCH_MAGIC] == AUTO_DETECT_MAGIC) &&
      (sanitizeRuntimeOutputMode((outputMode_t)watchdog_hw->scratch[AUTO_DETECT_SCRATCH_STATE]) != OUTPUT_AUTO)
  };
  return bootState;
}

bool shouldReserveClassic2usbUsbSlots(outputMode_t mode) {
#ifdef PRODUCT_CLASSIC2USB
  switch (canonicalizeOutputMode(mode)) {
    case OUTPUT_HID:
    case OUTPUT_MISTER:
    case OUTPUT_MISTER_JOGCON:
    case OUTPUT_MISTER_NEGCON:
    case OUTPUT_XINPUT2P:
    case OUTPUT_SWITCH:
    case OUTPUT_SWITCHPRO:
      return true;
    default:
      return false;
  }
#else
  (void)mode;
  return false;
#endif
}

bool classic2usbInputCanReserveDetectedUsbSlots(DeviceEnum inputMode) {
#ifdef PRODUCT_CLASSIC2USB
  switch (inputMode) {
    #ifdef ENABLE_INPUT_PSX
    case RZORD_PSX:
    #endif
    #ifdef ENABLE_INPUT_SATURN
    case RZORD_SATURN:
    #endif
    #ifdef ENABLE_INPUT_PCE
    case RZORD_PCE:
    #endif
    #ifdef ENABLE_INPUT_SNES
    case RZORD_SNES:
    #endif
      return true;
    default:
      return false;
  }
#else
  return false;
#endif
}

uint8_t classic2usbBootUsbSlotCountForInput(outputMode_t mode, DeviceEnum inputMode,
                                            uint8_t portCount) {
  const uint8_t outputMax = get_max_output_for_mode(mode);
  const uint8_t physicalSlots = min(portCount, outputMax);
  if (shouldReserveClassic2usbUsbSlots(mode)) {
    if (portCount > outputMax && classic2usbInputCanReserveDetectedUsbSlots(inputMode)) {
      return min(portCount, (uint8_t)MAX_USB_OUT);
    }
    return outputMax;
  }
  return physicalSlots;
}

uint8_t classic2usbBootUsbSlotCount(outputMode_t mode) {
  return bootUsbSlotCountForModeAndInput(mode, deviceMode, inputPortCount());
}

void clearUnpolledUsbSlots(uint8_t firstSlot) {
  for (uint8_t i = firstSlot; i < max_devices && i < MAX_USB_OUT; ++i) {
    controllerFrame(i) = {};
  }
}

void restoreDetectedAutoOutputModeAtBoot() {
  outputMode_t detectedOutputMode = auto_detect_load_persisted_runtime_mode();
  if (detectedOutputMode != OUTPUT_AUTO) {
    auto_detect_restore_debug_snapshot();
    autoDetectProbeStage = AUTO_PROBE_GENERIC;
    outputMode = detectedOutputMode;
    autoDetectState = auto_detect_load_persisted_state(detectedOutputMode);
  } else {
    autoDetectProbeStage = auto_detect_load_probe_stage();
    output_autodetect_transition_reason = auto_detect_load_transition_reason();
    outputMode = auto_detect_probe_output_mode(autoDetectProbeStage);
    autoDetectState = AUTO_STATE_IDLE;
  }
}

void clearAutoOutputScratchRegisters() {
  watchdog_hw->scratch[AUTO_DETECT_SCRATCH_MAGIC] = 0;
  watchdog_hw->scratch[AUTO_DETECT_SCRATCH_STATE] = 0;
  watchdog_hw->scratch[AUTO_PROBE_SCRATCH_STAGE] = 0;
  watchdog_hw->scratch[AUTO_PROBE_SCRATCH_AUX] = 0;
}

}  // namespace

auto_output_boot_state_t restoreAutoOutputBootState() {
  auto_output_boot_state_t bootState = getAutoOutputBootStateFromScratch();

  if (configuredOutputMode == OUTPUT_AUTO) {
    restoreDetectedAutoOutputModeAtBoot();
  } else {
    autoDetectProbeStage = AUTO_PROBE_GENERIC;
    autoDetectState = AUTO_STATE_IDLE;
  }

  clearAutoOutputScratchRegisters();
  return bootState;
}

void restoreLockedOutputModeAfterInputSetup(bool outputModeLockedAtBoot, outputMode_t bootOutputMode) {
  if (!outputModeLockedAtBoot) {
    return;
  }

  // A resolved MiSTer/Linux host starts on generic DInput. PSX setup may then
  // identify a JogCon, NeGcon, or GunCon before USB is configured. Preserve
  // that descriptor promotion; all other Auto-host modes remain locked to the
  // host detector's result.
  if (canonicalizeOutputMode(bootOutputMode) == OUTPUT_MISTER &&
      output_is_specialized_mister_psx_mode(outputMode)) {
    return;
  }
  outputMode = bootOutputMode;
}

uint8_t bootUsbSlotCountForModeAndInput(outputMode_t mode, DeviceEnum inputMode, uint8_t portCount) {
  if (mode == OUTPUT_KEYBOARD) {
    return get_max_output_for_mode(mode);
  }

  return classic2usbBootUsbSlotCountForInput(mode, inputMode, portCount);
}

outputMode_t finalizeBootOutputMode() {
  outputMode_t effectiveOutputMode = get_effective_output_mode();
  if (effectiveOutputMode == OUTPUT_KEYBOARD) {
    // Keep fixed P1/P2 keyboard slots available even when the input mode
    // discovers extra ports after boot.
    max_devices = get_max_output_for_mode(effectiveOutputMode);
  } else {
    max_devices = classic2usbBootUsbSlotCount(effectiveOutputMode);
  }
  if (max_devices == 0) {
    max_devices = 1;
  }
  if (max_devices > inputPortCount()) {
    clearUnpolledUsbSlots(inputPortCount());
  }
  configureActiveInputAdapterBcdDeviceVersion();
  applyInputModeBcdDeviceVersion(deviceMode);

  #ifdef ADAPT_OUTPUT_USB_DEVICE
  if (must_set_serial_string(effectiveOutputMode)) {
    TinyUSBDevice.setSerialDescriptor(get_reflex_input_usb_serial_descriptor());
  }
  #endif

  return effectiveOutputMode;
}

void connectConfiguredOutputUsb() {
  #ifdef ADAPT_OUTPUT_USB_DEVICE
  #ifdef ENABLE_OUTPUT_JVS
  if (outputMode != OUTPUT_JVS)
  #endif
  {
    autoDetectBenchmarkMark(ADBENCH_USB_CONNECT);
    #if defined(AUTODETECT_DONOR_ATTACH_SEQUENCE)
    tud_disconnect();
    delay(250);
    #endif
    tud_connect();
    #if defined(AUTODETECT_DONOR_ATTACH_SEQUENCE)
    delay(50);
    #endif
  }
  #endif
}
