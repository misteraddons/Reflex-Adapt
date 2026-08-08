#pragma once
// GameCube-to-Wii U descriptor family extracted from
// output_descriptors_specialized_runtime.h.
static const uint8_t gcwiiu_report_desc[] = {
  0x05, 0x05,        // Usage Page (Game Ctrls)
  0x09, 0x00,        // Usage (Undefined)
  0xA1, 0x01,        // Collection (Application)
  0x85, 0x11,        //   Report ID (17)
  0x19, 0x00,        //   Usage Minimum (Undefined)
  0x2A, 0xFF, 0x00,  //   Usage Maximum (0xFF)
  0x15, 0x00,        //   Logical Minimum (0)
  0x26, 0xFF, 0x00,  //   Logical Maximum (255)
  0x75, 0x08,        //   Report Size (8)
  0x95, 0x05,        //   Report Count (5)
  0x91, 0x00,        //   Output (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
  0xC0,              // End Collection
  0xA1, 0x01,        // Collection (Application)
  0x85, 0x21,        //   Report ID (33)
  0x19, 0x00,        //   Usage Minimum (Undefined)
  0x2A, 0xFF, 0x00,  //   Usage Maximum (0xFF)
  0x15, 0x00,        //   Logical Minimum (0)
  0x26, 0xFF, 0x00,  //   Logical Maximum (255)
  0x75, 0x08,        //   Report Size (8)
  0x95, 0x25,        //   Report Count (37)
  0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0xC0,              // End Collection
  0xA1, 0x01,        // Collection (Application)
  0x85, 0x12,        //   Report ID (18)
  0x19, 0x00,        //   Usage Minimum (Undefined)
  0x2A, 0xFF, 0x00,  //   Usage Maximum (0xFF)
  0x15, 0x00,        //   Logical Minimum (0)
  0x26, 0xFF, 0x00,  //   Logical Maximum (255)
  0x75, 0x08,        //   Report Size (8)
  0x95, 0x01,        //   Report Count (1)
  0x91, 0x00,        //   Output (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
  0xC0,              // End Collection
  0xA1, 0x01,        // Collection (Application)
  0x85, 0x22,        //   Report ID (34)
  0x19, 0x00,        //   Usage Minimum (Undefined)
  0x2A, 0xFF, 0x00,  //   Usage Maximum (0xFF)
  0x15, 0x00,        //   Logical Minimum (0)
  0x26, 0xFF, 0x00,  //   Logical Maximum (255)
  0x75, 0x08,        //   Report Size (8)
  0x95, 0x19,        //   Report Count (25)
  0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0xC0,              // End Collection
  0xA1, 0x01,        // Collection (Application)
  0x85, 0x13,        //   Report ID (19)
  0x19, 0x00,        //   Usage Minimum (Undefined)
  0x2A, 0xFF, 0x00,  //   Usage Maximum (0xFF)
  0x15, 0x00,        //   Logical Minimum (0)
  0x26, 0xFF, 0x00,  //   Logical Maximum (255)
  0x75, 0x08,        //   Report Size (8)
  0x95, 0x01,        //   Report Count (1)
  0x91, 0x00,        //   Output (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
  0xC0,              // End Collection
  0xA1, 0x01,        // Collection (Application)
  0x85, 0x23,        //   Report ID (35)
  0x19, 0x00,        //   Usage Minimum (Undefined)
  0x2A, 0xFF, 0x00,  //   Usage Maximum (0xFF)
  0x15, 0x00,        //   Logical Minimum (0)
  0x26, 0xFF, 0x00,  //   Logical Maximum (255)
  0x75, 0x08,        //   Report Size (8)
  0x95, 0x02,        //   Report Count (2)
  0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0xC0,              // End Collection
  0xA1, 0x01,        // Collection (Application)
  0x85, 0x14,        //   Report ID (20)
  0x19, 0x00,        //   Usage Minimum (Undefined)
  0x2A, 0xFF, 0x00,  //   Usage Maximum (0xFF)
  0x15, 0x00,        //   Logical Minimum (0)
  0x26, 0xFF, 0x00,  //   Logical Maximum (255)
  0x75, 0x08,        //   Report Size (8)
  0x95, 0x01,        //   Report Count (1)
  0x91, 0x00,        //   Output (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
  0xC0,              // End Collection
  0xA1, 0x01,        // Collection (Application)
  0x85, 0x24,        //   Report ID (36)
  0x19, 0x00,        //   Usage Minimum (Undefined)
  0x2A, 0xFF, 0x00,  //   Usage Maximum (0xFF)
  0x15, 0x00,        //   Logical Minimum (0)
  0x26, 0xFF, 0x00,  //   Logical Maximum (255)
  0x75, 0x08,        //   Report Size (8)
  0x95, 0x02,        //   Report Count (2)
  0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0xC0,              // End Collection
  0xA1, 0x01,        // Collection (Application)
  0x85, 0x15,        //   Report ID (21)
  0x19, 0x00,        //   Usage Minimum (Undefined)
  0x2A, 0xFF, 0x00,  //   Usage Maximum (0xFF)
  0x15, 0x00,        //   Logical Minimum (0)
  0x26, 0xFF, 0x00,  //   Logical Maximum (255)
  0x75, 0x08,        //   Report Size (8)
  0x95, 0x01,        //   Report Count (1)
  0x91, 0x00,        //   Output (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
  0xC0,              // End Collection
  0xA1, 0x01,        // Collection (Application)
  0x85, 0x25,        //   Report ID (37)
  0x19, 0x00,        //   Usage Minimum (Undefined)
  0x2A, 0xFF, 0x00,  //   Usage Maximum (0xFF)
  0x15, 0x00,        //   Logical Minimum (0)
  0x26, 0xFF, 0x00,  //   Logical Maximum (255)
  0x75, 0x08,        //   Report Size (8)
  0x95, 0x02,        //   Report Count (2)
  0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0xC0,              // End Collection
};

typedef struct TU_ATTR_PACKED {
  uint8_t : 1; // 0
  uint8_t : 1; // 0 wavebird sets this
  uint8_t powered : 1; // 1 powered? or featuring rumble? just having the usb power cable connected sets this on all four ports
  uint8_t : 1; // 0
  uint8_t connection_type : 2; // 0:not connected, 1: wired, 2: wireless.
  uint8_t : 1; // 0
  uint8_t : 1; // 0

  union {
    struct TU_ATTR_PACKED {
      uint8_t a : 1;
      uint8_t b : 1;
      uint8_t x : 1;
      uint8_t y : 1;
      uint8_t hat_l : 1;
      uint8_t hat_r : 1;
      uint8_t hat_d : 1;
      uint8_t hat_u : 1;
      uint8_t start : 1;
      uint8_t z : 1;
      uint8_t r : 1;
      uint8_t l : 1;
      uint8_t : 4; // 0
    };
    uint16_t buttons;
  };

  uint8_t lx;
  uint8_t ly;
  uint8_t rx;
  uint8_t ry;
  uint8_t lt;
  uint8_t rt;
} usbout_gc2wiiu_port_report_t;

typedef struct TU_ATTR_PACKED {
  uint8_t report_id; // 0x21
  usbout_gc2wiiu_port_report_t port[4];
} usbout_gc2wiiu_report_t;

typedef struct TU_ATTR_PACKED {
  //uint8_t report_id; // 0x11
  uint8_t port[4];
} usbout_gc2wiiu_rumble_report_t;
