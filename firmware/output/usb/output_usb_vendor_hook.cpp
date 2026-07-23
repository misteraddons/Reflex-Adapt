#include "../../product_config.h"

#ifdef ADAPT_OUTPUT_USB_DEVICE

#include <stdint.h>

extern "C" bool adapt_usb_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, const void *request);

// Keep this shim free of TinyUSB headers. The donor TinyUSB headers declare
// tud_vendor_control_xfer_cb as weak, and including them here would make our
// override weak too.
extern "C" bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, const void *request) {
  return adapt_usb_vendor_control_xfer_cb(rhport, stage, request);
}

#endif  // ADAPT_OUTPUT_USB_DEVICE
