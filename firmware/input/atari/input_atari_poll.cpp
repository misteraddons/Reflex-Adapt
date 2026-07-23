#include "Input_Atari.h"

bool RZInputAtari::poll() {
  beginPollCycle();

  for (uint8_t port = 0; port < input_ports && port < MAX_USB_OUT; ++port) {
    const input_atari_config_t& c = input_atari_config[port];
    bool has_changes = false;

    bool new_joystick_state[5];
    new_joystick_state[0] = (c.up_pin != 255) ? !digitalRead(c.up_pin) : false;
    new_joystick_state[1] = (c.down_pin != 255) ? !digitalRead(c.down_pin) : false;
    new_joystick_state[2] = (c.left_pin != 255) ? !digitalRead(c.left_pin) : false;
    new_joystick_state[3] = (c.right_pin != 255) ? !digitalRead(c.right_pin) : false;
    new_joystick_state[4] = (c.trigger_pin != 255) ? !digitalRead(c.trigger_pin) : false;

    uint16_t new_paddle_values[2];
    new_paddle_values[0] = (c.paddle_a_pin != 255) ? measurePaddleRC(c.paddle_a_pin) : 0;
    new_paddle_values[1] = (c.paddle_b_pin != 255) ? measurePaddleRC(c.paddle_b_pin) : 0;

    bool new_paddle_buttons[2];
    new_paddle_buttons[0] = (c.paddle_a_btn != 255) ? !digitalRead(c.paddle_a_btn) : false;
    new_paddle_buttons[1] = (c.paddle_b_btn != 255) ? !digitalRead(c.paddle_b_btn) : false;

    for (uint8_t i = 0; i < 5; ++i) {
      if (new_joystick_state[i] != old_joystick_state[port][i]) {
        has_changes = true;
        break;
      }
    }
    for (uint8_t i = 0; i < 2; ++i) {
      if (new_paddle_values[i] != old_paddle_values[port][i] ||
          new_paddle_buttons[i] != old_paddle_buttons[port][i]) {
        has_changes = true;
        break;
      }
    }

    if (has_changes) {
      resetState(port);
      controller_state_t& frame = inputFrame(port);
      frame.PAD_U = new_joystick_state[0];
      frame.PAD_D = new_joystick_state[1];
      frame.PAD_L = new_joystick_state[2];
      frame.PAD_R = new_joystick_state[3];
      frame.A = new_joystick_state[4];

      uint8_t paddle_a_pos = rcTimeToPaddleValue(new_paddle_values[0]);
      uint8_t paddle_b_pos = rcTimeToPaddleValue(new_paddle_values[1]);
      frame.LX = (int8_t)(paddle_a_pos - 128);
      frame.LY = (int8_t)(paddle_b_pos - 128);
      frame.paddle = paddle_a_pos;
      frame.B = new_paddle_buttons[0];
      frame.X = new_paddle_buttons[1];
      setInputFrameConnected(frame, true);

      for (uint8_t i = 0; i < 5; ++i)
        old_joystick_state[port][i] = new_joystick_state[i];
      for (uint8_t i = 0; i < 2; ++i) {
        old_paddle_values[port][i] = new_paddle_values[i];
        old_paddle_buttons[port][i] = new_paddle_buttons[i];
      }

      setUpdated(port);

      if (port == 0) {
        uint8_t raw[16] = {0};
        raw[0] = 1;
        raw[1] = (new_joystick_state[0] ? 0x01 : 0) |
                 (new_joystick_state[1] ? 0x02 : 0) |
                 (new_joystick_state[2] ? 0x04 : 0) |
                 (new_joystick_state[3] ? 0x08 : 0) |
                 (new_joystick_state[4] ? 0x10 : 0);
        raw[2] = new_paddle_values[0] >> 8;
        raw[3] = new_paddle_values[0] & 0xFF;
        raw[4] = new_paddle_values[1] >> 8;
        raw[5] = new_paddle_values[1] & 0xFF;
        raw[6] = (new_paddle_buttons[0] ? 0x01 : 0) |
                 (new_paddle_buttons[1] ? 0x02 : 0);
        raw[7] = port;
        webhid_store_raw_data(raw, 16);
      }
    }
  }

  return endPollCycle();
}
