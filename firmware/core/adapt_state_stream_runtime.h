#pragma once

#include <Arduino.h>
#include <stdint.h>

class Adafruit_USBD_CDC;

// Semantic serial state stream for Adapt.html / overlay tools.
// This complements the pixel OLED mirror by sending tiny neutral-frame and
// menu-state packets at controller-friendly rates.

constexpr uint16_t ADAPT_STATE_DEFAULT_RATE_HZ = 60;
constexpr uint16_t ADAPT_STATE_MIN_RATE_HZ = 1;
constexpr uint16_t ADAPT_STATE_MAX_RATE_HZ = 60;

void adaptStateSetEnabled(bool enabled);
bool adaptStateIsEnabled();

void adaptStateSetRateHz(uint16_t rateHz);
uint16_t adaptStateRateHz();
uint32_t adaptStateFrameCount();

void adaptStateWriteStatus(Print& stream);
void adaptStateTask(Adafruit_USBD_CDC& stream);
