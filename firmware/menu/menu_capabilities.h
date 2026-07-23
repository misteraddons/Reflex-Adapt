#pragma once

#include "../core/device_mode.h"
#include "../output/output_mode.h"

bool menuModeHasAnalog(DeviceEnum mode);
bool menuModeHasRightStick(DeviceEnum mode);
bool menuModeCurrentControllerHasAnalog(DeviceEnum mode);
bool menuModeCurrentControllerHasAnalogStick(DeviceEnum mode);
bool menuModeCurrentControllerHasRightStick(DeviceEnum mode);
bool menuModeCurrentControllerHasAnalogTriggers(DeviceEnum mode);
bool menuModeHasRumble(DeviceEnum mode);
bool menuModeHasAnalogTriggers(DeviceEnum mode);
bool menuModeSupportsTriggerAxisMode(DeviceEnum mode);
bool menuModeSupportsTriggerBothMode(DeviceEnum mode);
bool menuModeHasIndependentDigitalTriggerButtons(DeviceEnum mode);
bool outputModeSupportsTriggerBothMode(outputMode_t mode);
bool menuModeIsNintendoController(DeviceEnum mode);
bool menuModeIsN64Controller(DeviceEnum mode);
bool shouldHideMenuOutputMode(outputMode_t mode);
