#pragma once

#include "input_autodetect_runtime.h"
#include "input_autodetect_support.h"

#ifdef ENABLE_INPUT_AUTODETECT
bool inputAutoDetectAssistCandidatePending(const PassiveAssistScanResult& scan,
                                           bool reset_idle_timer = false);
void logInputAutodetectDebug(const char* phase, bool is_hotswap, uint8_t detected_port,
                             DeviceEnum detectedMode);
bool handleConnectedAutoDetectHotSwap(bool waitingForInitialResolve);
bool handleDisconnectedAutoDetectHotSwap(bool waitingForInitialResolve);
#endif
