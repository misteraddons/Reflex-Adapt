#pragma once

#include <stdint.h>

uint32_t auth_msc_virtual_drive_sector_count();
uint16_t auth_msc_virtual_drive_sector_size();
void auth_msc_virtual_drive_build();
void auth_msc_virtual_drive_refresh_status();
void auth_msc_virtual_drive_refresh_latency();
void auth_msc_virtual_drive_process_import();
void auth_msc_virtual_drive_read_sector(uint32_t lba, uint8_t* out);
void auth_msc_virtual_drive_write_sector(uint32_t lba, const uint8_t* in);
