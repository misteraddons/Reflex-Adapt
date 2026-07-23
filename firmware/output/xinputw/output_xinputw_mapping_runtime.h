#pragma once

inline void map_xinputw_output(uint8_t port) {
  const controller_state_t& frame = controllerFrameConst(port);
  const analog_stick_precision input_precision = frame.sticks_precision_bits;
  const analog_stick_precision output_precision = ANALOG_STICK_PRECISION_16;
  const uint32_t faceButtons = output_apply_n64_c_buttons_to_face_buttons(
    applyNintendoPositionButtonMap(frame.digital_buttons, isPositionButtonMapActive()),
    frame);
  _xinputw_report[port].a = (faceButtons & INPUT_A) != 0;
  _xinputw_report[port].b = (faceButtons & INPUT_B) != 0;
  _xinputw_report[port].x = (faceButtons & INPUT_X) != 0;
  _xinputw_report[port].y = (faceButtons & INPUT_Y) != 0;

  _xinputw_report[port].lb = isN64ZCombined() ? (frame.L1 || frame.L2) : frame.L1;
  _xinputw_report[port].rb = frame.R1;
  _xinputw_report[port].ls = frame.L3;
  _xinputw_report[port].rs = frame.R3;
  _xinputw_report[port].back = output_n64_c_backing_select(frame);
  _xinputw_report[port].start = frame.START;
  _xinputw_report[port].home = frame.HOME;

  _xinputw_report[port].dpad_up = frame.PAD_U;
  _xinputw_report[port].dpad_down = frame.PAD_D;
  _xinputw_report[port].dpad_left = frame.PAD_L;
  _xinputw_report[port].dpad_right = frame.PAD_R;

  _xinputw_report[port].lx = convertAnalogPrecision(frame.LX, input_precision, output_precision);
  _xinputw_report[port].ly = ~convertAnalogPrecision(frame.LY, input_precision, output_precision);
  _xinputw_report[port].rx = convertAnalogPrecision(frame.RX, input_precision, output_precision);
  _xinputw_report[port].ry = ~convertAnalogPrecision(frame.RY, input_precision, output_precision);

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
  if (has_analog_triggers) {
    _xinputw_report[port].lt = frame.ANALOG_L2;
    _xinputw_report[port].rt = frame.ANALOG_R2;
  } else {
    uint8_t lt_btn = isN64ZCombined() ? 0 : frame.L2;
    _xinputw_report[port].lt = lt_btn ? 0xff : 0x00;
    _xinputw_report[port].rt = output_n64_c_backing_r2(frame) ? 0xff : 0x00;
  }
}
