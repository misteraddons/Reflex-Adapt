#include "../../product_config.h"
#ifdef ADAPT_OUTPUT_USB_DEVICE

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

#include <Arduino.h>

#include "out_xinput.h"
#include "output_xinput_auth_runtime.h"
#include "output_xinput_descriptor_runtime.h"
#include "output_xinput_descriptor_state.h"

Adafruit_USBD_XInput *xinput_device = NULL;

#ifdef ARDUINO_ARCH_ESP32
static uint16_t xinput_load_descriptor(uint8_t *dst, uint8_t *itf) {
    // uint8_t str_index = tinyusb_add_string_descriptor("TinyUSB XInput");
    uint8_t str_index = 0;

    uint8_t ep_in = tinyusb_get_free_in_endpoint();
    uint8_t ep_out = tinyusb_get_free_out_endpoint();
    TU_VERIFY(ep_in && ep_out);
    ep_in |= EPIN;

    const uint8_t descriptor[TUD_XINPUT_DESC_LEN] = {
        // Interface number, string index, EP Out & EP In address, EP size
        TUD_XINPUT_DESCRIPTOR(*itf, str_index, ep_out, ep_in, EPSIZE, 1)
    };

    *itf += 1;
    memcpy(dst, descriptor, TUD_XINPUT_DESC_LEN);
    return TUD_XINPUT_DESC_LEN;
}
#endif

Adafruit_USBD_XInput::Adafruit_USBD_XInput(uint8_t interval_ms) {
    _interval_ms = interval_ms;

#ifdef ARDUINO_ARCH_ESP32
    // ESP32 requires setup configuration descriptor within constructor
    xinput_device = this;
    const uint16_t desc_len = getInterfaceDescriptor(0, NULL, 0);
    tinyusb_enable_interface(USB_INTERFACE_VENDOR, desc_len, xinput_load_descriptor);
#endif
}

uint16_t Adafruit_USBD_XInput::getInterfaceDescriptor(
    uint8_t itfnum_deprecated,
    uint8_t *buf,
    uint16_t bufsize
) {
    (void)itfnum_deprecated;

    // Skip config header (9 bytes) - Adafruit adds its own header
    const uint16_t CONFIG_HEADER_SIZE = 9;
    const uint16_t len = sizeof(xinput_config_descriptor) - CONFIG_HEADER_SIZE;

    if (!buf) {
        return len;
    }

    if (bufsize < len) {
        return 0;
    }

    if (!descriptor_initialized) {
        xinput_set_subtype(XINPUT_SUBTYPE_GAMEPAD);
    }

    // Copy interface descriptors only (skip config header)
    memcpy(buf, &xinput_config_descriptor_mutable[CONFIG_HEADER_SIZE], len);

    // Allocate interfaces and endpoints
    // Use FIXED endpoint addresses matching official Xbox 360 controller
    // DO NOT use dynamic allocation - Xbox 360 console may expect specific addresses
    uint8_t itfnum = TinyUSBDevice.allocInterface(4);  // Allocate all 4 interfaces

    // Fixed endpoint addresses matching official Xbox 360 wired controller:
    // Gamepad: IN 0x81, OUT 0x02
    // Audio: IN 0x83, OUT 0x04, IN 0x85, OUT 0x06
    // Plugin: IN 0x86
    const uint8_t EP_GAMEPAD_IN  = 0x81;
    const uint8_t EP_GAMEPAD_OUT = 0x02;

    // Store our main endpoints (fixed values)
    _endpoint_in = EP_GAMEPAD_IN;
    _endpoint_out = EP_GAMEPAD_OUT;
    _security_itfnum = itfnum + 3;

    // Patch interface numbers (byte positions adjusted for -9 offset)
    // Original offsets: 11, 51, 115, 140 -> Now: 2, 42, 106, 131
    buf[2] = itfnum;         // Interface 0 (Control)
    buf[42] = itfnum + 1;    // Interface 1 (Audio)
    buf[106] = itfnum + 2;   // Interface 2 (Plugin)
    buf[131] = itfnum + 3;   // Interface 3 (Security)

    // Patch the security string index exactly like the donor implementation.
    buf[137] = TinyUSBDevice.addStringDescriptor(
        "Xbox Security Method 3, Version 1.00, \xc2\xa9 2005 Microsoft Corporation. All rights reserved.");

    // Endpoint addresses are already correct in xinput_config_descriptor_mutable.
    // No patching needed - use the hardcoded values from the donor descriptor.

    // update the bFirstInterface in MS OS 2.0 descriptor
    xinput_set_ms_os_first_interface(itfnum);

    return len;
}

bool Adafruit_USBD_XInput::begin(void) {
    if (!TinyUSBDevice.addInterface(*this)) {
        xinput_note_begin_result(false);
        return false;
    }

    // Xbox 360 uses USB 2.00, not 2.10
    TinyUSBDevice.setVersion(0x0200);

    // Set device version to match official Xbox 360 controller (1.10)
    TinyUSBDevice.setDeviceVersion(0x0110);
    TinyUSBDevice.setMaxPacketSize0(8);

    xinput_device = this;

    // Initialize XSM3 authentication
    xinput_auth_init();

    xinput_note_begin_result(true);
    return true;
}

#endif  // ADAPT_OUTPUT_USB_DEVICE
