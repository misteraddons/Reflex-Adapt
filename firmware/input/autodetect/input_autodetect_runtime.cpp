#include "../../product_config.h"
#include "../../firmware_platform_config.h"

#include <Arduino.h>
#include "../../core/device_runtime_state.h"
#include "../../core/firmware_support.h"
#include "../../platform/button_handler.h"
#include "../../platform/buzzer.h"
#include "input_autodetect_runtime.h"
#include "input_autodetect_benchmark.h"
#include "input_autodetect_passive_internal.h"
#include "input_autodetect_runtime_internal.h"
#include "input_autodetect_runtime_state.h"
#include "input_autodetect_support.h"
#include "../mixed/input_mixed_runtime_state.h"
#include "../runtime/input_adapter_runtime.h"
#include "../runtime/input_frame_runtime.h"
#include "../../output/output_runtime_state.h"

AutoDetectResult autoDetectResult = AUTODETECT_NONE;

void setAutoDetectResult(AutoDetectResult result) {
  autoDetectResult = result;
}

void clearAutoDetectResult() {
  setAutoDetectResult(AUTODETECT_NONE);
  clearInputMixedModeState();
}

#ifdef ENABLE_INPUT_AUTODETECT
namespace {

#if AUTODETECT_ENABLE_PASSIVE_ASSIST
AutoDetectResult detectPassiveAssistOnPort(uint8_t port) {
  #ifdef ENABLE_INPUT_JAGUAR
  AutoDetectResult jaguarResult = detectJaguarAssistHold(port);
  if (jaguarResult != AUTODETECT_NONE) {
    return jaguarResult;
  }
  #endif
  return input_autodetect_passive_internal::probePassiveOnlyAssistedRoute(port);
}

bool passiveAssistPendingOnPort(uint8_t port) {
  #ifdef ENABLE_INPUT_JAGUAR
  if (input_autodetect_passive_internal::jaguarAssistHoldPresent(port)) {
    return true;
  }
  #endif
  return input_autodetect_passive_internal::passiveHoldRoutePresent(port);
}

void fillPassiveAssistPortResults(AutoDetectResult (&portResults)[INPUT_MIXED_PORT_COUNT],
                                  DeviceEnum (&portModes)[INPUT_MIXED_PORT_COUNT],
                                  uint8_t& detected_port, uint8_t& timing_flags) {
  uint32_t assist_wait_started = millis();
  while ((millis() - assist_wait_started) < inputAutoDetectPassiveAssistWindowMs()) {
    bool anyPending = false;
    bool anyResolved = false;
    for (uint8_t port = 0; port < INPUT_MIXED_PORT_COUNT; ++port) {
      if (portModes[port] != RZORD_NONE) {
        continue;
      }

      AutoDetectResult passiveResult = detectPassiveAssistOnPort(port);
      if (passiveResult != AUTODETECT_NONE) {
        portResults[port] = passiveResult;
        portModes[port] = autoDetectResultToDeviceMode(passiveResult);
        if (portModes[port] != RZORD_NONE) {
          detected_port = port;
          timing_flags |= INPUT_AUTODETECT_TIMING_PASSIVE;
          anyResolved = true;
          continue;
        }
      }
      anyPending |= passiveAssistPendingOnPort(port);
    }

    if (anyResolved || !anyPending) {
      break;
    }
    delayWithButtonPolling(5);
  }
}
#endif

void selectPrimaryAutoDetectResult(const AutoDetectResult (&portResults)[INPUT_MIXED_PORT_COUNT],
                                   uint8_t& detected_port) {
  if (portResults[0] != AUTODETECT_NONE) {
    autoDetectResult = portResults[0];
    detected_port = 0;
    return;
  }
  if (portResults[1] != AUTODETECT_NONE) {
    autoDetectResult = portResults[1];
    detected_port = 1;
    return;
  }
  autoDetectResult = AUTODETECT_NONE;
  detected_port = 0xFF;
}

bool runHotswapPsxQuickPass(AutoDetectResult (&portResults)[INPUT_MIXED_PORT_COUNT],
                            DeviceEnum (&portModes)[INPUT_MIXED_PORT_COUNT],
                            uint8_t& detected_port) {
  #ifdef ENABLE_INPUT_PSX
  for (uint8_t port = 0; port < INPUT_MIXED_PORT_COUNT; ++port) {
    portResults[port] = detectAutoInputPortPsxOnly(port, true);
    portModes[port] = autoDetectResultToDeviceMode(portResults[port]);
    if (portModes[port] != RZORD_NONE) {
      autoDetectResult = portResults[port];
      detected_port = port;
      return true;
    }
    resetAutoDetectPins();
    delayWithButtonPolling(inputAutoDetectInterPortHitDelayMs());
  }
  #else
  (void)portResults;
  (void)portModes;
  (void)detected_port;
  #endif
  return false;
}

bool runHotswapDreamcastQuickPass(AutoDetectResult (&portResults)[INPUT_MIXED_PORT_COUNT],
                                  DeviceEnum (&portModes)[INPUT_MIXED_PORT_COUNT],
                                  uint8_t& detected_port) {
  #ifdef ENABLE_INPUT_DREAMCAST
  for (uint8_t port = 0; port < INPUT_MIXED_PORT_COUNT; ++port) {
    portResults[port] = detectAutoInputPortDreamcastOnly(port, true);
    portModes[port] = autoDetectResultToDeviceMode(portResults[port]);
    if (portModes[port] != RZORD_NONE) {
      autoDetectResult = portResults[port];
      detected_port = port;
      return true;
    }
    resetAutoDetectPins();
    delayWithButtonPolling(inputAutoDetectInterPortHitDelayMs());
  }
  #else
  (void)portResults;
  (void)portModes;
  (void)detected_port;
  #endif
  return false;
}

bool runHotswapFastStrictBusPass(AutoDetectResult (&portResults)[INPUT_MIXED_PORT_COUNT],
                                 DeviceEnum (&portModes)[INPUT_MIXED_PORT_COUNT],
                                 uint8_t& detected_port) {
  if (runHotswapPsxQuickPass(portResults, portModes, detected_port)) {
    return true;
  }
  return runHotswapDreamcastQuickPass(portResults, portModes, detected_port);
}

bool isShiftRegisterAutoDetectResult(AutoDetectResult result) {
  return result == AUTODETECT_NES ||
         result == AUTODETECT_SNES ||
         result == AUTODETECT_VBOY;
}

bool runHotswapShiftRegisterQuickPass(AutoDetectResult (&portResults)[INPUT_MIXED_PORT_COUNT],
                                      DeviceEnum (&portModes)[INPUT_MIXED_PORT_COUNT],
                                      uint8_t& detected_port) {
  for (uint8_t port = 0; port < INPUT_MIXED_PORT_COUNT; ++port) {
    portResults[port] = detectAutoInputPortShiftRegisterOnly(port, true);
    if (!isShiftRegisterAutoDetectResult(portResults[port])) {
      portResults[port] = AUTODETECT_NONE;
    }
    portModes[port] = autoDetectResultToDeviceMode(portResults[port]);
    if (detected_port == 0xFF && portModes[port] != RZORD_NONE) {
      detected_port = port;
    }
    if ((port + 1) < INPUT_MIXED_PORT_COUNT) {
      resetAutoDetectPins();
      delayWithButtonPolling(inputAutoDetectInterPortHitDelayMs());
    }
  }

  if (detected_port == 0xFF) {
    return false;
  }

  selectPrimaryAutoDetectResult(portResults, detected_port);
  return true;
}

}  // namespace

