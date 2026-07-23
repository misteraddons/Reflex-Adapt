#pragma once

inline void map_switch_output(uint8_t port) {
  const controller_state_t& frame = controllerFrameConst(port);
  analog_stick_precision input_precision = frame.sticks_precision_bits;
  analog_stick_precision output_precision = ANALOG_STICK_PRECISION_8;
  const uint16_t analog_mid = 0x80;

  _pokken.hat = directionsToHat(port);

  const uint32_t faceButtons = output_apply_n64_c_buttons_to_face_buttons(
    applyNintendoPositionButtonMap(frame.digital_buttons, isPositionButtonMapActive()),
    frame);
  _pokken.b = (faceButtons & INPUT_A) != 0;
  _pokken.a = (faceButtons & INPUT_B) != 0;
  _pokken.y = (faceButtons & INPUT_X) != 0;
  _pokken.x = (faceButtons & INPUT_Y) != 0;

  _pokken.l = isN64ZCombined() ? (frame.L1 || frame.L2) : frame.L1;
  _pokken.r = frame.R1;
  _pokken.l3 = frame.L3;
  _pokken.r3 = frame.R3;
  _pokken.select = output_n64_c_backing_select(frame);
  _pokken.start = frame.START;
  _pokken.home = frame.HOME;
  _pokken.capture = frame.CAPTURE;

  if (frame.HAS_ANALOG_TRIGGERS) {
    _pokken.zl = frame.L2 | (frame.ANALOG_L2 > 100);
    _pokken.zr = output_n64_c_backing_r2(frame) | (frame.ANALOG_R2 > 100);
  } else {
    _pokken.zl = isN64ZCombined() ? 0 : frame.L2;
    _pokken.zr = output_n64_c_backing_r2(frame);
  }

  int8_t lx = convertAnalogPrecision(frame.LX, input_precision, output_precision);
  int8_t ly = convertAnalogPrecision(frame.LY, input_precision, output_precision);
  int8_t rx = convertAnalogPrecision(frame.RX, input_precision, output_precision);
  int8_t ry = convertAnalogPrecision(frame.RY, input_precision, output_precision);

  if (stick_invert & 0x01) ly = -ly;
  if (stick_invert & 0x02) ry = -ry;

  _pokken.lx = lx + analog_mid;
  _pokken.ly = ly + analog_mid;
  _pokken.rx = rx + analog_mid;
  _pokken.ry = ry + analog_mid;
}
