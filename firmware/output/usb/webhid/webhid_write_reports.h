#pragma once

#include <stdint.h>

void webhid_write_turbo_report(const uint8_t* buffer, uint16_t bufsize);
void webhid_write_remap_report(const uint8_t* buffer, uint16_t bufsize);
