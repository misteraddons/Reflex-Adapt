/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach for Adafruit Industries
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

//https://github.com/JonnyHaystack/Adafruit_TinyUSB_XInput/tree/master
//need to delete Adafruit_USBD_WebUSB

#ifndef ADAFRUIT_USBD_XINPUT_HPP_
#define ADAFRUIT_USBD_XINPUT_HPP_

#include <arduino/Adafruit_USBD_Device.h>
#include <device/usbd_pvt.h>
#include <tusb_option.h>
#include <cstddef> // offsetof
#include "output_xinput_protocol_runtime.h"
#include "output_xinput_report_types.h"

extern void rumble_callback(uint8_t index, uint8_t left, uint8_t right);

bool tud_xinput_ready();
void receive_xinput_report(void);
bool send_xinput_report(xinput_report_t *report);
uint16_t xinput_open(
    uint8_t rhport,
    const tusb_desc_interface_t *itf_descriptor,
    uint16_t max_length
);
bool xinput_xfer_callback(
    uint8_t rhport,
    uint8_t ep_addr,
    xfer_result_t result,
    uint32_t xferred_bytes
);

// XSM3 authentication functions
void xinput_auth_init(void);
void xinput_auth_process(void);  // Call from main loop to process pending auth
bool xinput_auth_handle_control(uint8_t rhport, uint8_t stage, const tusb_control_request_t *request);
bool xinput_auth_handle_control_for_security_interface(
    uint8_t rhport,
    uint8_t stage,
    const tusb_control_request_t *request,
    uint8_t security_itf
);
bool xinput_is_authenticated(void);
void xinput_auth_invalidate(void);  // Force reinit of XSM3 auth state

// Descriptor hook registration - call before TinyUSBDevice.begin()
void xinput_register_descriptor_hooks(void);

// WCID detection - set true when Windows requests string index 0xEE (MS OS String Descriptor)
extern bool xinput_seen_string_0xEE;
// Security interface string request (index 4) seen during enumeration.
// Useful as an early Xbox 360 hint in OUTPUT_AUTO Stage 0.
extern bool xinput_seen_string_4;

// XInput IN endpoint polling detection - set true when host consumes an XInput report
// Used by auto-detect to distinguish Windows (polls XInput) from Wii U (passive)
extern bool xinput_seen_in_xfer;

// Get debug info
void xinput_get_debug_info(XInputDebugInfo* info);

class Adafruit_USBD_XInput : public Adafruit_USBD_Interface {
  public:
    Adafruit_USBD_XInput(uint8_t interval_ms = 1);

    bool begin(void);

    bool ready(void);
    bool sendReport(xinput_report_t *report);

    // from Adafruit_USBD_Interface
    virtual uint16_t getInterfaceDescriptor(uint8_t itfnum, uint8_t *buf, uint16_t bufsize);

  private:
    uint8_t _interval_ms;
    uint8_t _endpoint_in = 0;
    uint8_t _endpoint_out = 0;
    uint8_t _xinput_out_buffer[EPSIZE] = {};
    uint8_t _security_itfnum = 0;

    friend bool tud_xinput_ready();
    friend void receive_xinput_report(void);
    friend bool send_xinput_report(xinput_report_t *report);

    friend uint16_t xinput_open(
        uint8_t rhport,
        const tusb_desc_interface_t *itf_descriptor,
        uint16_t max_length
    );
    friend bool xinput_xfer_callback(
        uint8_t rhport,
        uint8_t ep_addr,
        xfer_result_t result,
        uint32_t xferred_bytes
    );
    friend const usbd_class_driver_t *usbd_app_driver_get_cb(uint8_t *driver_count);
    friend bool _xinput_tud_vendor_control_xfer_cb(
        uint8_t rhport,
        uint8_t stage,
        const tusb_control_request_t *request
    );
    friend void xinput_auth_init(void);
    friend bool xinput_auth_handle_control(uint8_t rhport, uint8_t stage, const tusb_control_request_t *request);
    friend void xinput_get_debug_info(XInputDebugInfo* info);
};

// Flag to enable raw Xbox 360 descriptors (bypasses Adafruit wrapper)
extern bool xinput_use_raw_descriptors;

// Composite XInput+HID mode for auto-detect Stage 0
// When true, config descriptor includes HID interface 4 alongside XInput
extern bool xinput_composite_hid_enabled;

// Build and return the composite XInput+HID config descriptor
// Must be called after xinput_set_subtype() and before enumeration
void xinput_build_composite_descriptor(const uint8_t* hid_report_desc, uint16_t hid_report_desc_len);

// Auto-detect HID-primary descriptor setup (Stage 0)
// Prepares the mutable HID+Security config descriptor (VID 16D0:1460).
void autodetect_register_descriptor_hooks(uint16_t hid_report_desc_len);

#ifdef __cplusplus
extern "C"
{
#endif
  const usbd_class_driver_t *xinput_get_driver();
#ifdef __cplusplus
}
#endif

#endif /* ADAFRUIT_USBD_XINPUT_HPP_ */

