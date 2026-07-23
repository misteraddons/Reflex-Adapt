#pragma once

inline void map_switchpro_output(uint8_t port) {
  const controller_state_t& frame = controllerFrameConst(port);
  analog_stick_precision input_precision = frame.sticks_precision_bits;
  analog_stick_precision output_precision = ANALOG_STICK_PRECISION_12;
  const uint16_t analog_mid = 0x800;
  bool virtual_boy_on_switch = false;
  bool n64_nso_on_switch = false;
#ifdef ENABLE_INPUT_VBOY
  virtual_boy_on_switch = (deviceMode == RZORD_VBOY);
#endif
#ifdef ENABLE_INPUT_N64
  n64_nso_on_switch = (deviceMode == RZORD_N64) && is_nso_special_active();
#endif

  uint8_t effective_dpad_mode = effective_dpad_mode_for_sticks(
    dpad_mode,
    frame.HAS_ANALOG_STICK_MAIN,
    frame.HAS_ANALOG_STICK_AUX);
  bool isPopnOrGuitarFreaks = frame.PAD_L && frame.PAD_R;
  bool dancePadDetected = frame.PAD_U && frame.PAD_D && !isPopnOrGuitarFreaks;
  if (dancePadDetected) {
    effective_dpad_mode = DPAD_MODE_BUTTONS;
  }

  const uint32_t faceButtons = output_apply_n64_c_buttons_to_face_buttons(
    applyNintendoPositionButtonMap(frame.digital_buttons, isPositionButtonMapActive()),
    frame);

  if (virtual_boy_on_switch) {
    switchpro[port]->switchCommon->_switchReport.a = frame.A;
    switchpro[port]->switchCommon->_switchReport.b = frame.B;
    switchpro[port]->switchCommon->_switchReport.x = 0;
    switchpro[port]->switchCommon->_switchReport.y = 0;
  } else if (n64_nso_on_switch) {
    switchpro[port]->switchCommon->_switchReport.b = (faceButtons & INPUT_A) != 0;
    switchpro[port]->switchCommon->_switchReport.a = (faceButtons & INPUT_B) != 0;
    switchpro[port]->switchCommon->_switchReport.y = (faceButtons & INPUT_X) != 0;
    switchpro[port]->switchCommon->_switchReport.x = (faceButtons & INPUT_Y) != 0;
  } else {
    switchpro[port]->switchCommon->_switchReport.b = (faceButtons & INPUT_A) != 0;
    switchpro[port]->switchCommon->_switchReport.a = (faceButtons & INPUT_B) != 0;
    switchpro[port]->switchCommon->_switchReport.y = (faceButtons & INPUT_X) != 0;
    switchpro[port]->switchCommon->_switchReport.x = (faceButtons & INPUT_Y) != 0;
  }
  switchpro[port]->switchCommon->_switchReport.l = isN64ZCombined() ? (frame.L1 || frame.L2) : frame.L1;
  switchpro[port]->switchCommon->_switchReport.r = frame.R1;

  const bool triggers_to_right_stick = (trigger_mode == TRIGGER_MODE_RSTICK) && frame.HAS_ANALOG_TRIGGERS;

  if (virtual_boy_on_switch) {
    switchpro[port]->switchCommon->_switchReport.zl = frame.START && frame.L1;
    switchpro[port]->switchCommon->_switchReport.zr = frame.START && frame.R1;
  } else if (triggers_to_right_stick) {
    switchpro[port]->switchCommon->_switchReport.zl = 0;
    switchpro[port]->switchCommon->_switchReport.zr = 0;
  } else if (frame.HAS_ANALOG_TRIGGERS) {
    switchpro[port]->switchCommon->_switchReport.zl = frame.L2 | (frame.ANALOG_L2 > 100);
    switchpro[port]->switchCommon->_switchReport.zr =
      output_n64_c_backing_r2(frame) | (frame.ANALOG_R2 > 100);
  } else {
    switchpro[port]->switchCommon->_switchReport.zl = isN64ZCombined() ? 0 : frame.L2;
    switchpro[port]->switchCommon->_switchReport.zr = output_n64_c_backing_r2(frame);
  }

  switchpro[port]->switchCommon->_switchReport.l3 = frame.L3;
  switchpro[port]->switchCommon->_switchReport.r3 = frame.R3;
  switchpro[port]->switchCommon->_switchReport.plus = frame.START;
  switchpro[port]->switchCommon->_switchReport.minus = output_n64_c_backing_select(frame);
  switchpro[port]->switchCommon->_switchReport.home = frame.HOME;
  switchpro[port]->switchCommon->_switchReport.capture = frame.CAPTURE;

  int16_t lx = convertAnalogPrecision(frame.LX, input_precision, output_precision);
  int16_t ly = convertAnalogPrecision(frame.LY, input_precision, output_precision);
  int16_t rx = convertAnalogPrecision(frame.RX, input_precision, output_precision);
  int16_t ry = convertAnalogPrecision(frame.RY, input_precision, output_precision);

  if (effective_dpad_mode == DPAD_MODE_LEFT_STICK || effective_dpad_mode == DPAD_MODE_RIGHT_STICK) {
    int16_t dpad_x = 0;
    int16_t dpad_y = 0;
    if (frame.PAD_L) dpad_x = -2047;
    if (frame.PAD_R) dpad_x = 2047;
    if (frame.PAD_U) dpad_y = -2047;
    if (frame.PAD_D) dpad_y = 2047;

    if (effective_dpad_mode == DPAD_MODE_LEFT_STICK) {
      lx = dpad_x;
      ly = dpad_y;
    } else {
      rx = dpad_x;
      ry = dpad_y;
    }

    switchpro[port]->switchCommon->_switchReport.pad_u = 0;
    switchpro[port]->switchCommon->_switchReport.pad_d = 0;
    switchpro[port]->switchCommon->_switchReport.pad_l = 0;
    switchpro[port]->switchCommon->_switchReport.pad_r = 0;
  } else {
    switchpro[port]->switchCommon->_switchReport.pad_u = frame.PAD_U;
    switchpro[port]->switchCommon->_switchReport.pad_d = frame.PAD_D;
#ifdef ENABLE_INPUT_JAGUAR
    if (jaguarRotaryActiveOnPort(port)) {
      switchpro[port]->switchCommon->_switchReport.pad_l = 0;
      switchpro[port]->switchCommon->_switchReport.pad_r = 0;
    } else
#endif
    {
      switchpro[port]->switchCommon->_switchReport.pad_l = frame.PAD_L;
      switchpro[port]->switchCommon->_switchReport.pad_r = frame.PAD_R;
    }
  }

  if (virtual_boy_on_switch) {
    if (frame.Y) rx = -2047;
    else if (frame.A) rx = 2047;
    else rx = 0;

    if (frame.X) ry = -2047;
    else if (frame.B) ry = 2047;
    else ry = 0;
  }

  if (stick_invert & 0x01) ly = -ly;
  if (stick_invert & 0x02) ry = -ry;

  switchpro[port]->switchCommon->_switchReport.lx = lx + analog_mid;
  switchpro[port]->switchCommon->_switchReport.ly = ~ly + analog_mid;
  switchpro[port]->switchCommon->_switchReport.rx = rx + analog_mid;
  switchpro[port]->switchCommon->_switchReport.ry = ~ry + analog_mid;

  if (triggers_to_right_stick) {
    const int16_t gas = (int16_t)frame.ANALOG_R2 - 128;
    const int16_t brake = (int16_t)frame.ANALOG_L2 - 128;
    const int16_t combined = gas - brake;  // -255..+255
    const int32_t scaled = ((int32_t)combined + 255) * 4095 + 255;
    switchpro[port]->switchCommon->_switchReport.ry = (uint16_t)(scaled / 510);
  }
}