DeviceEnum runAutoDetectionPsxQuickOnly(bool is_hotswap) {
  autoDetectBenchmarkMark(ADBENCH_DETECT_START, is_hotswap ? 1 : 0);
  uint32_t detect_started = millis();
  uint8_t detected_port = 0xFF;
  uint8_t timing_flags = is_hotswap ? INPUT_AUTODETECT_TIMING_HOTSWAP : 0;
  AutoDetectResult portResults[INPUT_MIXED_PORT_COUNT] = { AUTODETECT_NONE, AUTODETECT_NONE };
  DeviceEnum portModes[INPUT_MIXED_PORT_COUNT] = { RZORD_NONE, RZORD_NONE };

  resetAutoDetectPins();
  delayWithButtonPolling(inputAutoDetectBootSettleMs());
  clearAutoDetectResult();

  if (runHotswapPsxQuickPass(portResults, portModes, detected_port)) {
    configureInputMixedAutoDetectModes(portModes[0], portModes[1]);
    timing_flags |= INPUT_AUTODETECT_TIMING_VALID;
    const uint32_t elapsed_ms = millis() - detect_started;
    recordInputAutodetectTiming(elapsed_ms, detected_port, timing_flags);
    autoDetectBenchmarkMark(ADBENCH_DETECT_END, detected_port, elapsed_ms > 65535u ? 65535u : (uint16_t)elapsed_ms);
    #ifdef AUTODETECT_DEBUG
    logInputAutodetectDebug("detect", is_hotswap, detected_port, portModes[detected_port]);
    #endif
    return portModes[detected_port];
  }

  const uint32_t elapsed_ms = millis() - detect_started;
  recordInputAutodetectTiming(elapsed_ms, 0xFF, timing_flags);
  autoDetectBenchmarkMark(ADBENCH_DETECT_END, 0xFF, elapsed_ms > 65535u ? 65535u : (uint16_t)elapsed_ms);
  #ifdef AUTODETECT_DEBUG
  logInputAutodetectDebug("none", is_hotswap, 0xFF, RZORD_NONE);
  #endif

  return RZORD_NONE;
}

