#include "../../product_config.h"
#ifdef ADAPT_OUTPUT_USB_DEVICE

#include <cstring>

#include "output_xid_interface_runtime.h"
#include "../xid/xid.h"

uint8_t xid_out_rhport = 0;
uint8_t xid_begin_called = 0;
uint8_t xid_begin_success = 0;

Adafruit_USBD_XID::Adafruit_USBD_XID() {}

bool Adafruit_USBD_XID::begin(void) {
  xid_begin_called++;
  if (!TinyUSBDevice.addInterface(*this)) {
    return false;
  }
  xid_begin_success++;
  return true;
}

uint16_t Adafruit_USBD_XID::getInterfaceDescriptor(uint8_t itfnum_deprecated, uint8_t *buf, uint16_t bufsize) {
  (void)itfnum_deprecated;

  uint8_t itfnum = 0;
  uint8_t ep_in = 0;
  uint8_t ep_out = 0;

  if (buf) {
    itfnum = TinyUSBDevice.allocInterface(1);
    ep_out = TinyUSBDevice.allocEndpoint(TUSB_DIR_OUT);
    ep_in = TinyUSBDevice.allocEndpoint(TUSB_DIR_IN);

    stored_itfnum = itfnum;
    stored_ep_in = ep_in;
    stored_ep_out = ep_out;
  }

  const uint8_t desc[] = { TUD_XID_WHEEL_DESCRIPTOR(itfnum, ep_out, ep_in) };
  const uint16_t len = sizeof(desc);

  if (bufsize < len)
    return 0;

  memcpy(buf, desc, len);
  return len;
}

#endif  // ADAPT_OUTPUT_USB_DEVICE
