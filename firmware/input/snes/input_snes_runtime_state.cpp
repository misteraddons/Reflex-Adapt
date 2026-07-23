#include "../../product_config.h"

#include "input_snes_runtime_state.h"

uint8_t snes_debug_tap0 = 0;
uint8_t snes_debug_tap1 = 0;
bool snes_debug_mtap_enabled0 = false;
bool snes_debug_mtap_enabled1 = false;
bool snes_debug_fourscore0 = false;
bool snes_debug_fourscore1 = false;
uint8_t snes_port0_controller_count = 0;
uint8_t snes_port1_controller_count = 0;
uint8_t snes_debug_dtype0 = 0;
uint8_t snes_debug_dtype1 = 0;
uint8_t snes_debug_stable_dtype0 = 0;
uint8_t snes_debug_stable_dtype1 = 0;
uint8_t snes_debug_candidate_dtype0 = 0;
uint8_t snes_debug_candidate_dtype1 = 0;
uint8_t snes_debug_candidate_count0 = 0;
uint8_t snes_debug_candidate_count1 = 0;
uint16_t snes_debug_glitch_frames0 = 0;
uint16_t snes_debug_glitch_frames1 = 0;
uint8_t snes_debug_id0 = 0;
uint8_t snes_debug_id1 = 0;
uint16_t snes_debug_raw_digital0 = 0;
uint16_t snes_debug_raw_digital1 = 0;
uint16_t snes_debug_filtered_digital0 = 0;
uint16_t snes_debug_filtered_digital1 = 0;
uint16_t snes_debug_extended0 = 0;
uint16_t snes_debug_extended1 = 0;
uint32_t snes_debug_filter_drop0 = 0;
uint32_t snes_debug_filter_drop1 = 0;
uint32_t snes_debug_invalid_frame0 = 0;
uint32_t snes_debug_invalid_frame1 = 0;
uint32_t snes_debug_invalid_id0 = 0;
uint32_t snes_debug_invalid_id1 = 0;
uint32_t snes_debug_all_pressed0 = 0;
uint32_t snes_debug_all_pressed1 = 0;
bool snes_debug_reset_pending = false;
uint16_t snes_debug_mouse_ext = 0;
int8_t snes_debug_mouse_x = 0;
int8_t snes_debug_mouse_y = 0;
int8_t snes_debug_mouse_x_min = 0;
int8_t snes_debug_mouse_x_max = 0;
int8_t snes_debug_mouse_y_min = 0;
int8_t snes_debug_mouse_y_max = 0;
uint16_t snes_debug_mouse_x_min_ext = 0;
uint16_t snes_debug_mouse_x_max_ext = 0;
uint16_t snes_debug_mouse_y_min_ext = 0;
uint16_t snes_debug_mouse_y_max_ext = 0;
uint8_t snes_debug_mouse_buttons = 0;
uint8_t snes_debug_extend_count = 0;
bool snes_rumbletech_detected0 = false;
bool snes_rumbletech_detected1 = false;
bool snes_debug_safe_poll0 = false;
bool snes_debug_safe_poll1 = false;
uint8_t snes_rumble_debug_data0 = 0;
uint8_t snes_rumble_debug_data1 = 0;
uint32_t snes_rumble_debug_tx0 = 0;
uint32_t snes_rumble_debug_tx1 = 0;

namespace {

void incrementIndexedCounter(uint8_t index, uint32_t& counter0, uint32_t& counter1) {
  uint32_t* counter = nullptr;
  if (index == 0) {
    counter = &counter0;
  } else if (index == 1) {
    counter = &counter1;
  }
  if (counter != nullptr && *counter < 0xFFFFFFFFu) {
    ++(*counter);
  }
}

}  // namespace

void setSnesPadDebugRaw(uint8_t index, uint8_t id, uint16_t digital, uint16_t extended) {
  if (index == 0) {
    snes_debug_id0 = id;
    snes_debug_raw_digital0 = digital;
    snes_debug_extended0 = extended;
  } else if (index == 1) {
    snes_debug_id1 = id;
    snes_debug_raw_digital1 = digital;
    snes_debug_extended1 = extended;
  }
}

void setSnesPadDebugFiltered(uint8_t index, uint16_t digital) {
  if (index == 0) {
    snes_debug_filtered_digital0 = digital;
  } else if (index == 1) {
    snes_debug_filtered_digital1 = digital;
  }
}

void noteSnesPadFilterDrop(uint8_t index) {
  incrementIndexedCounter(index, snes_debug_filter_drop0, snes_debug_filter_drop1);
}

void noteSnesPadInvalidFrame(uint8_t index) {
  incrementIndexedCounter(index, snes_debug_invalid_frame0, snes_debug_invalid_frame1);
}

void noteSnesPadInvalidId(uint8_t index) {
  incrementIndexedCounter(index, snes_debug_invalid_id0, snes_debug_invalid_id1);
}

void noteSnesPadAllPressed(uint8_t index) {
  incrementIndexedCounter(index, snes_debug_all_pressed0, snes_debug_all_pressed1);
}

void resetSnesPadDebugCounters() {
  snes_debug_filter_drop0 = 0;
  snes_debug_filter_drop1 = 0;
  snes_debug_invalid_frame0 = 0;
  snes_debug_invalid_frame1 = 0;
  snes_debug_invalid_id0 = 0;
  snes_debug_invalid_id1 = 0;
  snes_debug_all_pressed0 = 0;
  snes_debug_all_pressed1 = 0;
  snes_debug_glitch_frames0 = 0;
  snes_debug_glitch_frames1 = 0;
  snes_rumble_debug_tx0 = 0;
  snes_rumble_debug_tx1 = 0;
  snes_debug_reset_pending = true;
}

void resetSnesMouseDebugRange() {
  snes_debug_mouse_ext = 0;
  snes_debug_mouse_x = 0;
  snes_debug_mouse_y = 0;
  snes_debug_mouse_x_min = 0;
  snes_debug_mouse_x_max = 0;
  snes_debug_mouse_y_min = 0;
  snes_debug_mouse_y_max = 0;
  snes_debug_mouse_x_min_ext = 0;
  snes_debug_mouse_x_max_ext = 0;
  snes_debug_mouse_y_min_ext = 0;
  snes_debug_mouse_y_max_ext = 0;
  snes_debug_mouse_buttons = 0;
}
