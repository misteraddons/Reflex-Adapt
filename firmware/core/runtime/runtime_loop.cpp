#include "../../product_config.h"

#include "runtime_loop.h"

#include "../konami_code.h"

#include <Arduino.h>
#include <EEPROM.h>
#include <stdlib.h>

#if defined(ARDUINO_ARCH_RP2040)
#include <hardware/watchdog.h>
#endif

#ifdef ADAPT_OUTPUT_USB_DEVICE
#include <Adafruit_TinyUSB.h>
#endif

#ifdef ENABLE_INPUT_JVS
#include "../../input/jvs/jvs_host_runtime.h"
#endif
#ifdef ENABLE_INPUT_DREAMCAST
#include "../../input/dreamcast/Input_Dreamcast.h"
#endif
#if defined(ENABLE_USB_AUTH_SIDECAR) || defined(ENABLE_INPUT_USB)
#include "../../input/usb_host/input_usb_host_service.h"
#endif
#ifdef ENABLE_USB_BT_HCI
#include "../../input/usb_bt/usb_bt_hci_host.h"
#endif
#ifdef ENABLE_USB_XINPUT_AUTH_HOST
#include "../../input/usb_host/usb_xinput_host.h"
#endif
#ifdef ENABLE_INPUT_USB
#include <input_usb.h>
#endif
#ifdef ENABLE_INPUT_AUTODETECT
#include "../../input/autodetect/input_autodetect_runtime_state.h"
#endif
#ifdef ENABLE_USB_AUTH_SIDECAR
#include "../../output/auth/ps_auth_dongle_runtime.h"
#endif
#ifdef ENABLE_EXPERIMENTAL_XBOXONE_OUTPUT
#include "../../output/auth/xbone_auth_passthrough.h"
#include "../../output/xboxone/out_xboxone.h"
#endif
#include "../../input/runtime/input_poll_runtime.h"
#include "../../input/runtime/input_adapter_runtime.h"
#include "../../features/feature_module.h"
#include "../../menu/menu_idle_runtime.h"
#include "../../menu/menu_mode_state.h"
#include "../../menu/menu_runtime_state.h"
#include "../../menu/menu_ui_state.h"
#include "../../menu/menu_working_state.h"
#include "../../output/output_runtime_state.h"
#include "../../output/runtime/output_loop_runtime.h"
#include "../../platform/runtime/platform_background_runtime.h"
#include "../../platform/runtime/platform_feedback_runtime.h"
#include "../../platform/runtime/platform_runtime_ui.h"
#include "../../platform/latency_trace_gpio.h"
#include "../../platform/latency_test.h"
#include "../../platform/latency_service_mask.h"
#include "../controller_frame_state.h"
#include "../controller_delivery_state.h"
#include "../adapt_state_stream_runtime.h"
#include "../device_runtime_state.h"
#include "../firmware_support.h"
#include "../input_overlay_runtime.h"
#include "../oled_serial_runtime.h"
#include "../serial_debug_runtime.h"
#include "../settings_store.h"
#include "../controller_runtime_core.h"
#include "../turbo.h"
#include "runtime_debug_cdc_state.h"
#include "runtime_serial_debug.h"

