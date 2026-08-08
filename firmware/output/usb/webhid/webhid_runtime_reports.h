#pragma once

#include <stdint.h>

uint16_t webhid_get_turbo_report(uint8_t* buffer);
uint16_t webhid_get_remap_report(uint8_t* buffer);
uint16_t webhid_get_input_history_report(uint8_t* buffer);
uint16_t webhid_get_stats_report(uint8_t* buffer);
