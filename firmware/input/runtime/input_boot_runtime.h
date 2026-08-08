#pragma once

// Input-owned boot/setup boundary used by the firmware entrypoint.

void restoreInputModeFromScratchRegisters();
void initializeInputModuleForBoot();
void initializeInputModuleForRuntimeModeChange();
void maybeSetupInputUsbBridge(bool outputUsbReady);
