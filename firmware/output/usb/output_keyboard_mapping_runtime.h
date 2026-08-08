#pragma once

// USB keyboard output mapping for the arcade/MAME keyboard mode.

inline void clear_keyboard_report() {
  _keyboard.modifier = 0;
  _keyboard.reserved = 0;
  for (uint8_t i = 0; i < sizeof(_keyboard.keycode); ++i) {
    _keyboard.keycode[i] = 0;
  }
}

inline void add_keyboard_modifier(uint8_t modifier) {
  _keyboard.modifier |= modifier;
}

inline void add_keyboard_keycode(uint8_t keycode) {
  if (keycode == HID_KEY_NONE) {
    return;
  }

  for (uint8_t i = 0; i < sizeof(_keyboard.keycode); ++i) {
    if (_keyboard.keycode[i] == keycode) {
      return;
    }
  }

  for (uint8_t i = 0; i < sizeof(_keyboard.keycode); ++i) {
    if (_keyboard.keycode[i] == HID_KEY_NONE) {
      _keyboard.keycode[i] = keycode;
      return;
    }
  }
}

inline bool keyboard_output_digital_pressed(bool digital, uint8_t analog_value, uint8_t threshold = 100) {
  return digital || analog_value > threshold;
}

inline void apply_keyboard_output_dpad(uint8_t port, uint8_t up_key, uint8_t down_key, uint8_t left_key, uint8_t right_key) {
  const controller_state_t& frame = controllerFrameConst(port);

  const bool up = keyboard_output_digital_pressed(frame.PAD_U, frame.ANALOG_PAD_U);
  const bool down = keyboard_output_digital_pressed(frame.PAD_D, frame.ANALOG_PAD_D);
  const bool left = keyboard_output_digital_pressed(frame.PAD_L, frame.ANALOG_PAD_L);
  const bool right = keyboard_output_digital_pressed(frame.PAD_R, frame.ANALOG_PAD_R);

  if (up) {
    add_keyboard_keycode(up_key);
  }
  if (down) {
    add_keyboard_keycode(down_key);
  }
  if (left) {
    add_keyboard_keycode(left_key);
  }
  if (right) {
    add_keyboard_keycode(right_key);
  }
}

inline void apply_keyboard_output_player1(uint8_t port) {
  const controller_state_t& frame = controllerFrameConst(port);

  apply_keyboard_output_dpad(port, HID_KEY_ARROW_UP, HID_KEY_ARROW_DOWN, HID_KEY_ARROW_LEFT, HID_KEY_ARROW_RIGHT);

  // MAME default Player 1 arcade layout:
  // Button 1-8 = Left Ctrl, Left Alt, Space, Left Shift, Z, X, C, V
  // Start/Coin/Service/Test = 1, 5, 9, F2
  if (frame.X) {
    add_keyboard_modifier(KEYBOARD_MODIFIER_LEFTCTRL);
  }
  if (frame.Y) {
    add_keyboard_modifier(KEYBOARD_MODIFIER_LEFTALT);
  }
  if (frame.R1) {
    add_keyboard_keycode(HID_KEY_SPACE);
  }
  if (frame.L1) {
    add_keyboard_modifier(KEYBOARD_MODIFIER_LEFTSHIFT);
  }
  if (frame.A) {
    add_keyboard_keycode(HID_KEY_Z);
  }
  if (frame.B) {
    add_keyboard_keycode(HID_KEY_X);
  }
  if (keyboard_output_digital_pressed(frame.R2, frame.ANALOG_R2)) {
    add_keyboard_keycode(HID_KEY_C);
  }
  if (keyboard_output_digital_pressed(frame.L2, frame.ANALOG_L2)) {
    add_keyboard_keycode(HID_KEY_V);
  }

  if (frame.START) {
    add_keyboard_keycode(HID_KEY_1);
  }
  if (frame.CAPTURE) {
    add_keyboard_keycode(HID_KEY_5);
  }
  if (frame.SELECT) {
    add_keyboard_keycode(HID_KEY_9);
  }
  if (frame.HOME) {
    add_keyboard_keycode(HID_KEY_F2);
  }
}

inline void apply_keyboard_output_player2(uint8_t port) {
  const controller_state_t& frame = controllerFrameConst(port);

  apply_keyboard_output_dpad(port, HID_KEY_R, HID_KEY_F, HID_KEY_D, HID_KEY_G);

  // MAME default Player 2 arcade layout:
  // Button 1-5 = A, S, Q, W, E
  // Start/Coin = 2, 6
  //
  // Buttons 6-8 extend onto otherwise-unused arcade keys so the full Reflex
  // 2x4 JVS panel still has distinct keyboard outputs for every action.
  if (frame.X) {
    add_keyboard_keycode(HID_KEY_A);
  }
  if (frame.Y) {
    add_keyboard_keycode(HID_KEY_S);
  }
  if (frame.R1) {
    add_keyboard_keycode(HID_KEY_Q);
  }
  if (frame.L1) {
    add_keyboard_keycode(HID_KEY_W);
  }
  if (frame.A) {
    add_keyboard_keycode(HID_KEY_E);
  }
  if (frame.B) {
    add_keyboard_modifier(KEYBOARD_MODIFIER_RIGHTCTRL);
  }
  if (keyboard_output_digital_pressed(frame.R2, frame.ANALOG_R2)) {
    add_keyboard_modifier(KEYBOARD_MODIFIER_RIGHTSHIFT);
  }
  if (keyboard_output_digital_pressed(frame.L2, frame.ANALOG_L2)) {
    add_keyboard_keycode(HID_KEY_ENTER);
  }

  if (frame.START) {
    add_keyboard_keycode(HID_KEY_2);
  }
  if (frame.CAPTURE) {
    add_keyboard_keycode(HID_KEY_6);
  }
}

inline void map_keyboard_output(uint8_t port) {
  clear_keyboard_report();

  if (port >= 2) {
    return;
  }

  if (port == 0) {
    apply_keyboard_output_player1(port);
  } else {
    apply_keyboard_output_player2(port);
  }
}

inline void map_keyboard_output_combined(uint8_t portCount) {
  clear_keyboard_report();

  if (portCount > 0) {
    apply_keyboard_output_player1(0);
  }
  if (portCount > 1) {
    apply_keyboard_output_player2(1);
  }
}
