#pragma once

#include <cstring>

#include "../../core/button_map_mode.h"
#include "../../menu/menu_runtime_state.h"

// Internal HID-family mapping helpers. This stays header-only so it can reuse
// the shared controller-state accessors, output reports, and runtime flags that
// still live in out_usb.h.

inline void set_digital_from_analog(uint8_t port, uint8_t threshold = 100) {
  controller_state_t& frame = controllerFrame(port);
  if (frame.HAS_ANALOG_DPAD) {
    frame.PAD_U |= (frame.ANALOG_PAD_U > threshold);
    frame.PAD_D |= (frame.ANALOG_PAD_D > threshold);
    frame.PAD_L |= (frame.ANALOG_PAD_L > threshold);
    frame.PAD_R |= (frame.ANALOG_PAD_R > threshold);
  }
  if (frame.HAS_ANALOG_MAIN_BUTTONS) {
     // todo implement
  }
  if (frame.HAS_ANALOG_TRIGGERS) {
    frame.L2 |= (frame.ANALOG_L2 > threshold);
    frame.R2 |= (frame.ANALOG_R2 > threshold);
  }
}

// Button Map only applies to Nintendo-family layouts. Other controllers stay on name mapping.
inline bool inputModeUsesButtonMapMode() {
  return buttonMapModeAppliesToInputMode(deviceMode);
}

inline bool isPositionButtonMapActive() {
  return inputModeUsesButtonMapMode() && !is_nso_special_active() && button_map_mode == 1;
}

inline bool isPositionButtonMapActiveForGenericHidOutput() {
  return isPositionButtonMapActive();
}

inline bool input_has_independent_digital_triggers() {
  #ifdef ENABLE_INPUT_PSX
  if (deviceMode == RZORD_PSX) {
    return true;
  }
  #endif
  return false;
}

inline bool hid_output_saturn_mission_paddle_axis(const controller_state_t& frame) {
  return deviceMode == RZORD_SATURN &&
         std::strcmp(frame.controller_type_name, "Mission") == 0;
}

inline bool hid_output_dreamcast_wheel_dpad_buttons(const controller_state_t& frame) {
#ifdef ENABLE_INPUT_DREAMCAST
  return deviceMode == RZORD_DREAMCAST &&
         (std::strncmp(frame.controller_type_name, "Whl", 3) == 0 ||
          std::strcmp(frame.controller_type_name, "Wheel") == 0);
#else
  (void)frame;
  return false;
#endif
}

inline uint32_t applyPositionButtonMapToOutputButtons(uint32_t buttons) {
  return applyNintendoPositionButtonMap(buttons, isPositionButtonMapActiveForGenericHidOutput());
}

inline uint32_t mapPs3GenericHidButtons(uint32_t buttons) {
  // Runtime PS3 mode uses the clean generic HID transport. PS3 labels generic
  // buttons in the same order as the PS3 minimal descriptor: Square, Cross,
  // Circle, Triangle, L1, R1, L2, R2, Select, Start, L3, R3, PS.
  const uint32_t mappedInputs =
    INPUT_A | INPUT_B | INPUT_X | INPUT_Y |
    INPUT_L1 | INPUT_R1 | INPUT_L2 | INPUT_R2 |
    INPUT_L3 | INPUT_R3 | INPUT_START | INPUT_SELECT |
    INPUT_HOME | INPUT_CAPTURE;
  uint32_t mapped = buttons & ~mappedInputs;

  if (buttons & INPUT_X) mapped |= (1UL << 0);
  if (buttons & INPUT_A) mapped |= (1UL << 1);
  if (buttons & INPUT_B) mapped |= (1UL << 2);
  if (buttons & INPUT_Y) mapped |= (1UL << 3);
  if (buttons & INPUT_L1) mapped |= (1UL << 4);
  if (buttons & INPUT_R1) mapped |= (1UL << 5);
  if (buttons & INPUT_L2) mapped |= (1UL << 6);
  if (buttons & INPUT_R2) mapped |= (1UL << 7);
  if (buttons & INPUT_SELECT) mapped |= (1UL << 8);
  if (buttons & INPUT_START) mapped |= (1UL << 9);
  if (buttons & INPUT_L3) mapped |= (1UL << 10);
  if (buttons & INPUT_R3) mapped |= (1UL << 11);
  if (buttons & INPUT_HOME) mapped |= (1UL << 12);
  if (buttons & INPUT_CAPTURE) mapped |= (1UL << 13);

  return mapped;
}

