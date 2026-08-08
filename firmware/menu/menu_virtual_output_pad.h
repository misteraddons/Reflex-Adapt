#pragma once

#include "../core/device_mode.h"
#include "../output/output_mode.h"
#include "menu_pad_layouts.h"

bool shouldShowVirtualOutputPad(DeviceEnum inputMode, outputMode_t outputMode);
void getVirtualOutputPadLayout(DeviceEnum inputMode, outputMode_t outputMode,
                               const PadButton** layout, uint8_t* layoutCount);
char getVirtualOutputButtonLabel(outputMode_t outputMode, uint32_t mask);
char getVirtualOutputButtonGlyph(outputMode_t outputMode, uint32_t mask, bool pressed);
