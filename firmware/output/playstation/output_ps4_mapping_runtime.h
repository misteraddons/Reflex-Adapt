#pragma once

inline void map_ps4_output(uint8_t port) {
  const controller_state_t& frame = controllerFrameConst(port);
  analog_stick_precision input_precision = frame.sticks_precision_bits;
  analog_stick_precision output_precision = ANALOG_STICK_PRECISION_8;
  const uint16_t analog_mid = 0x80;

  uint8_t effective_dpad_mode = effective_dpad_mode_for_sticks(
    dpad_mode,
    frame.HAS_ANALOG_STICK_MAIN,
    frame.HAS_ANALOG_STICK_AUX);
  bool isPopnOrGuitarFreaks = frame.PAD_L && frame.PAD_R;
  bool dancePadDetected = frame.PAD_U && frame.PAD_D && !isPopnOrGuitarFreaks;
  if (dancePadDetected) {
    effective_dpad_mode = DPAD_MODE_BUTTONS;
  }

  const uint32_t faceButtons = playstationFaceButtons(frame);
  _ps4_report.cross = (faceButtons & INPUT_A) != 0;
  _ps4_report.circle = (faceButtons & INPUT_B) != 0;
  _ps4_report.square = (faceButtons & INPUT_X) != 0;
  _ps4_report.triangle = (faceButtons & INPUT_Y) != 0;
  _ps4_report.l1 = isN64ZCombined() ? (frame.L1 || frame.L2) : frame.L1;
  _ps4_report.r1 = frame.R1;
  _ps4_report.l3 = frame.L3;
  _ps4_report.r3 = frame.R3;
  _ps4_report.share = output_n64_c_backing_select(frame);
  _ps4_report.options = frame.START;
  _ps4_report.ps = frame.HOME;

  int8_t lx = convertAnalogPrecision(frame.LX, input_precision, output_precision);
  int8_t ly = convertAnalogPrecision(frame.LY, input_precision, output_precision);
  int8_t rx = convertAnalogPrecision(frame.RX, input_precision, output_precision);
  int8_t ry = convertAnalogPrecision(frame.RY, input_precision, output_precision);

  if (effective_dpad_mode == DPAD_MODE_LEFT_STICK || effective_dpad_mode == DPAD_MODE_RIGHT_STICK) {
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

    _ps4_report.dpad = 0x8;
  } else {
    _ps4_report.dpad = directionsToHat(port);
  }

  if (stick_invert & 0x01) ly = -ly;
  if (stick_invert & 0x02) ry = -ry;

  _ps4_report.lx = lx + analog_mid;
  _ps4_report.ly = ly + analog_mid;
  _ps4_report.rx = rx + analog_mid;
  _ps4_report.ry = ry + analog_mid;

  uint8_t l2_btn = isN64ZCombined() ? 0 : frame.L2;
  if (frame.HAS_ANALOG_TRIGGERS) {
    _ps4_report.analog_l2 = frame.ANALOG_L2;
    _ps4_report.analog_r2 = frame.ANALOG_R2;
    _ps4_report.l2 = l2_btn | (frame.ANALOG_L2 > 100);
    _ps4_report.r2 = output_n64_c_backing_r2(frame) | (frame.ANALOG_R2 > 100);
  } else {
    _ps4_report.analog_l2 = l2_btn ? 0xff : 0x00;
    _ps4_report.analog_r2 = output_n64_c_backing_r2(frame) ? 0xff : 0x00;
    _ps4_report.l2 = l2_btn;
    _ps4_report.r2 = output_n64_c_backing_r2(frame);
  }
}