namespace {

constexpr uint32_t kActiveFeedbackUiIntervalUs = 33333;
constexpr uint32_t kIdleFeedbackUiIntervalUs = 16667;
uint32_t lastPostPollUiServiceUs = 0;
uint32_t lastFeedbackUiUs = 0;
bool pendingRuntimeUiUpdate = false;

bool shouldUseMinimalJvsDebugLoop() {
  return false;
}

bool __not_in_flash_func(outputFramePreparationRequired)() {
  if (latencyTest.isEnabled() || outputMode == OUTPUT_PS3) {
    return true;
  }
#ifdef ENABLE_OUTPUT_PS5
  if (get_effective_output_mode() == OUTPUT_PS5) {
    return true;
  }
#endif

  for (uint8_t i = 0; i < MAX_USB_OUT; ++i) {
    if (controllerFrameNeedsDelivery(i)) {
      return true;
    }
  }
  return false;
}

void runDeferredSerialRuntimeTasks() {
  serialDebugRuntimeTask();

  #ifdef ENABLE_RUNTIME_SERIAL_DEBUG_CDC
  if (runtimeDebugCdcEnabled && !is_ps5_timing_quiet_mode_active()) {
    processRuntimeSerialCommands();
    if (!latencyServiceDisabled(LATENCY_SERVICE_SERIAL_LOG) &&
        latencyTest.isSerialLogEnabled()) {
      latencyTest.flushLog(runtimeDebugCdc);
    }
    if (!latencyServiceDisabled(LATENCY_SERVICE_ADAPT_STATE) &&
        adaptStateIsEnabled()) {
      adaptStateTask(runtimeDebugCdc);
    }
    if (!latencyServiceDisabled(LATENCY_SERVICE_INPUT_OVERLAY) &&
        inputOverlayIsEnabled()) {
      inputOverlayTask(runtimeDebugCdc);
    }
    if (!latencyServiceDisabled(LATENCY_SERVICE_OLED_SERIAL) &&
        oledSerialIsEnabled()) {
      oledSerialTask(runtimeDebugCdc);
    }
#ifdef ENABLE_USB_AUTH_SERIAL_TRACE
    ps_auth_dongle_trace_flush(runtimeDebugCdc);
#endif
  }
  #endif
}

void runPostPollUiIfNeeded(bool polled, bool updated) {
  if (latencyServiceDisabled(LATENCY_SERVICE_POSTPOLL_UI)) {
    pendingRuntimeUiUpdate = false;
    return;
  }
  if (polled) {
    pendingRuntimeUiUpdate = pendingRuntimeUiUpdate || updated;
    const uint32_t nowUs = micros();
    if (lastPostPollUiServiceUs != 0 &&
        (uint32_t)(nowUs - lastPostPollUiServiceUs) < kIdleFeedbackUiIntervalUs) {
      return;
    }
    lastPostPollUiServiceUs = nowUs;
  } else {
    lastPostPollUiServiceUs = micros();
  }

  const bool uiUpdated = pendingRuntimeUiUpdate;
  pendingRuntimeUiUpdate = false;

  runPlatformRuntimeControllerUi(uiUpdated);

  const uint32_t nowUs = micros();
  const uint32_t feedbackIntervalUs =
    uiUpdated ? kActiveFeedbackUiIntervalUs : kIdleFeedbackUiIntervalUs;
  if (lastFeedbackUiUs != 0 &&
      (uint32_t)(nowUs - lastFeedbackUiUs) < feedbackIntervalUs) {
    return;
  }
  lastFeedbackUiUs = nowUs;

  LatencyPhaseTraceScope uiPhaseTrace(LATENCY_TRACE_PHASE_UI);
  runPlatformRuntimeFeedbackUi(uiUpdated);
}

void runMinimalJvsUsbTransportTasks() {
  #ifdef ADAPT_OUTPUT_USB_DEVICE
  #if defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
  TinyUSB_Device_Task();
  #else
  TinyUSBDevice.task();
  #endif
  #endif
}

void runMinimalJvsDebugLoop() {
  runMinimalJvsUsbTransportTasks();
  runPlatformBackgroundTasks();
  turbo.update();

  if (runPlatformPrePollUi()) {
    runMinimalJvsUsbTransportTasks();
    return;
  }

  #ifdef ENABLE_INPUT_JVS
  serviceJvsHostRuntime();
  #endif

  bool updated = false;
  bool polled = false;
  {
    LatencyPhaseTraceScope pollPhaseTrace(LATENCY_TRACE_PHASE_POLL);
    LatencyTraceGpioScope pollTrace(PIN_LATENCY_TRACE_POLL);
    polled = pollInputModuleIfDue(&updated);
    if (!polled) {
      pollPhaseTrace.cancel();
    }
  }
  if (polled) {
    {
      LatencyPhaseTraceScope processPhaseTrace(LATENCY_TRACE_PHASE_PROCESS);
      LatencyTraceGpioScope processTrace(PIN_LATENCY_TRACE_PROCESS);
      processPolledInputFrame(updated);
    }
  }

  {
    LatencyPhaseTraceScope preparePhaseTrace(LATENCY_TRACE_PHASE_PREPARE);
    LatencyTraceGpioScope prepareTrace(PIN_LATENCY_TRACE_PREPARE);
    const bool latencyTestActive = latencyTest.isEnabled();
    if (latencyTestActive) {
      latencyTest.prepareOutputFrame();
    }
    finalizeControllerFrameForOutput();
    if (latencyTestActive) {
      latencyTest.noteOutputFramePrepared();
    }
  }
  {
    LatencyPhaseTraceScope sendPhaseTrace(LATENCY_TRACE_PHASE_SEND);
    LatencyTraceGpioScope sendTrace(PIN_LATENCY_TRACE_SEND);
    sendPreparedOutputFrame();
  }
  restoreControllerFrameAfterOutputSend();
  if (latencyTest.isEnabled()) {
    latencyTest.restoreOutputFrame();
  }
  updateKonamiCodeObserver(polled);
  runPostPollUiIfNeeded(polled, updated);

  runMinimalJvsUsbTransportTasks();
}

}  // namespace

