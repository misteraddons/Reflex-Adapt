#pragma once

#include <Arduino.h>

#include "../../product_config.h"
#include "../../firmware_platform_config.h"
#include <SaturnLib/SaturnLib.h>

extern SatLibConfig_t satlibconfig;

extern uint8_t saturn_debug_mouse_slot;
extern uint8_t saturn_debug_mouse_flags;
extern uint8_t saturn_debug_mouse_raw_x;
extern uint8_t saturn_debug_mouse_raw_y;
extern int8_t saturn_debug_mouse_x;
extern int8_t saturn_debug_mouse_y;
extern int8_t saturn_debug_mouse_x_min;
extern int8_t saturn_debug_mouse_x_max;
extern int8_t saturn_debug_mouse_y_min;
extern int8_t saturn_debug_mouse_y_max;
extern uint8_t saturn_debug_mouse_raw_x_min;
extern uint8_t saturn_debug_mouse_raw_x_max;
extern uint8_t saturn_debug_mouse_raw_y_min;
extern uint8_t saturn_debug_mouse_raw_y_max;
extern uint8_t saturn_debug_mouse_buttons;
extern bool saturn_debug_mouse_overflow;
extern uint8_t saturn_debug_raw_slot;
extern uint8_t saturn_debug_raw_type;
extern uint8_t saturn_debug_raw_size;
extern uint8_t saturn_debug_raw_count;
extern uint8_t saturn_debug_raw_x;
extern uint8_t saturn_debug_raw_y;
extern uint8_t saturn_debug_raw_l;
extern uint8_t saturn_debug_raw_r;
extern uint8_t saturn_debug_raw_x_min;
extern uint8_t saturn_debug_raw_x_max;
extern uint8_t saturn_debug_raw_nibbles[20];
extern uint8_t saturn_debug_map_slot;
extern int16_t saturn_debug_map_lx;
extern int16_t saturn_debug_map_lx_min;
extern int16_t saturn_debug_map_lx_max;
extern uint8_t saturn_debug_map_paddle;
extern uint8_t saturn_debug_map_flags;

struct SaturnInputDebugSlot {
  uint8_t stable_type;
  uint8_t observed_type;
  uint8_t megadrive_stable_type;
  uint32_t megadrive_downgrade_ms;
  uint8_t source_port;
  uint8_t source_index;
  uint8_t tap_ports;
};

struct SaturnInputDebugPort {
  uint8_t tap_ports;
  uint8_t last_tap_ports;
  uint8_t controller_count;
  uint8_t reserved_slots;
};

struct SaturnInputDebugSnapshot {
  uint8_t slot_count;
  uint8_t port_count;
  uint8_t total_reserved_slots;
  SaturnInputDebugSlot slots[MAX_USB_OUT];
  SaturnInputDebugPort ports[MAX_USB_OUT];
};

void input_saturn_debug_update_port(uint8_t port, uint8_t tap_ports,
                                    uint8_t last_tap_ports,
                                    uint8_t controller_count,
                                    uint8_t reserved_slots);
void input_saturn_debug_set_total_reserved_slots(uint8_t total_reserved_slots);
void input_saturn_debug_update_slot(uint8_t slot, uint8_t stable_type,
                                    uint8_t observed_type,
                                    uint8_t megadrive_stable_type,
                                    uint32_t megadrive_downgrade_ms,
                                    uint8_t source_port,
                                    uint8_t source_index,
                                    uint8_t tap_ports);
void input_saturn_debug_clear_slot(uint8_t slot);
void updateSaturnRawDebugRange(uint8_t slot, const SaturnControllerState& state);
void updateSaturnMapDebugRange(uint8_t slot, int16_t lx, uint8_t paddle,
                               bool analog_refresh, bool state_changed);
void updateSaturnMouseDebugRange(uint8_t slot, const SaturnControllerState& state);
void resetSaturnMouseDebugRange();
SaturnInputDebugSnapshot input_saturn_debug_snapshot();
