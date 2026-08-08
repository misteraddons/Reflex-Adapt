#include "../product_config.h"
#include "webhid_runtime.h"

#ifdef ADAPT_OUTPUT_USB_DEVICE
volatile uint16_t webhid_get_report_count = 0;
volatile uint8_t webhid_last_report_id = 0;
volatile uint8_t webhid_last_report_type = 0;
uint8_t webhid_cached_device_mode = 0;
InputHistoryEntry input_history[INPUT_HISTORY_SIZE];
uint8_t input_history_index = 0;
uint32_t input_history_count = 0;
uint32_t button_press_count[32] = {0};
uint32_t total_polls = 0;
uint32_t last_poll_time = 0;
uint16_t poll_rate_hz = 0;

void webhid_record_input(uint32_t buttons, int16_t lx, int16_t ly, int16_t rx, int16_t ry) {
  static uint32_t last_buttons = 0;

  uint32_t now = millis();
  if (last_poll_time > 0) {
    uint32_t delta = now - last_poll_time;
    if (delta > 0 && delta < 1000) {
      poll_rate_hz = 1000 / delta;
    }
  }
  last_poll_time = now;
  total_polls++;

  uint32_t new_presses = buttons & ~last_buttons;
  for (uint8_t i = 0; i < 32; i++) {
    if (new_presses & (1UL << i)) {
      button_press_count[i]++;
    }
  }
  last_buttons = buttons;

  (void)lx;
  (void)ly;
  (void)rx;
  (void)ry;

  if (input_history_count == 0 || buttons != input_history[(input_history_index + INPUT_HISTORY_SIZE - 1) % INPUT_HISTORY_SIZE].buttons) {
    input_history[input_history_index].timestamp = now;
    input_history[input_history_index].buttons = buttons;
    input_history[input_history_index].lx = lx;
    input_history[input_history_index].ly = ly;
    input_history[input_history_index].rx = rx;
    input_history[input_history_index].ry = ry;
    input_history_index = (input_history_index + 1) % INPUT_HISTORY_SIZE;
    input_history_count++;
  }
}

void webhid_clear_stats() {
  for (uint8_t i = 0; i < 32; i++) {
    button_press_count[i] = 0;
  }
  total_polls = 0;
  input_history_count = 0;
  input_history_index = 0;
}

void webhid_update_device_mode(uint8_t mode) {
  webhid_cached_device_mode = mode;
}
#endif
