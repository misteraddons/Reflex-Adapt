#include "Input_AtariPaddle.h"

#define PADDLE_SAMPLES 2

bool RZInputPaddle::poll() {
  beginPollCycle();

  debug_time_a = readPaddleRC(input_paddle_config.pot_a);
  samples_a[sample_index] = debug_time_a;
  debug_smoothed_a = getSmoothedValue(samples_a);
  debug_time_b = readPaddleRC(input_paddle_config.pot_b);
  samples_b[sample_index] = debug_time_b;
  debug_smoothed_b = getSmoothedValue(samples_b);
  sample_index = (sample_index + 1) % PADDLE_SAMPLES;
  debug_btn_a = (digitalRead(input_paddle_config.button_a) == LOW);
  debug_btn_b = (digitalRead(input_paddle_config.button_b) == LOW);

  if (0 < MAX_USB_OUT) {
    resetState(0);
    controller_state_t& frame = inputFrame(0);
    int16_t pos_a = timeToAxis(debug_smoothed_a, cal_min_a, cal_max_a);
    frame.LX = pos_a;
    frame.LY = 0;
    frame.RX = 0;
    frame.RY = 0;
    frame.A = debug_btn_a ? 1 : 0;
    setUpdated(0);
  }

  if (1 < MAX_USB_OUT) {
    resetState(1);
    controller_state_t& frame = inputFrame(1);
    int16_t pos_b = timeToAxis(debug_smoothed_b, cal_min_b, cal_max_b);
    frame.LX = pos_b;
    frame.LY = 0;
    frame.RX = 0;
    frame.RY = 0;
    frame.A = debug_btn_b ? 1 : 0;
    setUpdated(1);
  }

  uint8_t raw[16] = {0};
  raw[0] = 1;
  raw[1] = (debug_time_a >> 16) & 0xFF;
  raw[2] = (debug_time_a >> 8) & 0xFF;
  raw[3] = debug_time_a & 0xFF;
  raw[4] = (debug_time_b >> 16) & 0xFF;
  raw[5] = (debug_time_b >> 8) & 0xFF;
  raw[6] = debug_time_b & 0xFF;
  raw[7] = debug_btn_a ? 0x01 : 0x00;
  raw[8] = debug_btn_b ? 0x01 : 0x00;
  raw[9] = sample_index;
  webhid_store_raw_data(raw, 16);

  return endPollCycle();
}
