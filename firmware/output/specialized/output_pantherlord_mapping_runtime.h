#pragma once

inline void map_pantherlord_output(uint8_t port) {
  const controller_state_t& frame = controllerFrameConst(port);
  analog_stick_precision input_precision = frame.sticks_precision_bits;
  analog_stick_precision output_precision = ANALOG_STICK_PRECISION_8;
  const uint16_t analog_mid = 0x80;

  _pantherlord.hat = directionsToHat(port);

  const uint32_t faceButtons = output_apply_n64_c_buttons_to_face_buttons(
    applyNintendoPositionButtonMap(frame.digital_buttons, isPositionButtonMapActive()),
    frame);
  _pantherlord.cross = (faceButtons & INPUT_A) != 0;
  _pantherlord.circle = (faceButtons & INPUT_B) != 0;
  _pantherlord.square = (faceButtons & INPUT_X) != 0;
  _pantherlord.triangle = (faceButtons & INPUT_Y) != 0;

  _pantherlord.L1 = isN64ZCombined() ? (frame.L1 || frame.L2) : frame.L1;
  _pantherlord.R1 = frame.R1;
  _pantherlord.L2 = isN64ZCombined() ? 0 : frame.L2;
  _pantherlord.R2 = output_n64_c_backing_r2(frame);
  _pantherlord.L3 = frame.L3;
  _pantherlord.R3 = frame.R3;
  _pantherlord.select = output_n64_c_backing_select(frame);
  _pantherlord.start = frame.START;

  int8_t lx = convertAnalogPrecision(frame.LX, input_precision, output_precision);
  int8_t ly = convertAnalogPrecision(frame.LY, input_precision, output_precision);
  int8_t rx = convertAnalogPrecision(frame.RX, input_precision, output_precision);
  int8_t ry = convertAnalogPrecision(frame.RY, input_precision, output_precision);

  if (stick_invert & 0x01) ly = -ly;
  if (stick_invert & 0x02) ry = -ry;

  _pantherlord.lx = lx + analog_mid;
  _pantherlord.ly = ly + analog_mid;
  _pantherlord.rx = rx + analog_mid;
  _pantherlord.ry = ry + analog_mid;
}
