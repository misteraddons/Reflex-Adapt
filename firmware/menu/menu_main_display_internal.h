#pragma once

#include <stdint.h>

namespace menu_main_display_internal {

bool didPrimaryControllerNameChange();
bool shouldShowAutoDetectScanning(uint8_t connectedCount);
void renderAutoDetectScanningStatus();
void renderConnectedPortNames();
void renderJvsRawDebugLine(bool force = false);
void renderXinputMultiDiagOverlay(bool force = false);
bool shouldShowXinputMultiDiagOverlay();
void renderModeButtonIndicator(bool force = false);
void updateRealtimeButtons();

}  // namespace menu_main_display_internal
