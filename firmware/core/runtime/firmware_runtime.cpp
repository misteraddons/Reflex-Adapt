#include "../../product_config.h"

// Override TinyUSB defaults BEFORE any includes that might pull in TinyUSB.
// This translation unit owns the header-only USB output stack.
#define CFG_TUD_HID 6  // 6-player multitap support (Saturn, etc.)

#include "firmware_runtime.h"

#include "../device_runtime_state.h"

#include <Arduino.h>

// Input auto-detect probe diagnostics (OLED + USB CDC serial).
// Keep enabled while debugging Saturn/Jaguar mis-detection loops.
#ifndef AUTODETECT_DEBUG
  #define AUTODETECT_DEBUG
#endif

// https://github.com/DavidPagels/retro-pico-switch
//
// Switch Pro Rumble
// https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering/blob/master/rumble_data_table.md
// https://github.com/ndeadly/MissionControl
//
// XID (ogXbox) output
// https://github.com/Ryzee119/ogx360_t4
//
// Xinput output
// https://github.com/JonnyHaystack/Adafruit_TinyUSB_XInput/tree/master
//
// PS3 output
// https://github.com/matlo/GIMX-firmwares/blob/master/EMUPS3/
//
// PS4 auth: BearSSL RSA-PSS-SHA256 (ps4_auth.h), no USB host dongle needed
// Keys uploaded via WebHID (1712 bytes: serial + cert + RSA key components)

#include "../../firmware_platform_config.h"

// Input module gates are now set by product_config.h
// To override for development, uncomment individual lines below:
//#define ENABLE_INPUT_DUMMY
//#define ENABLE_INPUT_CUSTOM
//#define ENABLE_INPUT_GBA
//#define ENABLE_INPUT_PSX_DANCE
//#define ENABLE_INPUT_DREAMCAST
//#define ENABLE_INPUT_INTV
//#define ENABLE_INPUT_PADDLE
//#define ENABLE_INPUT_GAMEPORT
//#define ENABLE_INPUT_MEMCARD

#include <Adafruit_TinyUSB.h>

#include "hardware/watchdog.h"

// Verify CFG_TUD_HID is 6 for 6-player multitap support
static_assert(CFG_TUD_HID == 6, "CFG_TUD_HID must be 6 for 6-player multitap support");

// EEPROM - include early so input modules and out_usb.h can access it
#include <EEPROM.h>
#include "../eeprom_helper.h"
#include "../controller_settings_state.h"
#include "../../menu/menu_runtime_state.h"
#include "../settings_store.h"
#include "../firmware_support.h"
#include "../../platform/display_runtime_state.h"

//#include "../../input/shared/input_master.h"
#include "../../input/autodetect/input_autodetect_runtime_state.h"
#include "../../input/autodetect/input_autodetect_benchmark.h"
#include "../../output/usb/out_usb.h"
#include "../../platform/buzzer.h"
#include "../../platform/latency_test.h"
#include "../../platform/rgb_led.h"
#include "../../platform/button_handler.h"
#include "../turbo.h"
#include "../stick_center.h"

#if (MAX_USB_OUT > CFG_TUD_HID)
  #error MAX_USB_OUT > CFG_TUD_HID // Adafruit_TinyUSB_Library\src\arduino\ports\rp2040\tusb_config_rp2040.h
#endif

#include "firmware_runtime_forwards.h"
#include "../boot/boot_storage_runtime.h"
#include "../../input/base/RZInputModule.h"
#include "../../input/runtime/input_boot_runtime.h"
#include "../../output/runtime/output_boot_runtime.h"
#include "../controller_runtime_core.h"
#include "../serial_debug_runtime.h"
#include "runtime_loop.h"
#if defined(ENABLE_INPUT_USB)
#include "../../input/usb_host/input_usb_host_service.h"
#endif

#include "../../menu/menu.h"
#include "../../platform/boot/boot_hardware_runtime.h"
#include "../../platform/boot/boot_ui_runtime.h"
#include "../../platform/runtime/platform_feedback_runtime.h"

void runFirmwareSetup() {
  autoDetectBenchmarkMark(ADBENCH_BOOT_SETUP_START);
  prepareHardwareForBoot();
  showBootTraceMarker("1 hardware");
  beginPersistentStorage();
  serialDebugRuntimeSetup();
  showBootTraceMarker("2 storage");
  loadPersistedBootConfiguration();
  restoreInputModeFromScratchRegisters();
  auto_output_boot_state_t autoOutputBootState = restoreAutoOutputBootState();
  initializeRuntimeSettingsFromStorage();
  showBootTraceMarker("3 settings");
  const bool outputModeLockedAtBoot = is_auto_output_mode_selected();
  const outputMode_t bootOutputMode = outputMode;

  showBootSplashScreen();

  autoDetectBenchmarkMark(ADBENCH_BOOT_INPUT_START);
  initializeInputModuleForBoot();
  autoDetectBenchmarkMark(ADBENCH_BOOT_INPUT_END);
  showBootTraceMarker("4 input");
  restoreLockedOutputModeAfterInputSetup(outputModeLockedAtBoot, bootOutputMode);

  maybePlayResolvedBootJingle(autoOutputBootState.autoOutputProbeBoot, autoOutputBootState.autoOutputResolvedBoot);

  autoDetectBenchmarkMark(ADBENCH_BOOT_OUTPUT_START);
  finalizeBootOutputMode();
  autoDetectBenchmarkMark(ADBENCH_BOOT_OUTPUT_END);
  showBootTraceMarker("5 output");

  showBootTraceMarker("6a pre cfg");
#if !defined(DISABLE_BOOT_USB_CONNECT)
  autoDetectBenchmarkMark(ADBENCH_USB_CONFIG_START);
  configure_usb_output();
  autoDetectBenchmarkMark(ADBENCH_USB_CONFIG_END);
  showBootTraceMarker("6b cfg");
  connectConfiguredOutputUsb();
  showBootTraceMarker("6c conn");
#else
  showBootTraceMarker("6x no usb");
#endif
#if defined(ENABLE_INPUT_USB) && defined(DEFER_USB_INPUT_HOST_START)
  // Keep management/device USB recoverable first, then let the core1 PIO-USB
  // host stack come up for USB-controller input diagnostics.
  usb_host_service_request_start();
#endif
  showBootTraceMarker("6d usb");

  showBootUsbDebugInfo();
  showBootTraceMarker("6e info");
  // Update WebHID with current device mode
  webhid_update_device_mode(deviceMode);

  // Ignore any button state transitions that happened during power-up or the
  // boot splash so the first menu action must be a deliberate post-boot press.
  modeButton.suppressUntilRelease();
  resetButton.suppressUntilRelease();
  // Paint home once before runtime AUTO input scans can occupy the UI loop.
  runPlatformFeedbackServices(false);
  autoDetectBenchmarkMark(ADBENCH_MAIN_REFRESH);
  autoDetectBenchmarkMark(ADBENCH_BOOT_SETUP_DONE);
}

void runFirmwareLoop() {
#if defined(ENABLE_BOOTTRACE_OLED)
  static bool bootTraceLoopMarkerShown = false;
  if (!bootTraceLoopMarkerShown && millis() >= 1000) {
    showBootTraceMarker("7 loop");
    bootTraceLoopMarkerShown = true;
  }
#endif
  runMainRuntimeLoop();
}
