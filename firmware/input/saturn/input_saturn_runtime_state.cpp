#include "input_saturn_runtime_state.h"

SatLibConfig_t satlibconfig = SatLibConfig_default;
uint8_t saturn_debug_mouse_slot = 0xFF;
uint8_t saturn_debug_mouse_flags = 0;
uint8_t saturn_debug_mouse_raw_x = 0;
uint8_t saturn_debug_mouse_raw_y = 0;
int8_t saturn_debug_mouse_x = 0;
int8_t saturn_debug_mouse_y = 0;
int8_t saturn_debug_mouse_x_min = 0;
int8_t saturn_debug_mouse_x_max = 0;
int8_t saturn_debug_mouse_y_min = 0;
int8_t saturn_debug_mouse_y_max = 0;
uint8_t saturn_debug_mouse_raw_x_min = 0;
uint8_t saturn_debug_mouse_raw_x_max = 0;
uint8_t saturn_debug_mouse_raw_y_min = 0;
uint8_t saturn_debug_mouse_raw_y_max = 0;
uint8_t saturn_debug_mouse_buttons = 0;
bool saturn_debug_mouse_overflow = false;
uint8_t saturn_debug_raw_slot = 0xFF;
uint8_t saturn_debug_raw_type = 0;
uint8_t saturn_debug_raw_size = 0;
uint8_t saturn_debug_raw_count = 0;
uint8_t saturn_debug_raw_x = 0x80;
uint8_t saturn_debug_raw_y = 0x80;
uint8_t saturn_debug_raw_l = 0;
uint8_t saturn_debug_raw_r = 0;
uint8_t saturn_debug_raw_x_min = 0x80;
uint8_t saturn_debug_raw_x_max = 0x80;
uint8_t saturn_debug_raw_nibbles[20] = {};
uint8_t saturn_debug_map_slot = 0xFF;
int16_t saturn_debug_map_lx = 0;
int16_t saturn_debug_map_lx_min = 0;
int16_t saturn_debug_map_lx_max = 0;
uint8_t saturn_debug_map_paddle = 0x80;
uint8_t saturn_debug_map_flags = 0;

namespace {

SaturnInputDebugSnapshot saturn_debug_snapshot = {};

}  // namespace

void input_saturn_debug_update_port(uint8_t port, uint8_t tap_ports,
                                    uint8_t last_tap_ports,
                                    uint8_t controller_count,
                                    uint8_t reserved_slots) {
  if (port >= MAX_USB_OUT) {
    return;
  }
  SaturnInputDebugPort& debug = saturn_debug_snapshot.ports[port];
  debug.tap_ports = tap_ports;
  debug.last_tap_ports = last_tap_ports;
  debug.controller_count = controller_count;
  debug.reserved_slots = reserved_slots;
  if (saturn_debug_snapshot.port_count <= port) {
    saturn_debug_snapshot.port_count = port + 1;
  }
}

void input_saturn_debug_set_total_reserved_slots(uint8_t total_reserved_slots) {
  saturn_debug_snapshot.total_reserved_slots = total_reserved_slots;
}

void input_saturn_debug_update_slot(uint8_t slot, uint8_t stable_type,
                                    uint8_t observed_type,
                                    uint8_t megadrive_stable_type,
                                    uint32_t megadrive_downgrade_ms,
                                    uint8_t source_port,
                                    uint8_t source_index,
                                    uint8_t tap_ports) {
  if (slot >= MAX_USB_OUT) {
    return;
  }
  SaturnInputDebugSlot& debug = saturn_debug_snapshot.slots[slot];
  debug.stable_type = stable_type;
  debug.observed_type = observed_type;
  debug.megadrive_stable_type = megadrive_stable_type;
  debug.megadrive_downgrade_ms = megadrive_downgrade_ms;
  debug.source_port = source_port;
  debug.source_index = source_index;
  debug.tap_ports = tap_ports;
  if (saturn_debug_snapshot.slot_count <= slot) {
    saturn_debug_snapshot.slot_count = slot + 1;
  }
}

void input_saturn_debug_clear_slot(uint8_t slot) {
  if (slot >= MAX_USB_OUT) {
    return;
  }
  saturn_debug_snapshot.slots[slot] = {};
  if (slot + 1 == saturn_debug_snapshot.slot_count) {
    while (saturn_debug_snapshot.slot_count > 0 &&
           saturn_debug_snapshot.slots[saturn_debug_snapshot.slot_count - 1].stable_type == 0 &&
           saturn_debug_snapshot.slots[saturn_debug_snapshot.slot_count - 1].observed_type == 0 &&
           saturn_debug_snapshot.slots[saturn_debug_snapshot.slot_count - 1].megadrive_stable_type == 0) {
      --saturn_debug_snapshot.slot_count;
    }
  }
}

