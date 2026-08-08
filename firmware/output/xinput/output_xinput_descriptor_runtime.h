#ifndef OUTPUT_XINPUT_DESCRIPTOR_RUNTIME_H_
#define OUTPUT_XINPUT_DESCRIPTOR_RUNTIME_H_

#include <stdint.h>

#include "out_xinput.h"

void xinput_fill_descriptor_debug_info(XInputDebugInfo* info);
bool xinput_handle_ms_os_descriptor_request(
    uint8_t rhport,
    const tusb_control_request_t* request
);
void xinput_set_ms_os_first_interface(uint8_t itfnum);

#endif  // OUTPUT_XINPUT_DESCRIPTOR_RUNTIME_H_
