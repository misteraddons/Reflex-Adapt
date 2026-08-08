#pragma once

#include <stdint.h>

uint16_t webhid_get_settings_report(uint8_t* buffer);
void webhid_write_settings_report(const uint8_t* buffer, uint16_t bufsize);