void updateSaturnRawDebugRange(uint8_t slot, const SaturnControllerState& state) {
  saturn_debug_raw_slot = slot;
  saturn_debug_raw_type = state.debugControllerType;
  saturn_debug_raw_size = state.debugDataSize;
  saturn_debug_raw_count = min((uint8_t)sizeof(saturn_debug_raw_nibbles), state.debugNibbleCount);
  saturn_debug_raw_x = state.analogX;
  saturn_debug_raw_y = state.analogY;
  saturn_debug_raw_l = state.analogL;
  saturn_debug_raw_r = state.analogR;
  if (state.analogX < saturn_debug_raw_x_min) saturn_debug_raw_x_min = state.analogX;
  if (state.analogX > saturn_debug_raw_x_max) saturn_debug_raw_x_max = state.analogX;
  for (uint8_t i = 0; i < saturn_debug_raw_count; ++i) {
    saturn_debug_raw_nibbles[i] = state.debugNibbles[i] & 0x0F;
  }
}

void updateSaturnMapDebugRange(uint8_t slot, int16_t lx, uint8_t paddle,
                               bool analog_refresh, bool state_changed) {
  saturn_debug_map_slot = slot;
  saturn_debug_map_lx = lx;
  saturn_debug_map_paddle = paddle;
  saturn_debug_map_flags = (analog_refresh ? 0x01 : 0x00) | (state_changed ? 0x02 : 0x00);
  if (lx < saturn_debug_map_lx_min) saturn_debug_map_lx_min = lx;
  if (lx > saturn_debug_map_lx_max) saturn_debug_map_lx_max = lx;
}

void updateSaturnMouseDebugRange(uint8_t slot, const SaturnControllerState& state) {
  saturn_debug_mouse_slot = slot;
  saturn_debug_mouse_flags = state.mouseFlags;
  saturn_debug_mouse_raw_x = state.mouseRawX;
  saturn_debug_mouse_raw_y = state.mouseRawY;
  saturn_debug_mouse_x = state.mouseX;
  saturn_debug_mouse_y = state.mouseY;
  if (state.mouseX < saturn_debug_mouse_x_min) saturn_debug_mouse_x_min = state.mouseX;
  if (state.mouseX > saturn_debug_mouse_x_max) saturn_debug_mouse_x_max = state.mouseX;
  if (state.mouseY < saturn_debug_mouse_y_min) saturn_debug_mouse_y_min = state.mouseY;
  if (state.mouseY > saturn_debug_mouse_y_max) saturn_debug_mouse_y_max = state.mouseY;
  if (state.mouseRawX < saturn_debug_mouse_raw_x_min) saturn_debug_mouse_raw_x_min = state.mouseRawX;
  if (state.mouseRawX > saturn_debug_mouse_raw_x_max) saturn_debug_mouse_raw_x_max = state.mouseRawX;
  if (state.mouseRawY < saturn_debug_mouse_raw_y_min) saturn_debug_mouse_raw_y_min = state.mouseRawY;
  if (state.mouseRawY > saturn_debug_mouse_raw_y_max) saturn_debug_mouse_raw_y_max = state.mouseRawY;
  saturn_debug_mouse_buttons = state.mouseButtons;
  saturn_debug_mouse_overflow = state.mouseOverflow;
}

void resetSaturnMouseDebugRange() {
  saturn_debug_mouse_slot = 0xFF;
  saturn_debug_mouse_flags = 0;
  saturn_debug_mouse_raw_x = 0;
  saturn_debug_mouse_raw_y = 0;
  saturn_debug_mouse_x = 0;
  saturn_debug_mouse_y = 0;
  saturn_debug_mouse_x_min = 0;
  saturn_debug_mouse_x_max = 0;
  saturn_debug_mouse_y_min = 0;
  saturn_debug_mouse_y_max = 0;
  saturn_debug_mouse_raw_x_min = 0;
  saturn_debug_mouse_raw_x_max = 0;
  saturn_debug_mouse_raw_y_min = 0;
  saturn_debug_mouse_raw_y_max = 0;
  saturn_debug_mouse_buttons = 0;
  saturn_debug_mouse_overflow = false;
}

SaturnInputDebugSnapshot input_saturn_debug_snapshot() {
  return saturn_debug_snapshot;
}
