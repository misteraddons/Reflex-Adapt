#pragma once

#include <stdint.h>

void webhid_handle_command_report(const uint8_t* buffer, uint16_t bufsize);
void webhid_handle_stats_report(const uint8_t* buffer, uint16_t bufsize);
void webhid_process_commands();
bool webhid_process_rumble();
