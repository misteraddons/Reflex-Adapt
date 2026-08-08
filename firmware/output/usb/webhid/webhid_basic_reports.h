#pragma once

#include <stdint.h>

uint16_t webhid_get_device_info_report(uint8_t* buffer);
uint16_t webhid_get_key_status_report(uint8_t* buffer);
uint16_t webhid_get_input_mode_report(uint8_t* buffer);
void webhid_set_input_mode_report_page(uint8_t page, uint8_t catalog_kind = 0);
