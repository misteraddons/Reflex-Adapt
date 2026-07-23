#pragma once

// GunCon MiSTer descriptor family extracted from
// output_descriptors_specialized_peripheral_runtime.h.

static const uint8_t gunconmister_report_descriptor[] = {
  0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
  0x09, 0x05,        // Usage (Game Pad)
  0xA1, 0x01,        // Collection (Application)
  0x05, 0x09,        //   Usage Page (Button)
  0x19, 0x01,        //   Usage Minimum (1)
  0x29, 0x03,        //   Usage Maximum (3)
  0x15, 0x00,        //   Logical Minimum (0)
  0x25, 0x01,        //   Logical Maximum (1)
  0x75, 0x01,        //   Report Size (1)
  0x95, 0x03,        //   Report Count (3)
  0x55, 0x00,        //   Unit Exponent (0)
  0x65, 0x00,        //   Unit (NONE)
  0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0x75, 0x01,        //   Report Size (1)
  0x95, 0x05,        //   Report Count (5)
  0x81, 0x03,        //   Input (Const,Var,Abs)
  0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
  0x09, 0x01,        //   Usage (Pointer)
  0x15, 0x00,        //   Logical Minimum (0)
  0x26, 0xFF, 0x03,  //   Logical Maximum (1023)
  0x75, 0x10,        //   Report Size (16)
  0x95, 0x02,        //   Report Count (2)
  0xA1, 0x00,        //   Collection (Physical)
  0x09, 0x30,        //     Usage (X)
  0x09, 0x31,        //     Usage (Y)
  0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0xC0,              //   End Collection
  // WebHID Configuration Reports (0xE0-0xE6)
  0x06, 0x00, 0xFF,  //   Usage Page (Vendor Defined 0xFF00)
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
  union {
    struct TU_ATTR_PACKED {
      uint8_t a : 1;
      uint8_t b : 1;
      uint8_t c : 1;
      uint8_t : 5; // 0
    };
    uint8_t buttons;
  };
  uint16_t x;
  uint16_t y;
} usbout_gunconmister_report_t;
