#include "../../product_config.h"
#ifdef ADAPT_OUTPUT_USB_DEVICE

#include <stdint.h>

uint16_t const* usb_detect_handle_ms_os_10(void);
void usb_detect_handle_manufacturer_read(void);
void usb_detect_handle_descriptor_read(void);
void usb_detect_handle_edpt_clear_stall(uint8_t ep_addr);

extern "C" uint16_t const* get_ms_os_10_string_ptr(void) {
  return usb_detect_handle_ms_os_10();
}

void string_manufacturer_read_cb(void) {
  usb_detect_handle_manufacturer_read();
}

extern "C" void hid_descriptor_report_read_cb(void) {
  usb_detect_handle_descriptor_read();
}

extern "C" void usbd_edpt_clear_stall_cb(uint8_t ep_addr) {
  usb_detect_handle_edpt_clear_stall(ep_addr);
}

#endif  // ADAPT_OUTPUT_USB_DEVICE
