#pragma once

#include <Arduino.h>

// Input history buffer for debugging (circular buffer)
constexpr uint8_t INPUT_HISTORY_SIZE = 32;

struct InputHistoryEntry {
  uint32_t timestamp;
  uint32_t buttons;
  int16_t lx, ly, rx, ry;
};

#ifdef ADAPT_OUTPUT_USB_DEVICE
extern volatile uint16_t webhid_get_report_count;
extern volatile uint8_t webhid_last_report_id;
extern volatile uint8_t webhid_last_report_type;
extern uint8_t webhid_cached_device_mode;
extern InputHistoryEntry input_history[INPUT_HISTORY_SIZE];
extern uint8_t input_history_index;
extern uint32_t input_history_count;
extern uint32_t button_press_count[32];
extern uint32_t total_polls;
extern uint32_t last_poll_time;
extern uint16_t poll_rate_hz;

void webhid_record_input(uint32_t buttons, int16_t lx, int16_t ly, int16_t rx, int16_t ry);
void webhid_clear_stats();
void webhid_update_device_mode(uint8_t mode);
#else
inline void webhid_record_input(uint32_t, int16_t, int16_t, int16_t, int16_t) {}
inline void webhid_clear_stats() {}
inline void webhid_update_device_mode(uint8_t) {}
#endif
