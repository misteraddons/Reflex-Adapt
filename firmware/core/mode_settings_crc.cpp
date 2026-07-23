#include "mode_settings_crc.h"

static uint16_t crc16(const uint8_t* data, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; j++) {
      crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : (crc >> 1);
    }
  }
  return crc;
}

uint16_t calculateStoredModeSettingsCrc(const crc_data_t& data) {
  return crc16(reinterpret_cast<const uint8_t*>(&data), sizeof(crc_data_t));
}

uint16_t calculateModeSettingsCrc(
    DeviceEnum inputMode,
    outputMode_t outputModeValue,
    uint8_t socdValue,
    uint8_t deadzoneValue,
    uint8_t dpadModeValue,
    uint8_t nsoSpecialValue
) {
  crc_data_t crc_data;
  crc_data.devicemode = (uint8_t)inputMode;
  crc_data.outputmode = (uint8_t)outputModeValue;
  crc_data.socd = socdValue;
  crc_data.deadzone = deadzoneValue;
  crc_data.dpad_as_buttons = dpadModeValue;
  crc_data.nso_special = nsoSpecialValue;
  return calculateStoredModeSettingsCrc(crc_data);
}
