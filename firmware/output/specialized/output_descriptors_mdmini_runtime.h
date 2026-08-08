#pragma once

// Mega Drive Mini descriptor and report definitions extracted from
// output_descriptors_specialized_runtime.h.

static const uint8_t mdmini_report_descriptor[] = {
  0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
  0x09, 0x04,        // Usage (Joystick)
  0xA1, 0x01,        // Collection (Application)
  0xA1, 0x02,        //   Collection (Logical)
  0x75, 0x08,        //     Report Size (8)
  0x95, 0x05,        //     Report Count (5)
  0x15, 0x00,        //     Logical Minimum (0)
  0x26, 0xFF, 0x00,  //     Logical Maximum (255)
  0x35, 0x00,        //     Physical Minimum (0)
  0x46, 0xFF, 0x00,  //     Physical Maximum (255)
  0x09, 0x30,        //     Usage (X)
  0x09, 0x30,        //     Usage (X)
  0x09, 0x30,        //     Usage (X)
  0x09, 0x30,        //     Usage (X)
  0x09, 0x31,        //     Usage (Y)
  0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0x75, 0x04,        //     Report Size (4)
  0x95, 0x01,        //     Report Count (1)
  0x25, 0x07,        //     Logical Maximum (7)
  0x46, 0x3B, 0x01,  //     Physical Maximum (315)
  0x65, 0x14,        //     Unit (System: English Rotation, Length: Centimeter)
  0x09, 0x00,        //     Usage (Undefined)
  0x81, 0x42,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,Null State)
  0x65, 0x00,        //     Unit (None)
  0x75, 0x01,        //     Report Size (1)
  0x95, 0x0A,        //     Report Count (10)
  0x25, 0x01,        //     Logical Maximum (1)
  0x45, 0x01,        //     Physical Maximum (1)
  0x05, 0x09,        //     Usage Page (Button)
  0x19, 0x01,        //     Usage Minimum (0x01)
  0x29, 0x0A,        //     Usage Maximum (0x0A)
  0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0x06, 0x00, 0xFF,  //     Usage Page (Vendor Defined 0xFF00)
  0x75, 0x01,        //     Report Size (1)
  0x95, 0x0A,        //     Report Count (10)
  0x25, 0x01,        //     Logical Maximum (1)
  0x45, 0x01,        //     Physical Maximum (1)
  0x09, 0x01,        //     Usage (0x01)
  0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0xC0,              //   End Collection
  0xA1, 0x02,        //   Collection (Logical)
  0x75, 0x08,        //     Report Size (8)
  0x95, 0x04,        //     Report Count (4)
  0x46, 0xFF, 0x00,  //     Physical Maximum (255)
  0x26, 0xFF, 0x00,  //     Logical Maximum (255)
  0x09, 0x02,        //     Usage (0x02)
  0x91, 0x02,        //     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
  0xC0,              //   End Collection
  // WebHID Configuration Reports (0xE0-0xE6)
  0xA1, 0x02,        //   Collection (Logical)
  0x85, 0xE0,        //     Report ID (0xE0) - Device Info
  0x75, 0x08,        //     Report Size (8)
  0x95, 0x3F,        //     Report Count (63)
  0x09, 0x01,        //     Usage (0x01)
  0xB1, 0x02,        //     Feature (Data,Var,Abs)
  0xC0,              //   End Collection
  0xA1, 0x02,        //   Collection (Logical)
  0x85, 0xE3,        //     Report ID (0xE3) - Commands
  0x75, 0x08,        //     Report Size (8)
  0x95, 0x3F,        //     Report Count (63)
  0x09, 0x01,        //     Usage (0x01)
  0xB1, 0x02,        //     Feature (Data,Var,Abs)
  0xC0,              //   End Collection
  0xA1, 0x02,        //   Collection (Logical)
  0x85, 0xE4,        //     Report ID (0xE4) - Key Status
  0x75, 0x08,        //     Report Size (8)
  0x95, 0x3F,        //     Report Count (63)
  0x09, 0x01,        //     Usage (0x01)
  0xB1, 0x02,        //     Feature (Data,Var,Abs)
  0xC0,              //   End Collection
  0xA1, 0x02,        //   Collection (Logical)
  0x85, 0xE5,        //     Report ID (0xE5) - Key Write
  0x75, 0x08,        //     Report Size (8)
  0x95, 0x3F,        //     Report Count (63)
  0x09, 0x01,        //     Usage (0x01)
  0xB1, 0x02,        //     Feature (Data,Var,Abs)
  0xC0,              //   End Collection
  0xA1, 0x02,        //   Collection (Logical)
  0x85, 0xE6,        //     Report ID (0xE6) - Key Clear
  0x75, 0x08,        //     Report Size (8)
  0x95, 0x3F,        //     Report Count (63)
  0x09, 0x01,        //     Usage (0x01)
  0xB1, 0x02,        //     Feature (Data,Var,Abs)
  0xC0,              //   End Collection
  0xA1, 0x02,        //   Collection (Logical)
  0x85, 0xE7,        //     Report ID (0xE7) - Input State
  0x75, 0x08,        //     Report Size (8)
  0x95, 0x3F,        //     Report Count (63)
  0x09, 0x01,        //     Usage (0x01)
  0xB1, 0x02,        //     Feature (Data,Var,Abs)
  0xC0,              //   End Collection
  0xC0,              // End Collection
};

typedef struct TU_ATTR_PACKED {
  uint8_t report_id; // 0x1.

  uint8_t : 8;
  uint8_t : 8;

  uint8_t lx;
  uint8_t ly;

      uint8_t : 4;
      uint8_t y : 1;
      uint8_t b : 1;
      uint8_t a : 1;
      uint8_t x : 1;
      uint8_t z : 1;
      uint8_t c : 1;
      uint8_t l : 1; // only on m30 2.4g usb dongle?
      uint8_t r : 1; // only on m30 2.4g usb dongle?
      uint8_t mode : 1;
      uint8_t start : 1;
      uint16_t : 10; // (vendor)
} usbout_mdmini_report_t;
