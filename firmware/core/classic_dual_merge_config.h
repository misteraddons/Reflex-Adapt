#pragma once

#include <stdint.h>

#include "device_mode.h"

constexpr uint8_t CLASSIC_DUAL_MERGE_BUTTON_COUNT = 29;

extern uint32_t classic_dual_merge_p2_mask;

uint32_t defaultClassicDualMergeP2Mask(DeviceEnum mode);
bool classicDualMergeEnabledForMode(DeviceEnum mode);
uint32_t storedClassicDualMergeP2Mask(DeviceEnum mode);
uint32_t sanitizeClassicDualMergeP2Mask(DeviceEnum mode, uint32_t mask);
uint32_t getClassicDualMergeDisplayMask(DeviceEnum mode);
void loadClassicDualMergeConfigForMode(DeviceEnum mode);
void saveClassicDualMergeEnabledForMode(DeviceEnum mode, bool enabled);
void saveClassicDualMergeP2MaskForMode(DeviceEnum mode, uint32_t mask);
void clearClassicDualMergeConfig();
uint16_t classicDualMergeStorageRequiredEnd();

uint8_t getClassicDualMergeButtonCount(DeviceEnum mode);
uint32_t getClassicDualMergeButtonMask(DeviceEnum mode, uint8_t displayIndex);
const char* getClassicDualMergeButtonName(DeviceEnum mode, uint8_t displayIndex);
