#include "../../product_config.h"

#include "input_poll_runtime.h"

#include <Arduino.h>

#include "../../core/controller_frame_state.h"
#include "../../core/controller_runtime_core.h"
#include "../../platform/latency_trace_gpio.h"
#include "../../platform/latency_test.h"
#include "../../core/device_runtime_state.h"
#include "../../menu/menu_ui_state.h"
#include "../autodetect/input_autodetect_runtime.h"
#include "../autodetect/input_autodetect_runtime_state.h"
#ifdef ENABLE_INPUT_JVS
#include "../jvs/jvs_host_runtime.h"
#endif
#include "input_adapter_runtime.h"
#include "../state/input_poll_runtime_state.h"

namespace {

bool shouldSkipInputPollForPinDebug() {
  #ifdef ADAPT_INPUT_RETRO
  return pinDebugActive;
  #else
  return false;
  #endif
}

void serviceFastInputRuntimes() {
  #ifdef ENABLE_INPUT_JVS
  if (deviceMode == RZORD_JVS) {
    serviceJvsHostRuntime();
  }
  #endif
}

bool __not_in_flash_func(pollInputModuleIfReady)(bool* updated_out) {
  if (shouldSkipInputPollForPinDebug()) {
    return false;
  }

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
  if (!polled) {
    return false;
  }
  if (updated_out) {
    *updated_out = updated;
  }

  if (latencyTest.isEnabled()) {
    for (uint8_t i = 0; i < max_devices && i < MAX_USB_OUT; ++i) {
      const controller_state_t& frame = controllerFrameConst(i);
      if (frame.connected) {
        latencyTest.observeButtons(0, frame.digital_buttons, true);
        break;
      }
      if (i == 0) {
        latencyTest.observeButtons(0, 0, false);
      }
    }
  }

  {
    LatencyPhaseTraceScope processPhaseTrace(LATENCY_TRACE_PHASE_PROCESS);
    LatencyTraceGpioScope processTrace(PIN_LATENCY_TRACE_PROCESS);
    processPolledInputFrame(updated);
  }
  return true;
}

bool shouldDeferInitialAutoDetectUntilHomeVisible() {
#if defined(ENABLE_INPUT_AUTODETECT) && defined(USE_I2C_DISPLAY)
  return deviceMode == RZORD_AUTODETECT && !mainDisplayInitialized;
#else
  return false;
#endif
}

bool handleAutoDetectHotSwapIfNeeded() {
  #ifdef ENABLE_INPUT_AUTODETECT
  if (inputAutoDetectModeActive() && !isMenuOpen && !isQuickConfigOpen) {
    if (shouldDeferInitialAutoDetectUntilHomeVisible()) {
      return false;
    }
    if (checkAutoDetectHotSwap()) {
      return true;
    }
  }
  #endif
  return false;
}

}  // namespace

void resetInputPollSchedule() {
  resetInputLastPollAtUs();
}

bool __not_in_flash_func(pollInputModuleIfDue)(uint32_t& last_poll_at_us, bool* updated) {
  const uint32_t now_us = micros();
  return pollActiveInputAdapterIfDue(now_us, last_poll_at_us, updated);
}

bool __not_in_flash_func(pollInputModuleIfDue)(bool* updated) {
  uint32_t last_poll_at_us = inputLastPollAtUs();
  bool polled = pollInputModuleIfDue(last_poll_at_us, updated);
  if (polled) {
    setInputLastPollAtUs(last_poll_at_us);
  }
  return polled;
}

bool __not_in_flash_func(runInputRuntimeCycle)(bool* polled_out, bool* updated_out) {
  serviceFastInputRuntimes();
  bool updated = false;
  const bool polled = pollInputModuleIfReady(&updated);
  if (polled_out) {
    *polled_out = polled;
  }
  if (updated_out) {
    *updated_out = updated;
  }
  return handleAutoDetectHotSwapIfNeeded();
}
