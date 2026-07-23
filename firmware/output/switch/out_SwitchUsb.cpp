#include "../../product_config.h"
#ifdef ADAPT_OUTPUT_USB_DEVICE

/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
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
 *
 */

#include "out_SwitchUsb.h"

#include "pico/rand.h"
#include <string.h>
#include <tusb.h>

uint8_t *SwitchUsb::generate_usb_report() {
  set_empty_report();

  if (_switchRequestReport[0] == 0x80) {
    _report[0] = 0x81;
    _report[1] = _switchRequestReport[1];
    switch (_switchRequestReport[1]) {
      case 0x01:
        _report[3] = 0x03;
        for (int i = 0; i < 6; i++) {
          _report[4 + i] = _addr[5 - i];
        }
        break;
      case 0x02:
      case 0x03:
        break;
      default:
        _report[0] = 0x30;
        //_controller->getSwitchReport(&_switchReport);
        memcpy(_report + 2, (uint8_t *)&_switchReport, sizeof(SwitchReport));
        break;
    }
    if (_switchRequestReport[0] > 0x00) {
      set_empty_switch_request_report();
    }
    return _report;
  } else {
    generate_report();
    if (_switchRequestReport[0] > 0x00) {
      set_empty_switch_request_report();
    }
    // _report is a bluetooth report starting with 0xA1, which usb skips
    return _report + 1;
  }
};

void SwitchUsb::init() {
  //_controller = controller;
  _switchReport.batteryConnection = 0x81;
  _switchRequestReport[0] = 0x80;
  _switchRequestReport[1] = 0x01;
  uint8_t newAddr[] = {0x7c,
                       0xbb,
                       0x8a,
                       (uint8_t)(get_rand_32() % 0xff),
                       (uint8_t)(get_rand_32() % 0xff),
                       (uint8_t)(get_rand_32() % 0xff)};
  memcpy(_addr, newAddr, 6);
}

#endif // ADAPT_OUTPUT_USB_DEVICE
