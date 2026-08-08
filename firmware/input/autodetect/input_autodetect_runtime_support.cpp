#include "../../product_config.h"
#include "../../firmware_platform_config.h"

#include <Arduino.h>

#include "../../menu/menu_bridge.h"
#include "input_autodetect_runtime_internal.h"

#ifdef ENABLE_INPUT_AUTODETECT
bool inputAutoDetectAssistCandidatePending(const PassiveAssistScanResult& scan,
                                           bool reset_idle_timer) {
  if (scan.result != AUTODETECT_NONE || !scan.assist_candidate_pending) {
    return false;
  }

  if (reset_idle_timer) {
    resetIdleTimer();
  }
  return true;
}

void logInputAutodetectDebug(const char* phase, bool is_hotswap, uint8_t detected_port,
                             DeviceEnum detectedMode) {
  #if defined(AUTODETECT_DEBUG) && defined(AUTODETECT_DEBUG_CDC)
  Serial.print(F("[AD] "));
  Serial.print(phase);
  Serial.print(F(" hs="));
  Serial.print(is_hotswap ? 1 : 0);
  Serial.print(F(" res="));
  Serial.print(autoDetectResultName(autoDetectResult));
  Serial.print(F(" mode="));
  Serial.print((uint8_t)detectedMode);
  Serial.print(F(" port="));
  if (detected_port == 0xFF) {
    Serial.print(F("-"));
  } else {
    Serial.print((unsigned)(detected_port + 1));
  }
  Serial.print(F(" t="));
  Serial.print(input_autodetect_last_ms);
  Serial.println(F("ms"));

  char dbgLine[768];
  getAutoDetectSerialDebugLine(0, dbgLine, sizeof(dbgLine));
  Serial.print(F("[AD] "));
  Serial.println(dbgLine);
  if (getAutoDetectJaguarSerialDebugLine(0, dbgLine, sizeof(dbgLine))) {
    Serial.print(F("[AD] "));
    Serial.println(dbgLine);
  }
  getAutoDetectSerialDebugLine(1, dbgLine, sizeof(dbgLine));
  Serial.print(F("[AD] "));
  Serial.println(dbgLine);
  if (getAutoDetectJaguarSerialDebugLine(1, dbgLine, sizeof(dbgLine))) {
    Serial.print(F("[AD] "));
    Serial.println(dbgLine);
  }
  #else
  (void)phase;
  (void)is_hotswap;
  (void)detected_port;
  (void)detectedMode;
  #endif
}
#endif
