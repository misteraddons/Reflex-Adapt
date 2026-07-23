#pragma once

#include <stdint.h>

void webhid_write_auth_key_report(const uint8_t* buffer, uint16_t bufsize);
void webhid_clear_auth_key_report(const uint8_t* buffer, uint16_t bufsize);
