#pragma once

#include <stddef.h>
#include <stdint.h>

uint16_t auth_msc_latency_csv_cluster_count();
bool auth_msc_latency_export_enabled();
void auth_msc_latency_reset();
void auth_msc_latency_refresh();
const char* auth_msc_latency_status_text();
size_t auth_msc_latency_status_len();
const uint8_t* auth_msc_latency_csv_data();
size_t auth_msc_latency_csv_len();
const char* auth_msc_latency_csv_short_name();
