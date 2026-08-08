#pragma once

inline void map_ps3_simple_output(uint8_t port) {
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
  ps3_simple_data.cross_btn = (faceButtons & INPUT_A) != 0;
  ps3_simple_data.circle_btn = (faceButtons & INPUT_B) != 0;
  ps3_simple_data.square_btn = (faceButtons & INPUT_X) != 0;
  ps3_simple_data.triangle_btn = (faceButtons & INPUT_Y) != 0;
  ps3_simple_data.l1_btn = isN64ZCombined() ? (frame.L1 || frame.L2) : frame.L1;
  ps3_simple_data.r1_btn = frame.R1;
  ps3_simple_data.l2_btn = isN64ZCombined() ? 0 : frame.L2;
  ps3_simple_data.r2_btn = output_n64_c_backing_r2(frame);
  ps3_simple_data.l3_btn = frame.L3;
  ps3_simple_data.r3_btn = frame.R3;
  ps3_simple_data.select_btn = output_n64_c_backing_select(frame);
  ps3_simple_data.start_btn = frame.START;
  ps3_simple_data.ps_btn = frame.HOME;

  int8_t lx = convertAnalogPrecision(frame.LX, input_precision, output_precision);
  int8_t ly = convertAnalogPrecision(frame.LY, input_precision, output_precision);
  int8_t rx = convertAnalogPrecision(frame.RX, input_precision, output_precision);
  int8_t ry = convertAnalogPrecision(frame.RY, input_precision, output_precision);

  bool up = frame.PAD_U;
  bool down = frame.PAD_D;
  bool left = frame.PAD_L;
  bool right = frame.PAD_R;

  if (effective_dpad_mode == DPAD_MODE_LEFT_STICK || effective_dpad_mode == DPAD_MODE_RIGHT_STICK) {
    int8_t dpad_x = 0;
    int8_t dpad_y = 0;
    if (left) dpad_x = -127;
    if (right) dpad_x = 127;
    if (up) dpad_y = -127;
    if (down) dpad_y = 127;

    if (effective_dpad_mode == DPAD_MODE_LEFT_STICK) {
      lx = dpad_x;
      ly = dpad_y;
    } else {
      rx = dpad_x;
      ry = dpad_y;
    }
    ps3_simple_data.hat = 0x08;
  } else {
    if (up && right) ps3_simple_data.hat = 0x01;
    else if (right && down) ps3_simple_data.hat = 0x03;
    else if (down && left) ps3_simple_data.hat = 0x05;
    else if (left && up) ps3_simple_data.hat = 0x07;
    else if (up) ps3_simple_data.hat = 0x00;
    else if (right) ps3_simple_data.hat = 0x02;
    else if (down) ps3_simple_data.hat = 0x04;
    else if (left) ps3_simple_data.hat = 0x06;
    else ps3_simple_data.hat = 0x08;
  }

  if (stick_invert & 0x01) ly = -ly;
  if (stick_invert & 0x02) ry = -ry;

  ps3_simple_data.lx = lx + analog_mid;
  ps3_simple_data.ly = ly + analog_mid;
  ps3_simple_data.rx = rx + analog_mid;
  ps3_simple_data.ry = ry + analog_mid;

  if (frame.HAS_ANALOG_DPAD) {
    ps3_simple_data.up_axis = frame.ANALOG_PAD_U;
    ps3_simple_data.right_axis = frame.ANALOG_PAD_R;
    ps3_simple_data.down_axis = frame.ANALOG_PAD_D;
    ps3_simple_data.left_axis = frame.ANALOG_PAD_L;
  } else {
    ps3_simple_data.up_axis = up ? 0xff : 0x00;
    ps3_simple_data.right_axis = right ? 0xff : 0x00;
    ps3_simple_data.down_axis = down ? 0xff : 0x00;
    ps3_simple_data.left_axis = left ? 0xff : 0x00;
  }

  if (frame.HAS_ANALOG_MAIN_BUTTONS) {
    ps3_simple_data.cross_axis = frame.ANALOG_A;
    ps3_simple_data.circle_axis = frame.ANALOG_B;
    ps3_simple_data.square_axis = frame.ANALOG_X;
    ps3_simple_data.triangle_axis = frame.ANALOG_Y;
    ps3_simple_data.l1_axis = frame.ANALOG_L1;
    ps3_simple_data.r1_axis = frame.ANALOG_R1;
  } else {
    ps3_simple_data.cross_axis = ps3_simple_data.cross_btn ? 0xff : 0x00;
    ps3_simple_data.circle_axis = ps3_simple_data.circle_btn ? 0xff : 0x00;
    ps3_simple_data.square_axis = ps3_simple_data.square_btn ? 0xff : 0x00;
    ps3_simple_data.triangle_axis = ps3_simple_data.triangle_btn ? 0xff : 0x00;
    ps3_simple_data.l1_axis = ps3_simple_data.l1_btn ? 0xff : 0x00;
    ps3_simple_data.r1_axis = ps3_simple_data.r1_btn ? 0xff : 0x00;
  }

  if (frame.HAS_ANALOG_TRIGGERS) {
    ps3_simple_data.l2_axis = frame.ANALOG_L2;
    ps3_simple_data.r2_axis = frame.ANALOG_R2;
    ps3_simple_data.l2_btn = frame.L2 | (frame.ANALOG_L2 > 100);
    ps3_simple_data.r2_btn = output_n64_c_backing_r2(frame) | (frame.ANALOG_R2 > 100);
  } else {
    ps3_simple_data.l2_axis = frame.L2 ? 0xff : 0x00;
    ps3_simple_data.r2_axis = output_n64_c_backing_r2(frame) ? 0xff : 0x00;
    ps3_simple_data.l2_btn = frame.L2;
    ps3_simple_data.r2_btn = output_n64_c_backing_r2(frame);
  }
}
