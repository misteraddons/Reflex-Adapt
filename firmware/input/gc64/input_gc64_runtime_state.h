#pragma once

#include <stdint.h>

constexpr uint8_t GC64_DEBUG_PORTS = 2;

extern uint8_t gc64_debug_device_type[GC64_DEBUG_PORTS];
extern uint8_t gc64_n64_accessory_aux[GC64_DEBUG_PORTS];
extern bool gc64_n64_rumble_pak_detected[GC64_DEBUG_PORTS];
extern bool gc64_n64_rumble_command_pending[GC64_DEBUG_PORTS];
extern uint8_t gc64_n64_rumble_probe_attempts[GC64_DEBUG_PORTS];
extern int gc64_n64_rumble_probe_result[GC64_DEBUG_PORTS];
extern uint8_t gc64_n64_rumble_probe_byte[GC64_DEBUG_PORTS];
extern int gc64_n64_rumble_motor_result[GC64_DEBUG_PORTS];
extern int gc64_n64_rumble_motor_transport[GC64_DEBUG_PORTS];
extern uint8_t gc64_n64_rumble_motor_expected[GC64_DEBUG_PORTS];
extern uint8_t gc64_n64_rumble_motor_response[GC64_DEBUG_PORTS];
