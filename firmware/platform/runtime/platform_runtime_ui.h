#pragma once

// Internal boundary for platform-owned UI/runtime work that wraps the main
// controller loop's button/menu gate plus post-poll feedback side effects.

bool runPlatformPrePollUi();
void runPlatformRuntimeControllerUi(bool updated);
void runPlatformRuntimeFeedbackUi(bool updated);
