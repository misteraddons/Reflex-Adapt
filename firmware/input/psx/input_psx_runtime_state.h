#pragma once

#include <stdint.h>

#include "../../product_config.h"
#include "../../firmware_platform_config.h"

struct PsxInputDebugSlot {
  uint8_t connected;
  uint8_t protocol;
  uint8_t special_mask;
  uint8_t flags;
  uint16_t buttons;
};

struct PsxInputDebugSnapshot {
  uint8_t slot_count;
  PsxInputDebugSlot slots[MAX_USB_OUT];
};

enum PsxInputDebugFlags : uint8_t {
  PSX_DEBUG_FLAG_NEGCON = 0x01,
  PSX_DEBUG_FLAG_JOGCON = 0x02,
  PSX_DEBUG_FLAG_GUNCON = 0x04,
  PSX_DEBUG_FLAG_MOUSE = 0x08,
  PSX_DEBUG_FLAG_IR = 0x10,
  PSX_DEBUG_FLAG_DANCE = 0x20,
  PSX_DEBUG_FLAG_MULTITAP = 0x40,
};

void input_psx_debug_update_slot(uint8_t slot, bool connected, uint8_t protocol,
                                 uint16_t buttons, uint8_t special_mask,
                                 uint8_t flags);
void input_psx_debug_clear_slot(uint8_t slot);
PsxInputDebugSnapshot input_psx_debug_snapshot();
