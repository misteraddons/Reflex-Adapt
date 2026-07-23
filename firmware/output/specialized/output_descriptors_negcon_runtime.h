#pragma once

// neGcon MiSTer descriptor family extracted from
// output_descriptors_specialized_peripheral_runtime.h.

static const uint8_t negconmister_report_descriptor[] =
{
  0x05, 0x01,        // USAGE_PAGE (Generic Desktop)
  0x09, 0x05,        // USAGE (Gamepad)
  0xa1, 0x01,        // COLLECTION (Application)
  0x15, 0x00,        //   LOGICAL_MINIMUM (0)
  0x25, 0x01,        //   LOGICAL_MAXIMUM (1)
  0x35, 0x00,        //   PHYSICAL_MINIMUM (0)
  0x45, 0x01,        //   PHYSICAL_MAXIMUM (1)
  0x75, 0x01,        //   REPORT_SIZE (1)
  0x95, 0x0e,        //   REPORT_COUNT (14)
  0x05, 0x09,        //   USAGE_PAGE (Button)
  0x19, 0x01,        //   USAGE_MINIMUM (Button 1)
  0x29, 0x0e,        //   USAGE_MAXIMUM (Button 14)
  0x81, 0x02,        //   INPUT (Data,Var,Abs)
  0x95, 0x02,        //   REPORT_COUNT (3)
  0x81, 0x01,        //   INPUT (Cnst,Ary,Abs)
  0x05, 0x01,        //   USAGE_PAGE (Generic Desktop)
  0x25, 0x07,        //   LOGICAL_MAXIMUM (7)
  0x46, 0x3b, 0x01,  //   PHYSICAL_MAXIMUM (315)
  0x75, 0x04,        //   REPORT_SIZE (4)
  0x95, 0x01,        //   REPORT_COUNT (1)
  0x65, 0x14,        //   UNIT (Eng Rot:Angular Pos)
  0x09, 0x39,        //   USAGE (Hat switch)
  0x81, 0x42,        //   INPUT (Data,Var,Abs,Null)
  0x65, 0x00,        //   UNIT (None)
  0x95, 0x01,        //   REPORT_COUNT (1)
  0x81, 0x01,        //   INPUT (Cnst,Ary,Abs)
  0x26, 0xff, 0x00,  //   LOGICAL_MAXIMUM (255)
  0x46, 0xff, 0x00,  //   PHYSICAL_MAXIMUM (255)
  0x09, 0x30,        //   USAGE (X)
  //0x09, 0x31,        //   USAGE (Y)
  0x09, 0x32,        //   USAGE (Z)
  //0x09, 0x35,        //   USAGE (Rz)
  0x75, 0x08,        //   REPORT_SIZE (8)
  //0x95, 0x04,        //   REPORT_COUNT (4)
  0x95, 0x02,        //   REPORT_COUNT (2)
  0x81, 0x02,        //   INPUT (Data,Var,Abs)

  0x05, 0x02,        //   USAGE_PAGE (Simulation Controls)
  0x75, 0x08,        //   REPORT_SIZE (8)
  0x95, 0x03,        //   REPORT_COUNT (3)
  0x15, 0x00,        //   LOGICAL_MINIMUM (0)
  0x26, 0xff, 0x00,  //   LOGICAL_MAXIMUM (255)
  //0x35, 0x00,        //   PHYSICAL_MINIMUM (0)
  //0x46, 0xff, 0x00,  //   PHYSICAL_MAXIMUM (255)
  0x09, 0xbb,        //   USAGE (Throttle)
  0x09, 0xc5,        //   USAGE (Brake)
  0x09, 0xc8,        //   USAGE (Steering)
  0x81, 0x02,        //   INPUT (Data,Var,Abs)

  // 0x06, 0x00, 0xff,  //   USAGE_PAGE (Vendor Specific)
  // 0x09, 0x20,        //   Unknown
  // 0x09, 0x21,        //   Unknown
  // 0x09, 0x22,        //   Unknown
  // 0x09, 0x23,        //   Unknown
  // 0x09, 0x24,        //   Unknown
  // 0x09, 0x25,        //   Unknown
  // 0x09, 0x26,        //   Unknown
  // 0x09, 0x27,        //   Unknown
  // 0x09, 0x28,        //   Unknown
  // 0x09, 0x29,        //   Unknown
  // 0x09, 0x2a,        //   Unknown
  // 0x09, 0x2b,        //   Unknown
  // 0x95, 0x0c,        //   REPORT_COUNT (12)
  // 0x81, 0x02,        //   INPUT (Data,Var,Abs)
  // 0x0a, 0x21, 0x26,  //   Unknown
  // 0x95, 0x08,        //   REPORT_COUNT (8)
  // 0xb1, 0x02,        //   FEATURE (Data,Var,Abs)
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
  0xc0               // END_COLLECTION
};

typedef struct TU_ATTR_PACKED {
  // digital buttons, 0 = off, 1 = on

  uint8_t square_btn : 1;   // n/a
  uint8_t cross_btn : 1;    // n/a
  uint8_t circle_btn : 1;   // neg A, volume B
  uint8_t triangle_btn : 1; // neg B

  uint8_t l1_btn : 1;       // n/a
  uint8_t r1_btn : 1;       // neg r
  uint8_t l2_btn : 1;       // n/a
  uint8_t r2_btn : 1;       // n/a

  uint8_t select_btn : 1;   // n/a
  uint8_t start_btn : 1;    // neg start, volume A, pachinko
  uint8_t l3_btn : 1;       // n/a
  uint8_t r3_btn : 1;       // n/a
  
  uint8_t ps_btn : 1;       // n/a
  uint8_t guide2_btn : 1;   // MiSTer OSD chord partner
  uint8_t : 2;

  // digital direction, use the dir_* constants(enum)
  // 8 = center, 0 = up, 1 = up/right, 2 = right, 3 = right/down
  // 4 = down, 5 = down/left, 6 = left, 7 = left/up

  uint8_t direction;

  // left and right analog sticks, 0x00 left/up, 0x80 middle, 0xff right/down

  uint8_t axis_twist; //negcon X  (x)
  uint8_t axis_l; //negcon L  (z)
  uint8_t axis_i; //negcon I  (throttle)
  uint8_t axis_ii; //negcon II (brake)
  uint8_t axis_paddle; //paddle (steering)
} usbout_negconmister_report_t;
