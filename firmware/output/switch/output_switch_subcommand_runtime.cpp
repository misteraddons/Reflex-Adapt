#include "out_SwitchCommon.h"

#include <string.h>

void SwitchCommon::set_bt() {
  _report[14] = 0x81;
  _report[15] = 0x01;
  _report[16] = 0x03;
}

void SwitchCommon::set_device_info() {
  _report[14] = 0x82;
  _report[15] = 0x02;
  _report[16] = 0x03;
  _report[17] = 0x48;
  _report[18] = 0x03;
  _report[19] = 0x02;
  memcpy(_report + 20, _addr, 6);
  _report[26] = 0x01;
  _report[27] = spi_read_byte(0x60, 0x1B);

  switch (switchpro_mode) {
    case SWITCHPRO_PRO:
      _report[18] = 0x03;
      _report[19] = 0x02;
      break;
    case SWITCHPRO_NES:
      _report[18] = 0x09;
      _report[19] = 0x02;
      break;
    case SWITCHPRO_SNES:
      _report[18] = 0x0B;
      _report[19] = 0x02;
      break;
    case SWITCHPRO_N64:
      _report[18] = 0x0C;
      _report[19] = 0x11;
      break;
  }
}

void SwitchCommon::set_shipment() {
  _report[14] = 0x80;
  _report[15] = 0x08;
}

void SwitchCommon::toggle_imu() {
  _imu_enabled = _switchRequestReport[11] == 0x01;
  _report[14] = 0x80;
  _report[15] = 0x40;
}

void SwitchCommon::imu_sensitivity() {
  _report[14] = 0x80;
  _report[15] = 0x41;
}

void SwitchCommon::set_imu_data() {
  if (!_imu_enabled) {
    return;
  }

  uint8_t imu_data[49] = {0x75, 0xFD, 0xFD, 0xFF, 0x09, 0x10, 0x21, 0x00, 0xD5,
                          0xFF, 0xE0, 0xFF, 0x72, 0xFD, 0xF9, 0xFF, 0x0A, 0x10,
                          0x22, 0x00, 0xD5, 0xFF, 0xE0, 0xFF, 0x76, 0xFD, 0xFC,
                          0xFF, 0x09, 0x10, 0x23, 0x00, 0xD5, 0xFF, 0xE0, 0xFF};
  memcpy(_report + 14, imu_data, sizeof(imu_data));
}

uint8_t SwitchCommon::spi_read_byte(uint8_t addr_top, uint8_t addr_bottom) {
  const uint8_t six_axis[6] { 0x50, 0xFD, 0x00, 0x00, 0xC6, 0x0F };
  const uint8_t sa_calibration[24] = {0xcc, 0x00, 0x40, 0x00, 0x91, 0x01,
                                      0x00, 0x40, 0x00, 0x40, 0x00, 0x40,
                                      0xe7, 0xff, 0x0e, 0x00, 0xdc, 0xff,
                                      0x3b, 0x34, 0x3b, 0x34, 0x3b, 0x34};
  const uint8_t params[18] = {0x0F, 0x30, 0x61, 0x96, 0x30, 0xF3,
                              0xD4, 0x14, 0x54, 0x41, 0x15, 0x54,
                              0xC7, 0x79, 0x9C, 0x33, 0x36, 0x63};

  if (addr_top == 0x60) {
    if (addr_bottom == 0x12)
      return 0x03;
    else if (addr_bottom == 0x13)
      return 0x02;
    else if (addr_bottom == 0x1B)
      return 0x02;
    else if (addr_bottom >= 0x20 && addr_bottom <= 0x37)
      return sa_calibration[addr_bottom - 0x20];
    else if (addr_bottom >= 0x3D && addr_bottom <= 0x45)
      return switch_factory_stick_calibration[0].l_calibration[addr_bottom - 0x3D];
    else if (addr_bottom >= 0x46 && addr_bottom <= 0x4E)
      return switch_factory_stick_calibration[0].r_calibration[addr_bottom - 0x46];
    else if (addr_bottom >= 0x50 && addr_bottom <= 0x5B)
      return switch_colors[addr_bottom - 0x50];
    else if (addr_bottom == 0x5C)
      return 0x01;
    else if (addr_bottom >= 0x80 && addr_bottom <= 0x85)
      return six_axis[addr_bottom - 0x80];
    else if (addr_bottom >= 0x86 && addr_bottom <= 0x97)
      return params[addr_bottom - 0x86];
    else if (addr_bottom >= 0x98 && addr_bottom <= 0xA9)
      return params[addr_bottom - 0x98];
  }
  return 0xFF;
}

void SwitchCommon::spi_read() {
  uint8_t addr_top    = _switchRequestReport[12];
  uint8_t addr_bottom = _switchRequestReport[11];
  uint8_t read_length = _switchRequestReport[15];

  _report[14] = 0x90;
  _report[15] = 0x10;
  _report[16] = addr_bottom;
  _report[17] = addr_top;
  _report[20] = read_length;

  for (uint8_t i = 0; i < read_length; ++i) {
    _report[21 + i] = spi_read_byte(addr_top, addr_bottom + i);
  }
}

void SwitchCommon::set_mode() {
  _report[14] = 0x80;
  _report[15] = 0x03;
}

void SwitchCommon::set_trigger_buttons() {
  _report[14] = 0x83;
  _report[15] = 0x04;
}

void SwitchCommon::enable_vibration() {
  _report[14] = 0x80;
  _report[15] = 0x48;
  _vibration_enabled = true;
  _vibration_idx = 0;
  _vibration_report = VIB_OPTS[_vibration_idx];
}

void SwitchCommon::set_player_lights() {
  _report[14] = 0x80;
  _report[15] = 0x30;

  uint8_t bitfield = _switchRequestReport[11];
  if (bitfield == 0x01 || bitfield == 0x10) {
    _player_number = 1;
  } else if (bitfield == 0x03 || bitfield == 0x30) {
    _player_number = 2;
  } else if (bitfield == 0x07 || bitfield == 0x70) {
    _player_number = 3;
  } else if (bitfield == 0x0F || bitfield == 0xF0) {
    _player_number = 4;
  }
}

void SwitchCommon::set_nfc_ir_state() {
  _report[14] = 0x80;
  _report[15] = 0x22;
}

void SwitchCommon::set_nfc_ir_config() {
  _report[14] = 0xA0;
  _report[15] = 0x21;
  uint8_t params[8] = {0x01, 0x00, 0xFF, 0x00, 0x08, 0x00, 0x1B, 0x01};
  memcpy(_report + 16, params, sizeof(params));
  _report[49] = 0xC8;
}
