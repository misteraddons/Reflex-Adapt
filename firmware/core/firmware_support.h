#pragma once

#include <Arduino.h>

#include "../platform/button_handler.h"
#include "device_mode.h"
#include "../firmware_platform_config.h"

extern BootselButtonHandler resetButton;
extern ButtonHandler modeButton;

void delayWithButtonPolling(uint32_t ms);

enum ps4_device_type_emum {
  PS4_DEVICE_TYPE_GAMEPAD = 0,
  PS4_DEVICE_TYPE_ARCADESTICK,
};

extern ps4_device_type_emum ps4_type;

void reboot(void);
void setLed(uint8_t state);
void waitWithBuzzerUpdates(uint32_t durationMs);
void webhid_set_turbo_rate(uint8_t btn_index, uint8_t rate);
