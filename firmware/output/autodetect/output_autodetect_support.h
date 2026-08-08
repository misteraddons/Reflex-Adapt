#pragma once

#include <stdint.h>

uint16_t usb_detect_probe_device_version(void);
bool is_ps5_extended_feature_probe(uint8_t report_id);
void auto_detect_trigger_reenum(uint32_t next_output_mode);
