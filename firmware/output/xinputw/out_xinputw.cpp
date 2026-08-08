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

#include <device/usbd_pvt.h>
#include <tusb_option.h>

#include "out_xinputw.h"
#include "output_xinputw_runtime.h"

// TODO multiple instances
Adafruit_USBD_XInputW *_xinput_dev = NULL;
XInputWDiagInfo xinputw_diag = {};


//------------- IMPLEMENTATION -------------//

Adafruit_USBD_XInputW::Adafruit_USBD_XInputW(uint8_t interval_ms) {
    _interval_ms = interval_ms;
}

uint16_t Adafruit_USBD_XInputW::getInterfaceDescriptor(
    uint8_t itfnum_deprecated,
    uint8_t *buf,
    uint16_t bufsize
) {
    const uint16_t len = TUD_XINPUTW_DESC_LEN * XINPUT_WIRELESS_CONTROLLERS;

    // null buffer is used to get the length of descriptor only
    if (!buf) {
        return len;
    }

    if (bufsize < len) {
        return 0;
    }

    uint8_t ep_out[XINPUT_WIRELESS_CONTROLLERS] {0};
    uint8_t ep_in[XINPUT_WIRELESS_CONTROLLERS] {0};
    _itf_base = TinyUSBDevice.allocInterface(XINPUT_WIRELESS_CONTROLLERS);
    for (uint8_t i = 0; i < XINPUT_WIRELESS_CONTROLLERS; ++i) {
      ep_out[i] = TinyUSBDevice.allocEndpoint(TUSB_DIR_OUT);
      ep_in[i] = TinyUSBDevice.allocEndpoint(TUSB_DIR_IN);
    }

    // usb core will automatically update endpoint number
    //const uint8_t desc[] = { TUD_XINPUT_DESCRIPTOR(itfnum, 0, EPOUT, EPIN, XINPUTW_EPSIZE, _interval_ms) };
    for (uint8_t i = 0; i < XINPUT_WIRELESS_CONTROLLERS; ++i) {
      const uint8_t desc[TUD_XINPUTW_DESC_LEN] = { TUD_XINPUTW_DESCRIPTOR((uint8_t)(_itf_base + i), 0, ep_out[i], ep_in[i], XINPUTW_EPSIZE, _interval_ms) };
      memcpy(buf + (i * TUD_XINPUTW_DESC_LEN), desc, TUD_XINPUTW_DESC_LEN);
    }

    return len;
}

bool Adafruit_USBD_XInputW::begin(void) {
    _itf_base = 0xFF;
    memset(&xinputw_diag, 0, sizeof(xinputw_diag));
    if (!TinyUSBDevice.addInterface(*this)) {
        return false;
    }

    //TinyUSBDevice.setVersion(0x0210);

    for (uint8_t i = 0; i < XINPUT_WIRELESS_CONTROLLERS; ++i) {
      interfaces[i]._endpoint_in = 0;
      interfaces[i]._endpoint_out = 0;
      memset(interfaces[i]._xinput_out_buffer, 0, XINPUTW_EPSIZE);
      interfaces[i].info_state = DISCONNECTED;
      interfaces[i].connected = false;
    }

    _xinput_dev = this;
  
    return true;
}
