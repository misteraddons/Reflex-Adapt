#ifndef _USB_DETECT_DESCRIPTOR_DATA_H_
#define _USB_DETECT_DESCRIPTOR_DATA_H_

#include <stdint.h>

// Example: Extended Compatibility ID Descriptor
static const uint8_t MS_CompatIDDescriptor[] = {
    0x28, 0x00, 0x00, 0x00,
    0x00, 0x01,
    0x04, 0x00,
    0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00,
    0x01,
    'H','I','D',0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0x00,0x00,0x00,0x00,0x00,0x00
};

// Hardcoded MS OS 1.0 string descriptor (MSFT100 + vendor code + padding)
// Format: bLength, bDescriptorType(0x03), UTF-16LE "MSFT100", bVendorCode, bPad
// bLength = 18 (0x12)
// Windows caches it here HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\usbflags
static const uint8_t ms_os_string_desc[] = {
  0x12, 0x03,
  'M', 0x00, 'S', 0x00, 'F', 0x00, 'T', 0x00, '1', 0x00, '0', 0x00, '0', 0x00,
  0x20,
  0x00
};

#endif  // _USB_DETECT_DESCRIPTOR_DATA_H_
