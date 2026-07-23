#pragma once

#include <stdint.h>

#include "../../core/device_mode.h"
#include "input_autodetect_flags.h"
#include "input_autodetect_runtime_state.h"

enum AutoDetectResult : uint8_t {
  AUTODETECT_NONE = 0,
  AUTODETECT_JOYBUS_N64,
  AUTODETECT_JOYBUS_GC,
  AUTODETECT_JOYBUS_GBA,
  AUTODETECT_DREAMCAST,
  AUTODETECT_PSX,
  AUTODETECT_SNES,
  AUTODETECT_NES,
  AUTODETECT_VBOY,
  AUTODETECT_SATURN,
  AUTODETECT_MEGADRIVE,
  AUTODETECT_PCE,
  AUTODETECT_3DO,
  AUTODETECT_WII,
  AUTODETECT_JAGUAR,
  AUTODETECT_NEOGEO,
  AUTODETECT_ATARI_DRIVING,
  AUTODETECT_ATARI_PADDLE,
  AUTODETECT_SMS,
  AUTODETECT_JPC,
};

#ifdef ENABLE_INPUT_AUTODETECT
const char* autoDetectResultName(AutoDetectResult result);
DeviceEnum autoDetectResultToDeviceMode(AutoDetectResult result);
#else
inline const char* autoDetectResultName(AutoDetectResult) { return ""; }
inline DeviceEnum autoDetectResultToDeviceMode(AutoDetectResult) { return RZORD_NONE; }
#endif

extern AutoDetectResult autoDetectResult;

void setAutoDetectResult(AutoDetectResult result);

void clearAutoDetectResult();

constexpr uint32_t inputAutoDetectRetryIntervalMs() {
  return 1200;
}

constexpr uint32_t inputAutoDetectUnresolvedRetryIntervalMs() {
  return 250;
}

constexpr uint32_t inputAutoDetectNoInputRetryIntervalMs() {
  return 5000;
}

constexpr uint32_t inputAutoDetectFastNoInputRetryIntervalMs() {
  return 250;
}

constexpr uint8_t inputAutoDetectNoInputFullScanEveryQuickMisses() {
  return 8;
}

constexpr uint32_t inputAutoDetectDisconnectDelayMs() {
  return 1000;
}

constexpr uint32_t inputAutoDetectBootSettleMs() {
  return 60;
}

constexpr uint8_t kAutoInputResolveSourceAutoHomeReenum = 6;

constexpr uint32_t inputAutoDetectInterPortNoHitDelayMs() {
  return 30;
}

constexpr uint32_t inputAutoDetectInterPortHitDelayMs() {
  return 8;
}

constexpr uint32_t inputAutoDetectPassiveAssistWindowMs() {
  return 120;
}

void scheduleResolvedAutoDetectRetry(uint32_t now);

void scheduleUnresolvedAutoDetectRetry(uint32_t now);

void preserveDetectedInputModeForReboot(DeviceEnum mode);
void preserveInputModeForPlayerCountReboot(DeviceEnum mode, uint8_t port_count);
void preserveAutoDetectHomeForReboot();
uint32_t autoInputResolvedGraceMs(DeviceEnum mode);
void prepareAutoDetectInputModeAtBoot();
bool restoreAutoDetectModeForDisconnect();
bool inputAutoDetectSuspendedForIdleUi();
bool shouldDeferAutoDetectHotSwap(uint32_t now, bool waitingForInitialResolve,
                                  uint32_t disconnect_delay_ms);
inline DeviceEnum applyPassiveAutoDetectResult(AutoDetectResult result,
                                               uint32_t elapsed_ms,
                                               uint8_t port,
                                               uint8_t extra_flags = 0) {
  setAutoDetectResult(result);
  DeviceEnum passiveMode = autoDetectResultToDeviceMode(autoDetectResult);
  uint8_t passiveFlags = INPUT_AUTODETECT_TIMING_PASSIVE | extra_flags;
  if (passiveMode != RZORD_NONE) {
    passiveFlags |= INPUT_AUTODETECT_TIMING_VALID;
  }
  recordInputAutodetectTiming(elapsed_ms, port, passiveFlags);
  return passiveMode;
}
DeviceEnum runAutoDetection(bool is_hotswap = false);
DeviceEnum runAutoDetectionFastStrictBusOnly(bool is_hotswap = true);
DeviceEnum runAutoDetectionFastCommonOnly(bool is_hotswap = true);
DeviceEnum runAutoDetectionPsxQuickOnly(bool is_hotswap = true);
bool checkAutoDetectHotSwap();
