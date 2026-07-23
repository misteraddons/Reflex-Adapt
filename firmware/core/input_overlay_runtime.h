#pragma once

#include <Arduino.h>
#include <stdint.h>

// Input Overlay serial bridge for external controller viewers.
// The stream is intentionally command-gated so management CDC stays usable for
// debug/recovery until a host explicitly asks for continuous packets.

constexpr uint16_t INPUT_OVERLAY_DEFAULT_RATE_HZ = 60;
constexpr uint16_t INPUT_OVERLAY_MIN_RATE_HZ = 1;
constexpr uint16_t INPUT_OVERLAY_MAX_RATE_HZ = 60;

void inputOverlaySetEnabled(bool enabled);
bool inputOverlayIsEnabled();

void inputOverlaySetRateHz(uint16_t rateHz);
uint16_t inputOverlayRateHz();
uint32_t inputOverlayFrameCount();

void inputOverlayWriteStatus(Print& stream);
void inputOverlayTask(Print& stream);
