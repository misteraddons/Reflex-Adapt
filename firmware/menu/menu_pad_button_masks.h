#pragma once

#include <cstdint>

uint32_t buildButtonMaskFromDigitalButtons(uint32_t buttons);
uint32_t buildButtonMaskFromReport(uint8_t index);
uint32_t buildButtonMaskFromOutputState(uint8_t index);
