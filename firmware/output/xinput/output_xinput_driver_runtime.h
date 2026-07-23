#ifndef OUTPUT_XINPUT_DRIVER_RUNTIME_H_
#define OUTPUT_XINPUT_DRIVER_RUNTIME_H_

#include "out_xinput.h"

extern uint8_t xinput_out_rhport;

bool _xinput_tud_vendor_control_xfer_cb(
    uint8_t rhport,
    uint8_t stage,
    const tusb_control_request_t *request
);

#endif  // OUTPUT_XINPUT_DRIVER_RUNTIME_H_