DeviceEnum runAutoDetectionFastStrictBusOnly(bool is_hotswap) {
  autoDetectBenchmarkMark(ADBENCH_DETECT_START, is_hotswap ? 1 : 0);
  uint32_t detect_started = millis();
  uint8_t detected_port = 0xFF;
  uint8_t timing_flags = is_hotswap ? INPUT_AUTODETECT_TIMING_HOTSWAP : 0;
  AutoDetectResult portResults[INPUT_MIXED_PORT_COUNT] = { AUTODETECT_NONE, AUTODETECT_NONE };
  DeviceEnum portModes[INPUT_MIXED_PORT_COUNT] = { RZORD_NONE, RZORD_NONE };

  resetAutoDetectPins();
  delayWithButtonPolling(inputAutoDetectBootSettleMs());
  clearAutoDetectResult();

  if (runHotswapFastStrictBusPass(portResults, portModes, detected_port)) {
    configureInputMixedAutoDetectModes(portModes[0], portModes[1]);
    timing_flags |= INPUT_AUTODETECT_TIMING_VALID;
    const uint32_t elapsed_ms = millis() - detect_started;
    recordInputAutodetectTiming(elapsed_ms, detected_port, timing_flags);
    autoDetectBenchmarkMark(ADBENCH_DETECT_END, detected_port, elapsed_ms > 65535u ? 65535u : (uint16_t)elapsed_ms);
    #ifdef AUTODETECT_DEBUG
    logInputAutodetectDebug("detect", is_hotswap, detected_port, portModes[detected_port]);
    #endif
    return portModes[detected_port];
  }

  const uint32_t elapsed_ms = millis() - detect_started;
  recordInputAutodetectTiming(elapsed_ms, 0xFF, timing_flags);
  autoDetectBenchmarkMark(ADBENCH_DETECT_END, 0xFF, elapsed_ms > 65535u ? 65535u : (uint16_t)elapsed_ms);
  #ifdef AUTODETECT_DEBUG
  logInputAutodetectDebug("none", is_hotswap, 0xFF, RZORD_NONE);
  #endif

  return RZORD_NONE;
}

DeviceEnum runAutoDetectionFastCommonOnly(bool is_hotswap) {
  autoDetectBenchmarkMark(ADBENCH_DETECT_START, is_hotswap ? 1 : 0);
  uint32_t detect_started = millis();
  uint8_t detected_port = 0xFF;
  uint8_t timing_flags = is_hotswap ? INPUT_AUTODETECT_TIMING_HOTSWAP : 0;
  AutoDetectResult portResults[INPUT_MIXED_PORT_COUNT] = { AUTODETECT_NONE, AUTODETECT_NONE };
  DeviceEnum portModes[INPUT_MIXED_PORT_COUNT] = { RZORD_NONE, RZORD_NONE };

  resetAutoDetectPins();
  delayWithButtonPolling(inputAutoDetectBootSettleMs());
  clearAutoDetectResult();

  if (runHotswapShiftRegisterQuickPass(portResults, portModes, detected_port) ||
      runHotswapFastStrictBusPass(portResults, portModes, detected_port)) {
    configureInputMixedAutoDetectModes(portModes[0], portModes[1]);
    timing_flags |= INPUT_AUTODETECT_TIMING_VALID;
    const uint32_t elapsed_ms = millis() - detect_started;
    recordInputAutodetectTiming(elapsed_ms, detected_port, timing_flags);
    autoDetectBenchmarkMark(ADBENCH_DETECT_END, detected_port, elapsed_ms > 65535u ? 65535u : (uint16_t)elapsed_ms);
    #ifdef AUTODETECT_DEBUG
    logInputAutodetectDebug("detect-common-quick", is_hotswap, detected_port, portModes[detected_port]);
    #endif
    return portModes[detected_port];
  }

  const uint32_t elapsed_ms = millis() - detect_started;
  recordInputAutodetectTiming(elapsed_ms, 0xFF, timing_flags);
  autoDetectBenchmarkMark(ADBENCH_DETECT_END, 0xFF, elapsed_ms > 65535u ? 65535u : (uint16_t)elapsed_ms);
  #ifdef AUTODETECT_DEBUG
  logInputAutodetectDebug("none-common-quick", is_hotswap, 0xFF, RZORD_NONE);
  #endif

  return RZORD_NONE;
}

