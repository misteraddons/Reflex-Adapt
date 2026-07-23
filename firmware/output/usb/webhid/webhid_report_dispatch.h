#pragma once

#include <stdint.h>

uint16_t webhid_get_report(uint8_t report_id, uint8_t* buffer, uint16_t reqlen);
void webhid_set_report(uint8_t report_id, const uint8_t* buffer, uint16_t bufsize);
