#pragma once

#include <stdint.h>

#include "input_autodetect_runtime.h"

struct PassiveAssistScanResult {
  AutoDetectResult result;
  uint8_t port;
  bool assist_candidate_pending;
};

AutoDetectResult detectAutoInputPort(uint8_t port, bool is_hotswap = false);
AutoDetectResult detectAutoInputPortPsxOnly(uint8_t port, bool is_hotswap = false);
AutoDetectResult detectAutoInputPortDreamcastOnly(uint8_t port, bool is_hotswap = false);
AutoDetectResult detectAutoInputPortShiftRegisterOnly(uint8_t port, bool is_hotswap = false);
PassiveAssistScanResult runPassiveAssistedAutoDetectScan();
PassiveAssistScanResult runPassiveOnlyAutoDetectScan();
AutoDetectResult detectJaguarAssistHold(uint8_t port);
void resetAutoDetectPins();
void restoreAutoDetectSharedInputLines(DeviceEnum mode);

#ifdef AUTODETECT_DEBUG
void getAutoDetectDebugStr(char* buf, size_t len);
void getAutoDetectOledDebugLine(uint8_t port, char* buf, size_t len);
void getAutoDetectSerialDebugLine(uint8_t port, char* buf, size_t len);
bool getAutoDetectJaguarSerialDebugLine(uint8_t port, char* buf, size_t len);
#endif