inline void __not_in_flash_func(map_hid_output)(uint8_t port) {
  const controller_state_t& frame = controllerFrameConst(port);
  const outputMode_t effectiveOutputMode = get_effective_output_mode();
  if ((effectiveOutputMode == OUTPUT_HID || effectiveOutputMode == OUTPUT_MISTER) &&
      dpad_mode == DPAD_MODE_DPAD &&
      !frame.HAS_ANALOG_STICK_MAIN &&
      !frame.HAS_ANALOG_STICK_AUX &&
      !frame.HAS_ANALOG_TRIGGERS &&
      !frame.HAS_ANALOG_DPAD &&
      !frame.HAS_ANALOG_MAIN_BUTTONS) {
    _hidgp.buttons = applyPositionButtonMapToOutputButtons(
      applyZButtonOutputMode(frame.digital_buttons, frame));
    _hidgp.hat = directionsToHat(port);
    _hidgp.x = 0x80;
    _hidgp.y = 0x80;
    _hidgp.z = 0x80;
    _hidgp.rx = 0x80;
    _hidgp.ry = 0x80;
    _hidgp.rz = 0x80;
    debug_hid_x[port] = _hidgp.x;
    debug_hid_y[port] = _hidgp.y;
    debug_hid_z[port] = _hidgp.z;
    debug_hid_rx[port] = _hidgp.rx;
    return;
  }

  const analog_stick_precision input_precision = frame.sticks_precision_bits;
  const analog_stick_precision output_precision = ANALOG_STICK_PRECISION_8;
  const uint8_t analog_mid = 0x80;
  bool has_analog_triggers = frame.HAS_ANALOG_TRIGGERS;
#ifdef ENABLE_INPUT_DREAMCAST
  if (deviceMode == RZORD_DREAMCAST) {
    has_analog_triggers = true;
  }
#endif
  const bool analog_trigger_mode =
    has_analog_triggers &&
    (trigger_mode == TRIGGER_MODE_ANALOG ||
     trigger_mode == TRIGGER_MODE_RSTICK ||
     trigger_mode == TRIGGER_MODE_BOTH);

  bool isPopnOrGuitarFreaks = frame.PAD_L && frame.PAD_R;
  bool dancePadDetected = frame.PAD_U && frame.PAD_D && !isPopnOrGuitarFreaks;
  const bool dreamcastWheelDpadButtons = hid_output_dreamcast_wheel_dpad_buttons(frame);

  uint8_t effective_dpad_mode = effective_dpad_mode_for_sticks(
    dpad_mode,
    frame.HAS_ANALOG_STICK_MAIN,
    frame.HAS_ANALOG_STICK_AUX);
  if (dancePadDetected || dreamcastWheelDpadButtons) {
    effective_dpad_mode = DPAD_MODE_BUTTONS;
  }

  uint32_t outputButtons = applyPositionButtonMapToOutputButtons(
    applyZButtonOutputMode(frame.digital_buttons, frame));
  outputButtons = output_apply_n64_c_buttons_to_face_buttons(outputButtons, frame);
  if (has_analog_triggers &&
      (trigger_mode == TRIGGER_MODE_DIGITAL || trigger_mode == TRIGGER_MODE_BOTH)) {
    if (frame.ANALOG_L2 > TRIGGER_DIGITAL_THRESHOLD) outputButtons |= INPUT_L2;
    if (frame.ANALOG_R2 > TRIGGER_DIGITAL_THRESHOLD) outputButtons |= INPUT_R2;
  }

  if (analog_trigger_mode &&
      trigger_mode == TRIGGER_MODE_ANALOG &&
      !input_has_independent_digital_triggers()) {
    outputButtons &= ~(INPUT_L2 | INPUT_R2);
  }

  uint32_t hidButtons = outputButtons;
  if (effective_dpad_mode == DPAD_MODE_BUTTONS) {
    _hidgp.hat = 0x8;
  } else {
    _hidgp.hat = directionsToHat(port);
    hidButtons &= ~(INPUT_PAD_U | INPUT_PAD_D | INPUT_PAD_L | INPUT_PAD_R);
  }
  if (effectiveOutputMode == OUTPUT_PS3) {
    hidButtons = mapPs3GenericHidButtons(hidButtons);
  }
  _hidgp.buttons = hidButtons;

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
    _hidgp.hat = 0x8;
  }

  if (stick_invert & 0x01) ly = -ly;
  if (stick_invert & 0x02) ry = -ry;

  _hidgp.x = lx + analog_mid;
  _hidgp.y = ly + analog_mid;
  _hidgp.z = rx + analog_mid;
  _hidgp.rx = ry + analog_mid;
  debug_hid_x[port] = _hidgp.x;
  debug_hid_y[port] = _hidgp.y;
  debug_hid_z[port] = _hidgp.z;
  debug_hid_rx[port] = _hidgp.rx;

  _hidgp.ry = analog_mid;
  _hidgp.rz = analog_mid;
  if (effectiveOutputMode == OUTPUT_HID || effectiveOutputMode == OUTPUT_MISTER) {
    if (analog_trigger_mode) {
      _hidgp.ry = frame.ANALOG_L2;
      _hidgp.rz = frame.ANALOG_R2;
    } else if (hid_output_saturn_mission_paddle_axis(frame)) {
      _hidgp.rz = frame.paddle;
    }
  }
}

