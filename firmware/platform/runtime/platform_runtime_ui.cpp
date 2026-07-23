#include "../../product_config.h"

#include "platform_runtime_ui.h"

#include "../../core/controller_runtime_core.h"
#include "../../core/firmware_support.h"
#include "../../input/runtime/input_mode_runtime.h"
#include "../../menu/menu_idle_runtime.h"
#include "../../platform/latency_service_mask.h"
#include "../../platform/latency_trace_gpio.h"
#include "platform_feedback_runtime.h"
#include "platform_menu_runtime.h"

namespace {

constexpr uint32_t kResetButtonIdlePollIntervalUs = 16000;
constexpr uint32_t kControllerUiServiceIntervalUs = 16666;

uint32_t lastResetButtonIdlePollUs = 0;
uint32_t lastControllerUiServiceUs = 0;

ButtonEvent updateResetButtonForRuntime() {
  const uint32_t nowUs = micros();
  if (resetButton.isPressed() ||
      lastResetButtonIdlePollUs == 0 ||
      (uint32_t)(nowUs - lastResetButtonIdlePollUs) >= kResetButtonIdlePollIntervalUs) {
    lastResetButtonIdlePollUs = nowUs;
    return resetButton.update();
  }
  return BTN_EVENT_NONE;
}

}  // namespace

bool runPlatformPrePollUi() {
  ButtonEvent modeEvent = BTN_EVENT_NONE;
  {
    LatencyPhaseTraceScope modeButtonTrace(LATENCY_TRACE_PHASE_MODE_BUTTON);
    modeEvent = modeButton.update();
  }
  ButtonEvent resetEvent = BTN_EVENT_NONE;
  {
    LatencyPhaseTraceScope resetButtonTrace(LATENCY_TRACE_PHASE_RESET_BUTTON);
    resetEvent = updateResetButtonForRuntime();
  }

  if (handlePendingAutoDetectRebootRelease()) {
    return true;
  }

  if (wakeFromVisibleScreensaver(modeEvent, resetEvent)) {
    return true;
  }

  {
    LatencyPhaseTraceScope menuHandleTrace(LATENCY_TRACE_PHASE_MENU_HANDLE);
    handleMenuAndButtonUi(modeEvent, resetEvent);
  }
  return false;
}

void runPlatformRuntimeControllerUi(bool updated) {
  (void)updated;
  const uint32_t nowUs = micros();
  bool consumedControllerWake = false;
  if (lastControllerUiServiceUs == 0 ||
      (uint32_t)(nowUs - lastControllerUiServiceUs) >= kControllerUiServiceIntervalUs) {
    lastControllerUiServiceUs = nowUs;
    if (!latencyServiceDisabled(LATENCY_SERVICE_CONTROLLER_CACHE)) {
      cacheProcessedControllerOutputState();
    }
    if (!latencyServiceDisabled(LATENCY_SERVICE_IDLE_WAKE)) {
      consumedControllerWake = handleControllerIdleWakeActivity();
    }
  }
  if (!consumedControllerWake && !latencyServiceDisabled(LATENCY_SERVICE_MENU_HOTKEYS)) {
    runPlatformMenuHotkeys();
  }
}

void runPlatformRuntimeFeedbackUi(bool updated) {
  runPlatformFeedbackServices(updated);
}
