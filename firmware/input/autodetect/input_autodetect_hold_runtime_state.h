#pragma once

#include <Arduino.h>

extern uint16_t passive_assisted_hold_last_mask[];
extern uint32_t passive_assisted_hold_stable_since[];
extern bool passive_assisted_hold_latched[];

extern uint8_t jaguar_assist_hold_last_mask[];
extern uint32_t jaguar_assist_hold_stable_since[];
extern bool jaguar_assist_hold_latched[];
