#pragma once

// Firmware-owned Arduino entry orchestration. This keeps firmware.cpp
// as a thin wrapper while the real setup/loop composition lives in normal
// runtime modules.

void runFirmwareSetup();
void runFirmwareLoop();
