#pragma once

// Internal USB output report defaults and mutable runtime state. This stays
// header-only so the rest of the output runtime can keep sharing the existing
// report instances without widening the public surface.

constexpr const usbout_hid_generic_report_t _default_hidgp = {
  .id = 0x01,
  .x = 0x80, .y = 0x80, .z = 0x80, .rz = 0x80, .rx = 0x80, .ry = 0x80,
  .hat = 0x08
};
constexpr const usbout_jogconmister_report_t _default_jogcon = { .direction = 0x08 };
constexpr const usbout_gunconmister_report_t _default_guncon = { .x = 100, .y = 100 };
constexpr const usbout_negconmister_report_t _default_negcon = { .direction = 0x08, .axis_twist = 0x80, .axis_l = 0, .axis_i = 0, .axis_ii = 0, .axis_paddle = 0x80 };
constexpr const xinput_report_t _default_xinput_report = { .report_size = sizeof(xinput_report_t) };
constexpr const USB_XboxWheel_InReport_t _default_xpad_data = { .bLength = sizeof(USB_XboxWheel_InReport_t) };
constexpr const usbout_pokken_report_t _default_pokken = { .lx = 0x7f, .ly = 0x7f, .rx = 0x7f, .ry = 0x7f };
constexpr const usbout_ps3_report_t _default_ps3 = {
  .joystick_lx = 0x7f, .joystick_ly = 0x7f, .joystick_rx = 0x7f, .joystick_ry = 0x7f,
  .plugged = 0x02,
  .power_status = 0x05,
  .rumble_status = 0x10,
  .acceler_x = 0x02, .acceler_y = 0x02, .acceler_z = 0x02, .gyro_z = 0x02,
};
constexpr const usbout_ps3_simple_report_t _default_ps3_simple = {
  .hat = 0x08,
  .lx = 0x80, .ly = 0x80, .rx = 0x80, .ry = 0x80,
};
constexpr const usbout_ps3_minimal_report_t _default_ps3_minimal = {
  .hat = 0x08,
  .lx = 0x80, .ly = 0x80, .rx = 0x80, .ry = 0x80,
};
constexpr const usbout_ps4_report_t _default_ps4_report {
  .report_id = 0x01,
  .lx = 0x80, .ly = 0x80, .rx = 0x80, .ry = 0x80,
  .dpad = 0x08,
  .touchpadData = {0x80,0x00,0x00,0x00,0x80,0x00,0x00,0x00}
};
constexpr const usbout_ps5_general_report_t _default_ps5_general_report = {
  .report_id = 0x01,
  .left_stick_x = 0x80,
  .left_stick_y = 0x80,
  .right_stick_x = 0x80,
  .right_stick_y = 0x80,
  .dpad = 0x08,
  .data_30_31_0x001a = 0x001a,
};
constexpr const usbout_pantherlord_twinusb_report_t _default_pantherlord = {
  .report_id = 0x01,
  .ry = 0x7f, .rx = 0x7f, .lx = 0x7f, .ly = 0x7f,
  .hat = 0xf
};
constexpr const usbout_gc2wiiu_port_report_t _default_gcwiiu = {
  .powered = 0,
  .connection_type = 0,
  .a = 0, .b = 0, .x = 0, .y = 0,
  .hat_l = 0, .hat_r = 0, .hat_d = 0, .hat_u = 0,
  .start = 0, .z = 0, .r = 0, .l = 0,
  .lx = 0x80, .ly = 0x80, .rx = 0x80, .ry = 0x80,
  .lt = 0, .rt = 0
};
constexpr const usbout_mdmini_report_t _default_mdmini = {
  .report_id = 0x01,
  .lx = 0x7f, .ly = 0x7f
};
constexpr const hid_keyboard_report_t _default_keyboard = {};

usbout_hid_generic_report_t _hidgp = _default_hidgp;
usbout_jogconmister_report_t _jogcon = _default_jogcon;
usbout_gunconmister_report_t _guncon = _default_guncon;
usbout_negconmister_report_t _negcon = _default_negcon;
xinput_report_t _xinput_report = _default_xinput_report;
xinput_report_t _xinput2p_report[XINPUT_MULTI_CONTROLLERS] = { _default_xinput_report, _default_xinput_report };
constexpr const xinputw_report_t _default_xinputw_report = {};
xinputw_report_t _xinputw_report[XINPUT_WIRELESS_CONTROLLERS] = {};
USB_XboxWheel_InReport_t xpad_data = _default_xpad_data;
usbout_pokken_report_t _pokken = _default_pokken;
usbout_ps3_report_t ps3_data = _default_ps3;
usbout_ps3_simple_report_t ps3_simple_data = _default_ps3_simple;
usbout_ps3_minimal_report_t ps3_minimal_data = _default_ps3_minimal;
usbout_ps4_report_t _ps4_report = _default_ps4_report;
usbout_ps5_general_report_t _ps5_general_report = _default_ps5_general_report;
uint32_t ps5_general_report_sequence = 0;
usbout_pantherlord_twinusb_report_t _pantherlord = _default_pantherlord;
usbout_gc2wiiu_report_t _gcwiiu = { .report_id = 0x21, .port = { _default_gcwiiu, _default_gcwiiu, _default_gcwiiu, _default_gcwiiu } };
bool gcwiiu_initialized = false;
usbout_mdmini_report_t _mdmini = _default_mdmini;
hid_keyboard_report_t _keyboard = _default_keyboard;

OutputSwitchPro* switchpro[MAX_USB_OUT];

static bool new_report_out_ = false;
static uint8_t masterBdaddr[6];
static uint8_t byte_6_ef;
static uint8_t reply = 0;