DeviceEnum runAutoDetection(bool is_hotswap) {
  autoDetectBenchmarkMark(ADBENCH_DETECT_START, is_hotswap ? 1 : 0);
  uint32_t detect_started = millis();
  uint8_t detected_port = 0xFF;
  uint8_t timing_flags = is_hotswap ? INPUT_AUTODETECT_TIMING_HOTSWAP : 0;
  AutoDetectResult portResults[INPUT_MIXED_PORT_COUNT] = { AUTODETECT_NONE, AUTODETECT_NONE };
  DeviceEnum portModes[INPUT_MIXED_PORT_COUNT] = { RZORD_NONE, RZORD_NONE };

  resetAutoDetectPins();
  delayWithButtonPolling(inputAutoDetectBootSettleMs());
  clearAutoDetectResult();

  if (is_hotswap &&
      runHotswapShiftRegisterQuickPass(portResults, portModes, detected_port)) {
    configureInputMixedAutoDetectModes(portModes[0], portModes[1]);
    timing_flags |= INPUT_AUTODETECT_TIMING_VALID;
    const uint32_t elapsed_ms = millis() - detect_started;
    recordInputAutodetectTiming(elapsed_ms, detected_port, timing_flags);
    autoDetectBenchmarkMark(ADBENCH_DETECT_END, detected_port, elapsed_ms > 65535u ? 65535u : (uint16_t)elapsed_ms);
    #ifdef AUTODETECT_DEBUG
    logInputAutodetectDebug("detect-shift-quick", is_hotswap, detected_port, portModes[detected_port]);
    #endif
    return portModes[detected_port];
  }

  if (is_hotswap &&
      runHotswapFastStrictBusPass(portResults, portModes, detected_port)) {
    configureInputMixedAutoDetectModes(portModes[0], portModes[1]);
    timing_flags |= INPUT_AUTODETECT_TIMING_VALID;
    const uint32_t elapsed_ms = millis() - detect_started;
    recordInputAutodetectTiming(elapsed_ms, detected_port, timing_flags);
    autoDetectBenchmarkMark(ADBENCH_DETECT_END, detected_port, elapsed_ms > 65535u ? 65535u : (uint16_t)elapsed_ms);
    #ifdef AUTODETECT_DEBUG
    logInputAutodetectDebug("detect", is_hotswap, detected_port, portModes[detected_port]);
    #endif
    return portModes[detected_port];
  }

  portResults[0] = detectAutoInputPort(0, is_hotswap);
  portModes[0] = autoDetectResultToDeviceMode(portResults[0]);
  if (portResults[0] != AUTODETECT_NONE) {
    detected_port = 0;
  }

  if (is_hotswap && isShiftRegisterAutoDetectResult(portResults[0])) {
    resetAutoDetectPins();
    delayWithButtonPolling(inputAutoDetectInterPortHitDelayMs());
    portResults[1] = detectAutoInputPortShiftRegisterOnly(1, true);
    if (!isShiftRegisterAutoDetectResult(portResults[1])) {
      portResults[1] = AUTODETECT_NONE;
    }
    portModes[1] = autoDetectResultToDeviceMode(portResults[1]);

    selectPrimaryAutoDetectResult(portResults, detected_port);
    configureInputMixedAutoDetectModes(portModes[0], portModes[1]);

    DeviceEnum detectedMode = (portModes[0] != RZORD_NONE)
        ? portModes[0]
        : autoDetectResultToDeviceMode(autoDetectResult);
    if (detectedMode != RZORD_NONE) {
      timing_flags |= INPUT_AUTODETECT_TIMING_VALID;
      const uint32_t elapsed_ms = millis() - detect_started;
      recordInputAutodetectTiming(elapsed_ms, detected_port, timing_flags);
      autoDetectBenchmarkMark(ADBENCH_DETECT_END, detected_port, elapsed_ms > 65535u ? 65535u : (uint16_t)elapsed_ms);
      #ifdef AUTODETECT_DEBUG
      logInputAutodetectDebug("detect-shift", is_hotswap, detected_port, detectedMode);
      #endif
      return detectedMode;
    }
  }

  resetAutoDetectPins();
  delayWithButtonPolling((portResults[0] == AUTODETECT_NONE)
      ? inputAutoDetectInterPortNoHitDelayMs()
      : inputAutoDetectInterPortHitDelayMs());
  portResults[1] = detectAutoInputPort(1, is_hotswap);
  portModes[1] = autoDetectResultToDeviceMode(portResults[1]);
  if (portResults[0] == AUTODETECT_NONE && portResults[1] != AUTODETECT_NONE) {
    autoDetectResult = portResults[1];
    detected_port = 1;
  }

  #if AUTODETECT_ENABLE_PASSIVE_ASSIST
  fillPassiveAssistPortResults(portResults, portModes, detected_port, timing_flags);
  #endif

  selectPrimaryAutoDetectResult(portResults, detected_port);
  configureInputMixedAutoDetectModes(portModes[0], portModes[1]);

  DeviceEnum detectedMode = (portModes[0] != RZORD_NONE)
      ? portModes[0]
      : autoDetectResultToDeviceMode(autoDetectResult);
  if (detectedMode != RZORD_NONE) {
    timing_flags |= INPUT_AUTODETECT_TIMING_VALID;
    const uint32_t elapsed_ms = millis() - detect_started;
    recordInputAutodetectTiming(elapsed_ms, detected_port, timing_flags);
    autoDetectBenchmarkMark(ADBENCH_DETECT_END, detected_port, elapsed_ms > 65535u ? 65535u : (uint16_t)elapsed_ms);
    #ifdef AUTODETECT_DEBUG
    logInputAutodetectDebug("detect", is_hotswap, detected_port, detectedMode);
    #endif
    return detectedMode;
  }

  const uint32_t elapsed_ms = millis() - detect_started;
  recordInputAutodetectTiming(elapsed_ms, 0xFF, timing_flags);
  autoDetectBenchmarkMark(ADBENCH_DETECT_END, 0xFF, elapsed_ms > 65535u ? 65535u : (uint16_t)elapsed_ms);
  #ifdef AUTODETECT_DEBUG
  logInputAutodetectDebug("none", is_hotswap, 0xFF, RZORD_NONE);
  #endif

  if (is_hotswap) {
    return RZORD_NONE;
  }

  #ifdef ENABLE_INPUT_SNES
    return RZORD_SNES;
  #elif defined(ENABLE_INPUT_NES)
    return RZORD_NES;
  #elif defined(ENABLE_INPUT_SATURN)
    return RZORD_SATURN;
  #else
    return RZORD_NONE;
  #endif
}

