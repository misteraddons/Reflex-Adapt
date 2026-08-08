#pragma once

inline void init_ps5_general_report(usbout_ps5_general_report_t& report) {
  memset(&report, 0, sizeof(report));
  report.report_id = 0x01;
  report.left_stick_x = 0x80;
  report.left_stick_y = 0x80;
  report.right_stick_x = 0x80;
  report.right_stick_y = 0x80;
  report.dpad = 0x08;
  report.data_30_31_0x001a = 0x001a;
  report.touchpad_data.p1.unpressed = 1;
  report.touchpad_data.p1.set_x(P5GENERAL_TP_X_MAX / 2);
  report.touchpad_data.p1.set_y(P5GENERAL_TP_Y_MAX / 2);
  report.touchpad_data.p2.unpressed = 1;
  report.touchpad_data.p2.set_x(P5GENERAL_TP_X_MAX / 2);
  report.touchpad_data.p2.set_y(P5GENERAL_TP_Y_MAX / 2);
}

inline void map_ps5_general_output(uint8_t port) {
  const controller_state_t& frame = controllerFrameConst(port);
  analog_stick_precision input_precision = frame.sticks_precision_bits;
  analog_stick_precision output_precision = ANALOG_STICK_PRECISION_8;

  init_ps5_general_report(_ps5_general_report);
  // The auth sidecar assigns the monotonic sequence only when a report is
  // queued for signing. Keeping it stable here preserves changed-report pacing.
  _ps5_general_report.auth_seq_number = ps5_general_report_sequence;

  const uint32_t faceButtons = playstationFaceButtons(frame);
  _ps5_general_report.button_south = (faceButtons & INPUT_A) != 0;
  _ps5_general_report.button_east = (faceButtons & INPUT_B) != 0;
  _ps5_general_report.button_west = (faceButtons & INPUT_X) != 0;
  _ps5_general_report.button_north = (faceButtons & INPUT_Y) != 0;

  const uint8_t l2_btn = isN64ZCombined() ? 0 : frame.L2;
  _ps5_general_report.button_l1 = isN64ZCombined() ? (frame.L1 || frame.L2) : frame.L1;
  _ps5_general_report.button_r1 = frame.R1;
  _ps5_general_report.button_l2 = l2_btn;
  _ps5_general_report.button_r2 = output_n64_c_backing_r2(frame);
  _ps5_general_report.button_select = output_n64_c_backing_select(frame);
  _ps5_general_report.button_start = frame.START;
  _ps5_general_report.button_l3 = frame.L3;
  _ps5_general_report.button_r3 = frame.R3;
  _ps5_general_report.button_home = frame.HOME;
  _ps5_general_report.button_touchpad = frame.CAPTURE;

  int8_t lx = convertAnalogPrecision(frame.LX, input_precision, output_precision);
  int8_t ly = convertAnalogPrecision(frame.LY, input_precision, output_precision);
  int8_t rx = convertAnalogPrecision(frame.RX, input_precision, output_precision);
  int8_t ry = convertAnalogPrecision(frame.RY, input_precision, output_precision);

  const uint8_t effective_dpad_mode = effective_dpad_mode_for_sticks(
    dpad_mode,
    frame.HAS_ANALOG_STICK_MAIN,
    frame.HAS_ANALOG_STICK_AUX);

  if (effective_dpad_mode == DPAD_MODE_LEFT_STICK ||
      effective_dpad_mode == DPAD_MODE_RIGHT_STICK) {
    int8_t dpad_x = 0;
    int8_t dpad_y = 0;
    if (frame.PAD_L) dpad_x = -127;
    if (frame.PAD_R) dpad_x = 127;
    if (frame.PAD_U) dpad_y = -127;
    if (frame.PAD_D) dpad_y = 127;

    if (effective_dpad_mode == DPAD_MODE_LEFT_STICK) {
      lx = dpad_x;
      ly = dpad_y;
    } else {
      rx = dpad_x;
      ry = dpad_y;
    }
    _ps5_general_report.dpad = 0x08;
  } else {
    _ps5_general_report.dpad = directionsToHat(port);
  }

  if (stick_invert & 0x01) ly = -ly;
  if (stick_invert & 0x02) ry = -ry;

  _ps5_general_report.left_stick_x = (uint8_t)(lx + 0x80);
  _ps5_general_report.left_stick_y = (uint8_t)(ly + 0x80);
  _ps5_general_report.right_stick_x = (uint8_t)(rx + 0x80);
  _ps5_general_report.right_stick_y = (uint8_t)(ry + 0x80);

  if (frame.HAS_ANALOG_TRIGGERS) {
    _ps5_general_report.left_trigger = frame.ANALOG_L2;
    _ps5_general_report.right_trigger = frame.ANALOG_R2;
    _ps5_general_report.button_l2 |= (frame.ANALOG_L2 > 100);
    _ps5_general_report.button_r2 |= (frame.ANALOG_R2 > 100);
  } else {
    _ps5_general_report.left_trigger = _ps5_general_report.button_l2 ? 0xff : 0x00;
    _ps5_general_report.right_trigger = _ps5_general_report.button_r2 ? 0xff : 0x00;
  }
}
