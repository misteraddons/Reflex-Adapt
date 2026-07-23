#pragma once

inline void map_xinput_output_to_report(uint8_t port, xinput_report_t& report) {
  const controller_state_t& frame = controllerFrameConst(port);
  analog_stick_precision input_precision = frame.sticks_precision_bits;
  analog_stick_precision output_precision = ANALOG_STICK_PRECISION_16;
  const uint32_t faceButtons = output_apply_n64_c_buttons_to_face_buttons(
    applyNintendoPositionButtonMap(frame.digital_buttons, isPositionButtonMapActive()),
    frame);
  report.a = (faceButtons & INPUT_A) != 0;
  report.b = (faceButtons & INPUT_B) != 0;
  report.x = (faceButtons & INPUT_X) != 0;
  report.y = (faceButtons & INPUT_Y) != 0;

  report.lb = isN64ZCombined() ? (frame.L1 || frame.L2) : frame.L1;
  report.rb = frame.R1;
  report.ls = frame.L3;
  report.rs = frame.R3;
  report.back = output_n64_c_backing_select(frame);
  report.start = frame.START;
  report.home = frame.HOME;

  uint8_t effective_dpad_mode = effective_dpad_mode_for_sticks(
    dpad_mode,
    frame.HAS_ANALOG_STICK_MAIN,
    frame.HAS_ANALOG_STICK_AUX);
  bool isPopnOrGuitarFreaks = frame.PAD_L && frame.PAD_R;
  bool dancePadDetected = frame.PAD_U && frame.PAD_D && !isPopnOrGuitarFreaks;
  if (dancePadDetected) {
    effective_dpad_mode = DPAD_MODE_BUTTONS;
  }

  uint8_t subtype = xinput_get_subtype();
  int16_t lx;
  int16_t ly;
  int16_t rx;
  int16_t ry;

  if (subtype == XINPUT_SUBTYPE_WHEEL) {
    lx = (int16_t) frame.LX * 256;
    ly = 0;
    rx = convertAnalogPrecision(frame.RX, input_precision, output_precision);
    ry = convertAnalogPrecision(frame.RY, input_precision, output_precision);
  } else {
    lx = convertAnalogPrecision(frame.LX, input_precision, output_precision);
    ly = convertAnalogPrecision(frame.LY, input_precision, output_precision);
    rx = convertAnalogPrecision(frame.RX, input_precision, output_precision);
    ry = convertAnalogPrecision(frame.RY, input_precision, output_precision);
  }

  if (effective_dpad_mode == DPAD_MODE_LEFT_STICK || effective_dpad_mode == DPAD_MODE_RIGHT_STICK) {
    int16_t dpad_x = 0;
    int16_t dpad_y = 0;
    if (frame.PAD_L) dpad_x = -32767;
    if (frame.PAD_R) dpad_x = 32767;
    if (frame.PAD_U) dpad_y = -32767;
    if (frame.PAD_D) dpad_y = 32767;

    if (effective_dpad_mode == DPAD_MODE_LEFT_STICK) {
      lx = dpad_x;
      ly = dpad_y;
    } else {
      rx = dpad_x;
      ry = dpad_y;
    }
    report.dpad_up = 0;
    report.dpad_down = 0;
    report.dpad_left = 0;
    report.dpad_right = 0;
  } else if (effective_dpad_mode == DPAD_MODE_BUTTONS) {
    report.dpad_up = frame.PAD_U;
    report.dpad_down = frame.PAD_D;
    report.dpad_left = frame.PAD_L;
    report.dpad_right = frame.PAD_R;
  } else {
    report.dpad_up = frame.PAD_U;
    report.dpad_down = frame.PAD_D;
#ifdef ENABLE_INPUT_JAGUAR
    if (jaguarRotaryActiveOnPort(port)) {
      report.dpad_left = 0;
      report.dpad_right = 0;
    } else
#endif
    {
      report.dpad_left = frame.PAD_L;
      report.dpad_right = frame.PAD_R;
    }
  }

  if (stick_invert & 0x01) ly = -ly;
  if (stick_invert & 0x02) ry = -ry;

  report.lx = lx;
  report.ly = ~ly;
  report.rx = rx;
  report.ry = ~ry;

  bool force_digital_triggers = (subtype == XINPUT_SUBTYPE_ARCADESTICK || subtype == XINPUT_SUBTYPE_ARCADEPAD);
  bool has_analog_triggers = frame.HAS_ANALOG_TRIGGERS;
#ifdef ENABLE_INPUT_DREAMCAST
  if (deviceMode == RZORD_DREAMCAST) {
    has_analog_triggers =
      (trigger_mode == TRIGGER_MODE_ANALOG ||
       trigger_mode == TRIGGER_MODE_RSTICK ||
       trigger_mode == TRIGGER_MODE_BOTH);
  }
#endif
  if (trigger_mode == TRIGGER_MODE_DIGITAL) {
    has_analog_triggers = false;
  }

  if (has_analog_triggers && !force_digital_triggers) {
    report.lt = frame.ANALOG_L2;
    report.rt = frame.ANALOG_R2;
  } else {
    uint8_t lt_btn = isN64ZCombined() ? 0 : frame.L2;
    report.lt = lt_btn ? 0xff : 0x00;
    report.rt = output_n64_c_backing_r2(frame) ? 0xff : 0x00;
  }
}

inline void map_xinput_output(uint8_t port) {
  map_xinput_output_to_report(port, _xinput_report);
}