bool checkAutoDetectHotSwap() {
  // Host Auto owns USB control-transfer timing until it resolves and reboots
  // into the selected output. A full no-controller input scan is intentionally
  // blocking and can starve Windows enumeration if both detectors run at once.
  // Controller scanning begins on the resolved-output boot instead.
  if (is_auto_output_mode_selected() && autoDetectState == AUTO_STATE_IDLE) {
    return false;
  }

  const bool waitingForInitialResolve = (deviceMode == RZORD_AUTODETECT);
  static uint32_t lastCheckTraceMs = 0;
  const uint32_t now = millis();
  if (lastCheckTraceMs == 0 || (uint32_t)(now - lastCheckTraceMs) >= 250) {
    lastCheckTraceMs = now;
    autoDetectBenchmarkMark(
        ADBENCH_HOTSWAP_CHECK_ENTER,
        waitingForInitialResolve ? 1 : 0,
        inputHotSwapRevertedToAutoWhileDisconnected() ? 1 : 0);
  }

  if (!waitingForInitialResolve && inputAutoResolvedGraceActive(now)) {
    return false;
  }

  if (anyInputFrameConnected(MAX_USB_OUT) ||
      activeInputAdapterHasPhysicalConnectionForHotSwap()) {
    return handleConnectedAutoDetectHotSwap(waitingForInitialResolve);
  }
  return handleDisconnectedAutoDetectHotSwap(waitingForInitialResolve);
}

#endif
