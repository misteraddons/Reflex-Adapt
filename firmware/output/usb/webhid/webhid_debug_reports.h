#pragma once

#include <stdint.h>

uint16_t webhid_get_input_state_report(uint8_t* buffer);
void webhid_set_input_state_player(const uint8_t* buffer, uint16_t bufsize);
uint16_t webhid_get_gpio_state_report(uint8_t* buffer);
uint16_t webhid_get_raw_data_report(uint8_t* buffer);
