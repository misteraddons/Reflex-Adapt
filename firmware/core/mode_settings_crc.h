#pragma once

#include <stddef.h>
#include <stdint.h>

#include "device_mode.h"
#include "../output/output_mode.h"

struct crc_data_t {
  uint8_t devicemode;
  uint8_t outputmode;
  uint8_t socd;
  uint8_t deadzone;
  uint8_t dpad_as_buttons;
  uint8_t nso_special;
};

uint16_t calculateStoredModeSettingsCrc(const crc_data_t& data);
uint16_t calculateModeSettingsCrc(
  DeviceEnum inputMode,
  outputMode_t outputModeValue,
  uint8_t socdValue,
  uint8_t deadzoneValue,
  uint8_t dpadModeValue,
  uint8_t nsoSpecialValue
);
