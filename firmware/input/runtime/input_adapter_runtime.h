#pragma once

#include <stdint.h>

class RZInputPaddle;

bool hasActiveInputAdapter();
RZInputPaddle* activePaddleInputAdapter();
const char* activeInputAdapterDescriptionOr(const char* fallback);
const char* activeInputAdapterUsbIdOr(const char* fallback);
const char* activeInputAdapterPhysicalConnectionDisplayName();
bool activeInputAdapterHasPhysicalConnectionForHotSwap();
bool pollActiveInputAdapter(bool* updated = nullptr);
bool pollActiveInputAdapterIfDue(uint32_t now_us, uint32_t& last_poll_at_us,
                                 bool* updated = nullptr);
void runActiveInputAdapterAfterOutputFrameSent(bool polled, bool updated);
void setupActiveInputAdapter();
void setupActiveInputAdapterStage2();
void configureActiveInputAdapterBcdDeviceVersion();
