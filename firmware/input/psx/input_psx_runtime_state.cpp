#include "input_psx_runtime_state.h"

namespace {

PsxInputDebugSnapshot psx_debug_snapshot = {};

}  // namespace

void input_psx_debug_update_slot(uint8_t slot, bool connected, uint8_t protocol,
                                 uint16_t buttons, uint8_t special_mask,
                                 uint8_t flags) {
  if (slot >= MAX_USB_OUT) {
    return;
  }
  PsxInputDebugSlot& debug = psx_debug_snapshot.slots[slot];
  debug.connected = connected ? 1 : 0;
  debug.protocol = protocol;
  debug.special_mask = special_mask;
  debug.flags = flags;
  debug.buttons = buttons;
  if (psx_debug_snapshot.slot_count <= slot) {
    psx_debug_snapshot.slot_count = slot + 1;
  }
}

void input_psx_debug_clear_slot(uint8_t slot) {
  if (slot >= MAX_USB_OUT) {
    return;
  }
  psx_debug_snapshot.slots[slot] = {};
  if (slot + 1 == psx_debug_snapshot.slot_count) {
    while (psx_debug_snapshot.slot_count > 0 &&
           psx_debug_snapshot.slots[psx_debug_snapshot.slot_count - 1].connected == 0 &&
           psx_debug_snapshot.slots[psx_debug_snapshot.slot_count - 1].protocol == 0 &&
           psx_debug_snapshot.slots[psx_debug_snapshot.slot_count - 1].special_mask == 0) {
      --psx_debug_snapshot.slot_count;
    }
  }
}

PsxInputDebugSnapshot input_psx_debug_snapshot() {
  return psx_debug_snapshot;
}
