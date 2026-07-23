#ifndef OUTPUT_XINPUT_DESCRIPTOR_STATE_H_
#define OUTPUT_XINPUT_DESCRIPTOR_STATE_H_

#include <stdint.h>

#include "out_xinput.h"

extern uint8_t xinput_hid_composite_config[TUD_XINPUT_HID_COMPOSITE_DESC_LEN];
extern uint8_t xinput_config_descriptor_mutable[sizeof(xinput_config_descriptor)];
extern bool descriptor_initialized;

extern uint8_t autodetect_config_mutable[56];
extern bool autodetect_config_initialized;

extern uint8_t debug_desc_device_calls;
extern uint8_t debug_desc_config_calls;
extern bool debug_hooks_registered;

void xinput_init_autodetect_config(uint16_t hid_report_desc_len);

#endif  // OUTPUT_XINPUT_DESCRIPTOR_STATE_H_
