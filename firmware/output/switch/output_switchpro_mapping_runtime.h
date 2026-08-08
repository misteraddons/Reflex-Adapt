#pragma once

#include <cstring>

#include "output_switch_gamecube_mapping.h"
#include "output_switch_genesis_nso_mapping.h"

inline void map_switchpro_output(uint8_t port) {
  const controller_state_t& frame = controllerFrameConst(port);
  analog_stick_precision input_precision = frame.sticks_precision_bits;
  analog_stick_precision output_precision = ANALOG_STICK_PRECISION_12;
  const uint16_t analog_mid = 0x800;
  bool virtual_boy_on_switch = false;
  bool gamecube_on_switch = false;
  bool megadrive_on_switch = false;
  bool saturn_digital_on_switch = false;
  bool nes_on_switch = false;
  bool nes_nso_on_switch = false;
  bool snes_nso_on_switch = false;
  bool genesis_nso_on_switch = false;
  bool genesis_pro_layout_on_switch = false;
  bool pce_on_switch = false;
  bool pce_six_button_on_switch = false;
  bool n64_on_switch = false;
  bool n64_nso_on_switch = false;
  bool three_do_on_switch = false;
  bool jaguar_on_switch = false;
#ifdef ENABLE_INPUT_VBOY
  virtual_boy_on_switch = (deviceMode == RZORD_VBOY);
#endif
#ifdef ENABLE_INPUT_GAMECUBE
  gamecube_on_switch = (deviceMode == RZORD_GAMECUBE);
#endif
#ifdef ENABLE_INPUT_MEGADRIVE
  megadrive_on_switch = (deviceMode == RZORD_MEGADRIVE);
  genesis_nso_on_switch =
    megadrive_on_switch && output_uses_switch_genesis_profile();
  genesis_pro_layout_on_switch = megadrive_on_switch && !genesis_nso_on_switch;
#endif
#ifdef ENABLE_INPUT_SATURN
  saturn_digital_on_switch =
    (deviceMode == RZORD_SATURN) &&
    std::strcmp(frame.controller_type_name, "Saturn") == 0;
#endif
#ifdef ENABLE_INPUT_N64
  n64_on_switch = deviceMode == RZORD_N64;
  // Match the report mapping to the USB identity that is actually enumerated.
  n64_nso_on_switch =
    n64_on_switch && output_uses_switch_n64_profile();
#endif
#ifdef ENABLE_INPUT_PCE
  pce_on_switch = deviceMode == RZORD_PCE;
  pce_six_button_on_switch =
    pce_on_switch &&
    std::strcmp(frame.controller_type_name, "6-Button") == 0;
#endif
#ifdef ENABLE_INPUT_3DO
  three_do_on_switch = deviceMode == RZORD_3DO;
#endif
#ifdef ENABLE_INPUT_JAGUAR
  jaguar_on_switch = deviceMode == RZORD_JAGUAR;
#endif
#ifdef ENABLE_INPUT_NES
  nes_on_switch = deviceMode == RZORD_NES;
  nes_nso_on_switch =
    nes_on_switch && output_uses_switch_nes_profile();
#endif
#ifdef ENABLE_INPUT_SNES
  snes_nso_on_switch =
    (deviceMode == RZORD_SNES) && output_uses_switch_snes_profile();
#endif
  const bool native_nso_face_layout =
    nes_nso_on_switch || snes_nso_on_switch ||
    genesis_nso_on_switch;

  uint8_t effective_dpad_mode = effective_dpad_mode_for_sticks(
    dpad_mode,
    frame.HAS_ANALOG_STICK_MAIN,
    frame.HAS_ANALOG_STICK_AUX);
  bool isPopnOrGuitarFreaks = frame.PAD_L && frame.PAD_R;
  bool dancePadDetected = frame.PAD_U && frame.PAD_D && !isPopnOrGuitarFreaks;
  if (dancePadDetected) {
    effective_dpad_mode = DPAD_MODE_BUTTONS;
  }

  const bool position_button_map = isPositionButtonMapActive();
  const uint32_t faceButtons = output_apply_n64_c_buttons_to_face_buttons(
    frame.digital_buttons, frame);

  if (virtual_boy_on_switch) {
    // The right D-pad occupies the modern ABXY diamond while the two named
    // buttons remain available on matching A/B aliases. Name/Position affects
    // only the right-D-pad X/Y positions.
    switchpro[port]->switchCommon->_switchReport.a = frame.A || frame.R2;
    switchpro[port]->switchCommon->_switchReport.b = frame.B || frame.L2;
    switchpro[port]->switchCommon->_switchReport.x =
      position_button_map ? frame.Y : frame.X;
    switchpro[port]->switchCommon->_switchReport.y =
      position_button_map ? frame.X : frame.Y;
  } else if (n64_nso_on_switch) {
    // Native NSO N64 uses Y/X for C-Up/C-Left.
    switchpro[port]->switchCommon->_switchReport.a = frame.A;
    switchpro[port]->switchCommon->_switchReport.b = frame.B;
    switchpro[port]->switchCommon->_switchReport.x = frame.Y;
    switchpro[port]->switchCommon->_switchReport.y = frame.X;
  } else if (n64_on_switch) {
    // Preserve the named N64 A/B controls while placing C-Right, C-Down,
    // C-Up, and C-Left on the corresponding modern face-button positions.
    switchpro[port]->switchCommon->_switchReport.a =
      (position_button_map ? frame.B : frame.A) || frame.R3;
    switchpro[port]->switchCommon->_switchReport.b =
      (position_button_map ? frame.A : frame.B) || frame.L3;
    switchpro[port]->switchCommon->_switchReport.x = frame.X;
    switchpro[port]->switchCommon->_switchReport.y = frame.Y;
  } else if (native_nso_face_layout) {
    // Native NES, SNES, and Genesis controllers report their printed labels.
    switchpro[port]->switchCommon->_switchReport.a = frame.A;
    switchpro[port]->switchCommon->_switchReport.b = frame.B;
    switchpro[port]->switchCommon->_switchReport.x = frame.X;
    switchpro[port]->switchCommon->_switchReport.y = frame.Y;
  } else if (nes_on_switch) {
    // Keep physical NES A/B names when using the standard Switch Pro identity.
    switchpro[port]->switchCommon->_switchReport.a = frame.A;
    switchpro[port]->switchCommon->_switchReport.b = frame.B;
    switchpro[port]->switchCommon->_switchReport.x = frame.X;
    switchpro[port]->switchCommon->_switchReport.y = frame.Y;
  } else if (pce_on_switch && !pce_six_button_on_switch) {
    // A two-button PCE pad is laid out like NES: I is Switch A, II is Switch B.
    switchpro[port]->switchCommon->_switchReport.a = frame.B;
    switchpro[port]->switchCommon->_switchReport.b = frame.A;
    switchpro[port]->switchCommon->_switchReport.x = 0;
    switchpro[port]->switchCommon->_switchReport.y = 0;
  } else if (three_do_on_switch) {
    // Match the physical Genesis A/B/C row in NSO Genesis software.
    switchpro[port]->switchCommon->_switchReport.a = frame.X;  // 3DO C
    switchpro[port]->switchCommon->_switchReport.b = frame.B;
    switchpro[port]->switchCommon->_switchReport.x = 0;
    switchpro[port]->switchCommon->_switchReport.y = frame.A;
  } else if (position_button_map) {
    switchpro[port]->switchCommon->_switchReport.b = (faceButtons & INPUT_A) != 0;
    switchpro[port]->switchCommon->_switchReport.a = (faceButtons & INPUT_B) != 0;
    switchpro[port]->switchCommon->_switchReport.y = (faceButtons & INPUT_X) != 0;
    switchpro[port]->switchCommon->_switchReport.x = (faceButtons & INPUT_Y) != 0;
  } else {
    switchpro[port]->switchCommon->_switchReport.a = (faceButtons & INPUT_A) != 0;
    switchpro[port]->switchCommon->_switchReport.b = (faceButtons & INPUT_B) != 0;
    switchpro[port]->switchCommon->_switchReport.x = (faceButtons & INPUT_X) != 0;
    switchpro[port]->switchCommon->_switchReport.y = (faceButtons & INPUT_Y) != 0;
  }
  switchpro[port]->switchCommon->_switchReport.l =
    genesis_nso_on_switch
      ? frame.L1
      : (n64_nso_on_switch
          ? frame.L1
          : (isN64ZCombined() ? (frame.L1 || frame.L2) : frame.L1));
  switchpro[port]->switchCommon->_switchReport.r =
    genesis_nso_on_switch
      ? frame.R1
      : frame.R1;

  const bool triggers_to_right_stick = (trigger_mode == TRIGGER_MODE_RSTICK) && frame.HAS_ANALOG_TRIGGERS;

  if (virtual_boy_on_switch) {
    switchpro[port]->switchCommon->_switchReport.zl = frame.START && frame.L1;
    switchpro[port]->switchCommon->_switchReport.zr = frame.START && frame.R1;
  } else if (n64_nso_on_switch) {
    switchpro[port]->switchCommon->_switchReport.zl = frame.L2;
    switchpro[port]->switchCommon->_switchReport.zr = frame.L3;
  } else if (genesis_nso_on_switch) {
    switchpro[port]->switchCommon->_switchReport.zl = 0;
    switchpro[port]->switchCommon->_switchReport.zr = frame.R2;  // Mode
  } else if (saturn_digital_on_switch) {
    switchpro[port]->switchCommon->_switchReport.zl = frame.L2;
    switchpro[port]->switchCommon->_switchReport.zr = frame.R2;
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

  if (gamecube_on_switch) {
    const switch_gamecube::LeftShoulderButtons leftShoulder =
        switch_gamecube::map_left_shoulder(
            frame.L1 || frame.L2,
            frame.ANALOG_L2 > TRIGGER_DIGITAL_THRESHOLD,
            triggers_to_right_stick,
            gamecube_l_switch_mode);
    switchpro[port]->switchCommon->_switchReport.l = leftShoulder.l;
    switchpro[port]->switchCommon->_switchReport.zl = leftShoulder.zl;
  }

  switchpro[port]->switchCommon->_switchReport.l3 = n64_nso_on_switch ? 0 : frame.L3;
  switchpro[port]->switchCommon->_switchReport.r3 = n64_nso_on_switch ? 0 : frame.R3;
  switchpro[port]->switchCommon->_switchReport.plus = frame.START;
  if (n64_nso_on_switch) {
    switchpro[port]->switchCommon->_switchReport.minus = frame.R3;
  } else if (nes_nso_on_switch || snes_nso_on_switch) {
    switchpro[port]->switchCommon->_switchReport.minus = frame.SELECT;
  } else if (pce_on_switch) {
    switchpro[port]->switchCommon->_switchReport.minus = frame.SELECT;
  } else if (genesis_nso_on_switch) {
    switchpro[port]->switchCommon->_switchReport.minus = 0;
  } else {
    switchpro[port]->switchCommon->_switchReport.minus =
      output_n64_c_backing_select(frame);
  }
  switchpro[port]->switchCommon->_switchReport.home =
    frame.HOME;
  switchpro[port]->switchCommon->_switchReport.capture = frame.CAPTURE;

  if (genesis_pro_layout_on_switch) {
    // Make the empirically verified Switch Pro-identity compensation the final
    // authority for every button byte. D-pad-to-stick modes may still clear
    // the D-pad below.
    switch_genesis_nso::apply_button_bits(
      frame, switchpro[port]->switchCommon->_switchReport);
  } else if (saturn_digital_on_switch) {
    // Standard Saturn pads use the same compensated six-button layout as
    // Genesis, while Saturn L/R remain ZL/ZR.
    switch_genesis_nso::apply_six_button_position_bits(
      frame, switchpro[port]->switchCommon->_switchReport);
  } else if (pce_six_button_on_switch) {
    // Match the six physical PCE positions to compensated Genesis while
    // preserving PCE Select, Run, Home, Capture, and D-pad report bits.
    switch_genesis_nso::apply_pce_six_button_position_bits(
      frame, switchpro[port]->switchCommon->_switchReport);
  } else if (jaguar_on_switch) {
    // Match Jaguar Pro's six useful controls to the physical Genesis layout.
    switch_genesis_nso::apply_jaguar_six_button_position_bits(
      frame, switchpro[port]->switchCommon->_switchReport);
  }

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
