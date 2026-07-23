#pragma once

inline void map_ps3_output(uint8_t port) {
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
  ps3_data.cross_btn = (faceButtons & INPUT_A) != 0;
  ps3_data.circle_btn = (faceButtons & INPUT_B) != 0;
  ps3_data.square_btn = (faceButtons & INPUT_X) != 0;
  ps3_data.triangle_btn = (faceButtons & INPUT_Y) != 0;
  ps3_data.l1_btn = isN64ZCombined() ? (frame.L1 || frame.L2) : frame.L1;
  ps3_data.r1_btn = frame.R1;
  ps3_data.l2_btn = isN64ZCombined() ? 0 : frame.L2;
  ps3_data.r2_btn = output_n64_c_backing_r2(frame);
  ps3_data.l3_btn = frame.L3;
  ps3_data.r3_btn = frame.R3;
  ps3_data.select_btn = output_n64_c_backing_select(frame);
  ps3_data.start_btn = frame.START;
  ps3_data.ps_btn = frame.HOME;

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

    ps3_data.pad_up = 0;
    ps3_data.pad_down = 0;
    ps3_data.pad_left = 0;
    ps3_data.pad_right = 0;
  } else {
    ps3_data.pad_up = frame.PAD_U;
    ps3_data.pad_down = frame.PAD_D;
#ifdef ENABLE_INPUT_JAGUAR
    if (jaguarRotaryActiveOnPort(port)) {
      ps3_data.pad_left = 0;
      ps3_data.pad_right = 0;
    } else
#endif
    {
      ps3_data.pad_left = frame.PAD_L;
      ps3_data.pad_right = frame.PAD_R;
    }
  }

  if (stick_invert & 0x01) ly = -ly;
  if (stick_invert & 0x02) ry = -ry;

  ps3_data.joystick_lx = lx + analog_mid;
  ps3_data.joystick_ly = ly + analog_mid;
  ps3_data.joystick_rx = rx + analog_mid;
  ps3_data.joystick_ry = ry + analog_mid;

  if (frame.HAS_ANALOG_DPAD) {
    ps3_data.up_axis = frame.ANALOG_PAD_U;
    ps3_data.right_axis = frame.ANALOG_PAD_R;
    ps3_data.down_axis = frame.ANALOG_PAD_D;
    ps3_data.left_axis = frame.ANALOG_PAD_L;
  } else {
    ps3_data.up_axis = ps3_data.pad_up ? 0xff : 0x00;
    ps3_data.right_axis = ps3_data.pad_right ? 0xff : 0x00;
    ps3_data.down_axis = ps3_data.pad_down ? 0xff : 0x00;
    ps3_data.left_axis = ps3_data.pad_left ? 0xff : 0x00;
  }

  if (frame.HAS_ANALOG_MAIN_BUTTONS) {
    ps3_data.cross_axis = frame.ANALOG_A;
    ps3_data.circle_axis = frame.ANALOG_B;
    ps3_data.square_axis = frame.ANALOG_X;
    ps3_data.triangle_axis = frame.ANALOG_Y;
    ps3_data.l1_axis = frame.ANALOG_L1;
    ps3_data.r1_axis = frame.ANALOG_R1;
  } else {
    ps3_data.cross_axis = ps3_data.cross_btn ? 0xff : 0x00;
    ps3_data.circle_axis = ps3_data.circle_btn ? 0xff : 0x00;
    ps3_data.square_axis = ps3_data.square_btn ? 0xff : 0x00;
    ps3_data.triangle_axis = ps3_data.triangle_btn ? 0xff : 0x00;
    ps3_data.l1_axis = ps3_data.l1_btn ? 0xff : 0x00;
    ps3_data.r1_axis = ps3_data.r1_btn ? 0xff : 0x00;
  }

  if (frame.HAS_ANALOG_TRIGGERS) {
    ps3_data.l2_axis = frame.ANALOG_L2;
    ps3_data.r2_axis = frame.ANALOG_R2;
    ps3_data.l2_btn = frame.L2 | (frame.ANALOG_L2 > 100);
    ps3_data.r2_btn = output_n64_c_backing_r2(frame) | (frame.ANALOG_R2 > 100);
  } else {
    ps3_data.l2_axis = frame.L2 ? 0xff : 0x00;
    ps3_data.r2_axis = output_n64_c_backing_r2(frame) ? 0xff : 0x00;
    ps3_data.l2_btn = frame.L2;
    ps3_data.r2_btn = output_n64_c_backing_r2(frame);
  }
}
