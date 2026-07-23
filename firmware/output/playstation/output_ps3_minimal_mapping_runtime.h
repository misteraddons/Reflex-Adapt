#pragma once

inline void map_ps3_minimal_output(uint8_t port) {
  const controller_state_t& frame = controllerFrameConst(port);
  analog_stick_precision input_precision = frame.sticks_precision_bits;
  analog_stick_precision output_precision = ANALOG_STICK_PRECISION_8;
  const uint16_t analog_mid = 0x80;

  const uint32_t faceButtons = playstationFaceButtons(frame);
  ps3_minimal_data.cross_btn = (faceButtons & INPUT_A) != 0;
  ps3_minimal_data.circle_btn = (faceButtons & INPUT_B) != 0;
  ps3_minimal_data.square_btn = (faceButtons & INPUT_X) != 0;
  ps3_minimal_data.triangle_btn = (faceButtons & INPUT_Y) != 0;

  ps3_minimal_data.l1_btn = isN64ZCombined() ? (frame.L1 || frame.L2) : frame.L1;
  ps3_minimal_data.r1_btn = frame.R1;
  ps3_minimal_data.l2_btn = isN64ZCombined() ? 0 : frame.L2;
  ps3_minimal_data.r2_btn = output_n64_c_backing_r2(frame);
  ps3_minimal_data.l3_btn = frame.L3;
  ps3_minimal_data.r3_btn = frame.R3;
  ps3_minimal_data.select_btn = output_n64_c_backing_select(frame);
  ps3_minimal_data.start_btn = frame.START;
  ps3_minimal_data.ps_btn = frame.HOME;

  int8_t lx = convertAnalogPrecision(frame.LX, input_precision, output_precision);
  int8_t ly = convertAnalogPrecision(frame.LY, input_precision, output_precision);
  int8_t rx = convertAnalogPrecision(frame.RX, input_precision, output_precision);
  int8_t ry = convertAnalogPrecision(frame.RY, input_precision, output_precision);

  if (stick_invert & 0x01) ly = -ly;
  if (stick_invert & 0x02) ry = -ry;

  ps3_minimal_data.lx = lx + analog_mid;
  ps3_minimal_data.ly = ly + analog_mid;
  ps3_minimal_data.rx = rx + analog_mid;
  ps3_minimal_data.ry = ry + analog_mid;

  bool up = frame.PAD_U;
  bool down = frame.PAD_D;
  bool left = frame.PAD_L;
  bool right = frame.PAD_R;

  if (up && right) ps3_minimal_data.hat = 0x01;
  else if (right && down) ps3_minimal_data.hat = 0x03;
  else if (down && left) ps3_minimal_data.hat = 0x05;
  else if (left && up) ps3_minimal_data.hat = 0x07;
  else if (up) ps3_minimal_data.hat = 0x00;
  else if (right) ps3_minimal_data.hat = 0x02;
  else if (down) ps3_minimal_data.hat = 0x04;
  else if (left) ps3_minimal_data.hat = 0x06;
  else ps3_minimal_data.hat = 0x08;
}
