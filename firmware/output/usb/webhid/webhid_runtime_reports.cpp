#include "../../../product_config.h"

#include <string.h>

#include "../../../core/button_remap.h"
#include "../../../core/device_runtime_state.h"
#include "../../../core/turbo.h"
#include "../../../platform/webhid_runtime.h"
#include "webhid_protocol.h"
#include "webhid_runtime_reports.h"

uint16_t webhid_get_turbo_report(uint8_t* buffer) {
  buffer[0] = WEBHID_MAGIC_BYTE;
  buffer[1] = TURBO_BTN_COUNT;

  for (uint8_t i = 0; i < TURBO_BTN_COUNT; i++) {
    buffer[2 + i] = (uint8_t)turbo.getButtonRate((TurboButton)i);
  }

  buffer[2 + TURBO_BTN_COUNT] = deviceMode;

  return WEBHID_REPORT_SIZE;
}

uint16_t webhid_get_remap_report(uint8_t* buffer) {
  buffer[0] = WEBHID_MAGIC_BYTE;
  buffer[1] = REMAP_MAX_BUTTONS;

  for (uint8_t i = 0; i < REMAP_MAX_BUTTONS; i++) {
    buffer[2 + i] = active_remaps[i];
  }

  const uint8_t modeOffset = 2 + REMAP_MAX_BUTTONS;
  buffer[modeOffset] = (uint8_t)deviceMode;

  bool has_remaps = false;
  for (uint8_t i = 0; i < REMAP_MAX_BUTTONS; i++) {
    if (active_remaps[i] != i) {
      has_remaps = true;
      break;
    }
  }
  buffer[modeOffset + 1] = has_remaps ? 1 : 0;

  return WEBHID_REPORT_SIZE;
}

uint16_t webhid_get_input_history_report(uint8_t* buffer) {
  buffer[0] = WEBHID_MAGIC_BYTE;
  uint8_t count = (input_history_count < INPUT_HISTORY_SIZE) ? input_history_count : INPUT_HISTORY_SIZE;
  buffer[1] = count;
  buffer[2] = input_history_index;

  uint8_t entries_to_send = (count < 3) ? count : 3;
  for (uint8_t i = 0; i < entries_to_send; i++) {
    uint8_t idx = (input_history_index - 1 - i + INPUT_HISTORY_SIZE) % INPUT_HISTORY_SIZE;
    uint8_t offset = 3 + (i * 16);

    InputHistoryEntry& entry = input_history[idx];
    memcpy(&buffer[offset], &entry.timestamp, 4);
    memcpy(&buffer[offset + 4], &entry.buttons, 4);
    memcpy(&buffer[offset + 8], &entry.lx, 2);
    memcpy(&buffer[offset + 10], &entry.ly, 2);
    memcpy(&buffer[offset + 12], &entry.rx, 2);
    memcpy(&buffer[offset + 14], &entry.ry, 2);
  }

  return WEBHID_REPORT_SIZE;
}

uint16_t webhid_get_stats_report(uint8_t* buffer) {
  buffer[0] = WEBHID_MAGIC_BYTE;
  buffer[1] = (poll_rate_hz >> 8) & 0xFF;
  buffer[2] = poll_rate_hz & 0xFF;

  buffer[3] = total_polls & 0xFF;
  buffer[4] = (total_polls >> 8) & 0xFF;
  buffer[5] = (total_polls >> 16) & 0xFF;
  buffer[6] = (uint8_t)deviceMode;

  for (uint8_t i = 0; i < 14; i++) {
    memcpy(&buffer[7 + (i * 4)], &button_press_count[i], 4);
  }

  return WEBHID_REPORT_SIZE;
}
