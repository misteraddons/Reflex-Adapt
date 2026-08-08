#pragma once

#include <stddef.h>
#include <stdint.h>

void auth_msc_status_reset();
void auth_msc_status_refresh();
const char* auth_msc_status_text();
size_t auth_msc_status_len();
void auth_msc_status_set_result_text(const char* name, const char* text);
void auth_msc_status_set_result_with_name(const char* format, const char* name, uint32_t size);
