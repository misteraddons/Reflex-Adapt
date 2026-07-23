#pragma once

#include "../button_handler.h"

// Platform-owned menu/button runtime helpers that coordinate quick config,
// system menu entry, home-screen button actions, and menu hotkeys.

bool wakeFromVisibleScreensaver(ButtonEvent modeEvent, ButtonEvent resetEvent);
void handleMenuAndButtonUi(ButtonEvent modeEvent, ButtonEvent resetEvent);
void runPlatformMenuHotkeys();
bool queuePlatformMenuControlAction(const char* action);
