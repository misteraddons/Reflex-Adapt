#pragma once

// Input-owned mode/autodetect helpers used by the runtime loop and platform
// menu flows.

bool handlePendingAutoDetectRebootRelease();
void armAutoDetectReboot();
void cycleInputMode();
