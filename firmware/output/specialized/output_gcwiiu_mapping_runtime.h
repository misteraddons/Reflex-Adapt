#pragma once

inline void map_gcwiiu_output(uint8_t gcindex, uint8_t port) {
  if (!gcwiiu_initialized) {
    _gcwiiu.port[gcindex] = _default_gcwiiu;
    return;
  }

  const controller_state_t& frame = controllerFrameConst(port);
  analog_stick_precision input_precision = frame.sticks_precision_bits;
  analog_stick_precision output_precision = ANALOG_STICK_PRECISION_8;
  const uint16_t analog_mid = 0x80;

  _gcwiiu.port[gcindex].powered = 1;
  _gcwiiu.port[gcindex].connection_type = frame.connected;

  _gcwiiu.port[gcindex].hat_u = frame.PAD_U;
  _gcwiiu.port[gcindex].hat_d = frame.PAD_D;
#ifdef ENABLE_INPUT_JAGUAR
  if (jaguarRotaryActiveOnPort(port)) {
    _gcwiiu.port[gcindex].hat_l = 0;
    _gcwiiu.port[gcindex].hat_r = 0;
  } else
#endif
  {
    _gcwiiu.port[gcindex].hat_l = frame.PAD_L;
    _gcwiiu.port[gcindex].hat_r = frame.PAD_R;
  }

  const uint32_t faceButtons = output_apply_n64_c_buttons_to_face_buttons(
    applyNintendoPositionButtonMap(frame.digital_buttons, isPositionButtonMapActive()),
    frame);
  _gcwiiu.port[gcindex].a = (faceButtons & INPUT_A) != 0;
  _gcwiiu.port[gcindex].b = (faceButtons & INPUT_B) != 0;
  _gcwiiu.port[gcindex].x = (faceButtons & INPUT_X) != 0;
  _gcwiiu.port[gcindex].y = (faceButtons & INPUT_Y) != 0;

  bool native_gc_input = false;
#ifdef ENABLE_INPUT_GAMECUBE
  native_gc_input = (deviceMode == RZORD_GAMECUBE);
#endif
  const uint8_t gc_z = native_gc_input ? frame.SELECT : frame.R1;
  const uint8_t gc_l = native_gc_input ? frame.L1 : (isN64ZCombined() ? (frame.L1 || frame.L2) : frame.L2);
  const uint8_t gc_r = native_gc_input ? frame.R1 : output_n64_c_backing_r2(frame);

  _gcwiiu.port[gcindex].z = gc_z;
  _gcwiiu.port[gcindex].l = gc_l;
  _gcwiiu.port[gcindex].r = gc_r;
  _gcwiiu.port[gcindex].start = frame.START;

  int8_t lx = convertAnalogPrecision(frame.LX, input_precision, output_precision);
  int8_t ly = convertAnalogPrecision(frame.LY, input_precision, output_precision);
  int8_t rx = convertAnalogPrecision(frame.RX, input_precision, output_precision);
  int8_t ry = convertAnalogPrecision(frame.RY, input_precision, output_precision);

  if (stick_invert & 0x01) ly = -ly;
  if (stick_invert & 0x02) ry = -ry;

  _gcwiiu.port[gcindex].lx = lx + analog_mid;
  _gcwiiu.port[gcindex].ly = ~(ly + analog_mid);
  _gcwiiu.port[gcindex].rx = rx + analog_mid;
  _gcwiiu.port[gcindex].ry = ~(ry + analog_mid);

  if (frame.HAS_ANALOG_TRIGGERS) {
    _gcwiiu.port[gcindex].lt = frame.ANALOG_L2;
    _gcwiiu.port[gcindex].rt = frame.ANALOG_R2;
    _gcwiiu.port[gcindex].l = gc_l | (frame.ANALOG_L2 > 100);
    _gcwiiu.port[gcindex].r = gc_r | (frame.ANALOG_R2 > 100);
  } else {
    _gcwiiu.port[gcindex].lt = gc_l ? 0xff : 0x00;
    _gcwiiu.port[gcindex].rt = gc_r ? 0xff : 0x00;
    _gcwiiu.port[gcindex].l = gc_l;
    _gcwiiu.port[gcindex].r = gc_r;
  }
}
