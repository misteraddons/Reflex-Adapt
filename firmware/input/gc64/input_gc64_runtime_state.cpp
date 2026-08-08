#include "input_gc64_runtime_state.h"

uint8_t gc64_debug_device_type[GC64_DEBUG_PORTS] = {};
uint8_t gc64_n64_accessory_aux[GC64_DEBUG_PORTS] = {};
bool gc64_n64_rumble_pak_detected[GC64_DEBUG_PORTS] = {};
bool gc64_n64_rumble_command_pending[GC64_DEBUG_PORTS] = {};
uint8_t gc64_n64_rumble_probe_attempts[GC64_DEBUG_PORTS] = {};
int gc64_n64_rumble_probe_result[GC64_DEBUG_PORTS] = {};
uint8_t gc64_n64_rumble_probe_byte[GC64_DEBUG_PORTS] = {};
int gc64_n64_rumble_motor_result[GC64_DEBUG_PORTS] = {};
int gc64_n64_rumble_motor_transport[GC64_DEBUG_PORTS] = {};
uint8_t gc64_n64_rumble_motor_expected[GC64_DEBUG_PORTS] = {};
uint8_t gc64_n64_rumble_motor_response[GC64_DEBUG_PORTS] = {};
