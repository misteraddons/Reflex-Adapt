#pragma once

#include <Arduino.h>
#include <stdint.h>

class Adafruit_USBD_CDC;

// Serial OLED mirror for external screenshot/video tools.
// The stream is command-gated so normal DInput management serial remains quiet
// until a desktop viewer explicitly requests OLED frames.

constexpr uint16_t OLED_SERIAL_DEFAULT_RATE_HZ = 5;
constexpr uint16_t OLED_SERIAL_MIN_RATE_HZ = 1;
constexpr uint16_t OLED_SERIAL_MAX_RATE_HZ = 60;
constexpr uint16_t OLED_SERIAL_WIDTH = 128;
constexpr uint16_t OLED_SERIAL_HEIGHT = 64;
constexpr uint16_t OLED_SERIAL_FRAME_BYTES = (OLED_SERIAL_WIDTH * OLED_SERIAL_HEIGHT) / 8;

void oledSerialSetEnabled(bool enabled);
bool oledSerialIsEnabled();

void oledSerialSetRateHz(uint16_t rateHz);
uint16_t oledSerialRateHz();
uint32_t oledSerialFrameCount();

void oledSerialWriteStatus(Print& stream);
void oledSerialWriteFrame(Print& stream);
void oledSerialTask(Print& stream);
void oledSerialTask(Adafruit_USBD_CDC& stream);