void __not_in_flash_func(runMainRuntimeLoop)() {
  //USBHost.task();
  //tud_task();
  //hid_task();

  // Keep the controller-to-USB critical path first: sync the USB device stack,
  // read inputs, process transforms, prepare the output frame, and submit the
  // report before CDC/menu/OLED/WebHID/background services can stall the loop.
  runOutputTransportSyncTasks();

  if (shouldUseMinimalJvsDebugLoop()) {
    runMinimalJvsDebugLoop();
    return;
  }

  bool polled = false;
  bool updated = false;
  if (runInputRuntimeCycle(&polled, &updated)) {
    runDeferredSerialRuntimeTasks();
    return;
  }

  runPostPollOutputTasks(polled);
  updateKonamiCodeObserver(polled);
  runActiveInputAdapterAfterOutputFrameSent(polled, updated);

  {
    LatencyPhaseTraceScope prePollUiTrace(LATENCY_TRACE_PHASE_PREPOLL_UI);
    if (!latencyServiceDisabled(LATENCY_SERVICE_PREPOLL_UI) && runPlatformPrePollUi()) {
      runDeferredSerialRuntimeTasks();
      return;
    }
  }

  runPostPollUiIfNeeded(polled, updated);
  runLoopBackgroundTasks();

  {
    LatencyPhaseTraceScope pendingOutputTrace(LATENCY_TRACE_PHASE_PENDING_OUTPUT);
    if (!latencyServiceDisabled(LATENCY_SERVICE_PENDING_OUTPUT)) {
      processPendingOutputRuntimeTasks();
    }
  }

  #ifdef ENABLE_USB_BT_HCI
  usb_bt_hci_persist_task();
  #endif

  runDeferredSerialRuntimeTasks();

//  bool anyConnected = input_report[0].connected || input_report[1].connected || input_report[2].connected || input_report[3].connected;
//  #ifdef LED_BUILTIN
//    gpio_put(LED_BUILTIN, anyConnected ? HIGH : LOW);
//  #endif
}

void runLoopBackgroundTasks() {
  {
    LatencyPhaseTraceScope outputBgTrace(LATENCY_TRACE_PHASE_OUTPUT_BG);
    if (!latencyServiceDisabled(LATENCY_SERVICE_OUTPUT_BG)) {
      runOutputBackgroundTasks();
    }
  }
  {
    LatencyPhaseTraceScope platformBgTrace(LATENCY_TRACE_PHASE_PLATFORM_BG);
    if (!latencyServiceDisabled(LATENCY_SERVICE_PLATFORM_BG)) {
      runPlatformBackgroundTasks();
    }
  }
  {
    LatencyPhaseTraceScope turboTrace(LATENCY_TRACE_PHASE_TURBO);
    if (!latencyServiceDisabled(LATENCY_SERVICE_TURBO_TASK)) {
      turbo.update();
    }
  }
}

void __not_in_flash_func(runPostPollOutputTasks)(bool polled) {
  // Most loop iterations have no changed controller frame. Avoid copying,
  // transforming, and restoring every controller in that case. The transport
  // task still runs below so host feedback and output-specific housekeeping
  // remain live.
  if (!outputFramePreparationRequired()) {
    sendPreparedOutputFrame();
    return;
  }

  // Neo-Geo polls on every runtime pass and processPolledInputFrame() has
  // already applied every digital transform needed by this mode. Avoid
  // copying and rebuilding the same frame before its latency-sensitive send.
  const bool useProcessedFrameDirectly =
      polled && deviceMode == RZORD_NEOGEO && !menuOwnsControllerInput();

  if (!useProcessedFrameDirectly) {
    LatencyPhaseTraceScope preparePhaseTrace(LATENCY_TRACE_PHASE_PREPARE);
    LatencyTraceGpioScope prepareTrace(PIN_LATENCY_TRACE_PREPARE);
    const bool latencyTestActive = latencyTest.isEnabled();
    if (latencyTestActive) {
      latencyTest.prepareOutputFrame();
    }
    finalizeControllerFrameForOutput();
    if (latencyTestActive) {
      latencyTest.noteOutputFramePrepared();
    }
  }
  {
    LatencyPhaseTraceScope sendPhaseTrace(LATENCY_TRACE_PHASE_SEND);
    LatencyTraceGpioScope sendTrace(PIN_LATENCY_TRACE_SEND);
    sendPreparedOutputFrame();
  }
  restoreControllerFrameAfterOutputSend();
  if (latencyTest.isEnabled()) {
    latencyTest.restoreOutputFrame();
  }
}
