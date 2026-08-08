#include "../../product_config.h"
#ifdef ADAPT_OUTPUT_USB_DEVICE

#include <cstring>

#include "usb_detect_xinput_auth.h"

uint16_t Adafruit_USBD_XInputAuth::getInterfaceDescriptor(
    uint8_t itfnum_deprecated,
    uint8_t *buf,
    uint16_t bufsize
) {
  (void)itfnum_deprecated;

  static const uint8_t xinput_auth_itf_desc[] {
    0x09, 0x04, 0x00, 0x00, 0x00, 0xFF, 0xFD, 0x13, 0x00,
    0x06, 0x41, 0x00, 0x01, 0x01, 0x03
  };

  const uint16_t len = sizeof(xinput_auth_itf_desc);

  if (bufsize < len)
    return 0;

  if (buf) {
    memcpy(buf, xinput_auth_itf_desc, len);
    buf[2] = TinyUSBDevice.allocInterface(1);
    buf[8] = TinyUSBDevice.addStringDescriptor("Xbox Security Method 3, Version 1.00, \xc2\xa9 2005 Microsoft Corporation. All rights reserved.");
  }

  return len;
}

bool Adafruit_USBD_XInputAuth::begin(void) {
  if (!TinyUSBDevice.addInterface(*this))
    return false;
  return true;
}

#endif  // ADAPT_OUTPUT_USB_DEVICE