inline void map_hidjogconmister_output(uint8_t port) {
  const controller_state_t& frame = controllerFrameConst(port);
  const analog_stick_precision input_precision = frame.sticks_precision_bits;
  const analog_stick_precision output_precision = ANALOG_STICK_PRECISION_8;
  const uint16_t analog_mid = 0x80;

  _jogcon.direction = directionsToHat(port);
  _jogcon.buttons = applyZButtonOutputMode(frame.digital_buttons, frame);
  if (frame.HOME) {
    _jogcon.buttons |= (uint16_t)(INPUT_HOME | INPUT_CAPTURE);
  }
  _jogcon.spinner_axis = frame.spinner;
  _jogcon.paddle_axis = convertAnalogPrecision(frame.LX, input_precision, output_precision) + analog_mid;
}

inline void map_hidgunconmister_output(uint8_t port) {
  const controller_state_t& frame = controllerFrameConst(port);

  _guncon.buttons = applyZButtonOutputMode(frame.digital_buttons, frame);
  _guncon.x = frame.LX;
  _guncon.y = frame.LY;
}

inline void map_hidnegconmister_output(uint8_t port) {
  const controller_state_t& frame = controllerFrameConst(port);
  const analog_stick_precision input_precision = frame.sticks_precision_bits;
  const analog_stick_precision output_precision = ANALOG_STICK_PRECISION_8;

  _negcon.direction = directionsToHat(port);
  _negcon.square_btn = frame.X;
  _negcon.cross_btn = frame.A;
  _negcon.circle_btn = frame.B;
  _negcon.triangle_btn = frame.Y;
  _negcon.l1_btn = isN64ZCombined() ? (frame.L1 || frame.L2) : frame.L1;
  _negcon.r1_btn = frame.R1;
  _negcon.l2_btn = isN64ZCombined() ? 0 : frame.L2;
  _negcon.r2_btn = frame.R2;
  _negcon.select_btn = frame.SELECT;
  _negcon.start_btn = frame.START;
  _negcon.l3_btn = frame.L3;
  _negcon.r3_btn = frame.R3;
  _negcon.ps_btn = frame.HOME;
  _negcon.guide2_btn = frame.HOME;
  _negcon.axis_twist = frame.paddle;
  _negcon.axis_paddle = _negcon.axis_twist;
  _negcon.axis_l = convertAnalogPrecision(frame.RX, input_precision, output_precision);
  _negcon.axis_i = frame.ANALOG_L2;
  _negcon.axis_ii = frame.ANALOG_R2;
}
