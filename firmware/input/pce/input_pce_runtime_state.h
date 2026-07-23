#pragma once

#include <stdint.h>

#include "../../product_config.h"
#include "../../firmware_platform_config.h"

struct PceInputDebugSlot {
  uint8_t stable_type;
  uint8_t observed_type;
  uint8_t pending_type;
  uint16_t pending_count;
  uint8_t source_port;
  uint8_t source_index;
};

struct PceInputDebugSnapshot {
  uint8_t slot_count;
  PceInputDebugSlot slots[MAX_USB_OUT];
};

void input_pce_debug_update_slot(uint8_t slot, uint8_t stable_type,
                                 uint8_t observed_type, uint8_t pending_type,
                                 uint16_t pending_count, uint8_t source_port,
                                 uint8_t source_index);
void input_pce_debug_clear_slot(uint8_t slot);
PceInputDebugSnapshot input_pce_debug_snapshot();
