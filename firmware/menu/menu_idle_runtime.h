#pragma once

#include <stdint.h>

uint32_t getScreensaverTimeoutMs();
bool isScreensaverDimMode();
void resetIdleTimer();
bool handleIdleUiActivity(bool uiActivity);
bool handleControllerIdleWakeActivity();
bool isIdleTimeoutReached();
void resetAnimationState();
