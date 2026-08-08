#pragma once

#include <stdint.h>

class RZInputModule;
class RZInputPaddle;
class RZInputDreamcast;
class RZInputGC64;
class RZInputMixed;
class RZInputPSX;

bool hasCurrentInputModule();
RZInputPaddle* currentPaddleInputModule();
RZInputDreamcast* currentDreamcastInputModule();
RZInputGC64* currentGc64InputModule();
RZInputMixed* currentMixedInputModule();
RZInputPSX* currentPsxInputModule();
const char* currentInputModuleDescriptionOr(const char* fallback);
const char* currentInputModuleUsbIdOr(const char* fallback);
const char* currentInputModulePhysicalConnectionDisplayName();
uint32_t currentInputModulePollIntervalUs();
bool currentInputModuleHasPhysicalConnectionForHotSwap();
bool pollCurrentInputModule(bool* updated = nullptr);
bool pollCurrentInputModuleIfDue(uint32_t now_us, uint32_t& last_poll_at_us,
                                 bool* updated = nullptr);
void runCurrentInputModuleAfterOutputFrameSent(bool polled, bool updated);
void setupCurrentInputModule();
void setupCurrentInputModuleStage2();
void configureCurrentInputModuleBcdDeviceVersion();
