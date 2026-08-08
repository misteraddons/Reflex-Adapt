#pragma once

// Input-owned boot/setup boundary used by the firmware entrypoint.

void restoreInputModeFromScratchRegisters();
void resolveAutomaticSwitchProfileInputAtBoot(bool autoOutputResolvedBoot);
void initializeInputModuleForBoot();
void initializeInputModuleForRuntimeModeChange();
void maybeSetupInputUsbBridge(bool outputUsbReady);
