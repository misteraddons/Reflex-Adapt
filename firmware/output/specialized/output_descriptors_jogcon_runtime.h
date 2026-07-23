#pragma once

// JogCon MiSTer descriptor family extracted from
// output_descriptors_specialized_peripheral_runtime.h.

static const uint8_t jogconmister_report_descriptor[] = {
  0x05, 0x01,                       // USAGE_PAGE (Generic Desktop)
  0x09, 0x04,                       // USAGE (Joystick) (Maybe change to gamepad? I don't think so but...)
  0xa1, 0x01,                       // COLLECTION (Application)
    0xa1, 0x00,                     // COLLECTION (Physical)
    
      0x05, 0x09,                   // USAGE_PAGE (Button)
      0x19, 0x01,                   // USAGE_MINIMUM (Button 1)
      //0x29, 0x0C,                   // USAGE_MAXIMUM (Button 12)
    0x29, 0x10,                   // USAGE_MAXIMUM (Button 16)
      0x15, 0x00,                   // LOGICAL_MINIMUM (0)
      0x25, 0x01,                   // LOGICAL_MAXIMUM (1)
      0x95, 0x10,                   // REPORT_COUNT (16)
      0x75, 0x01,                   // REPORT_SIZE (1)
      0x81, 0x02,                   // INPUT (Data,Var,Abs)
    
      0x05, 0x01,                   // USAGE_PAGE (Generic Desktop)
      0x09, 0x01,                   // USAGE (pointer)
      0xa1, 0x00,                   // COLLECTION (Physical) 
        0x09, 0x30,                 // USAGE (X)
        0x09, 0x31,                 // USAGE (Y)
        0x15, 0x80,                 // LOGICAL_MINIMUM (-128)
        0x25, 0x7F,                 // LOGICAL_MAXIMUM (127)
        0x95, 0x02,                 // REPORT_COUNT (2)
        0x75, 0x08,                 // REPORT_SIZE (8)
        0x81, 0x02,                 // INPUT (Data,Var,Abs)
      0xc0,                         // END_COLLECTION

      0x09, 0x37,                   // USAGE (Dial)
      0x15, 0x80,                   // LOGICAL_MINIMUM (-128)
      0x25, 0x7F,                   // LOGICAL_MAXIMUM (127)
      0x95, 0x01,                   // REPORT_COUNT (1)
      0x75, 0x08,                   // REPORT_SIZE (8)
      0x81, 0x06,                   // INPUT (Data,Var,Rel)

      0x09, 0x38,                   // USAGE (Wheel)
      0x15, 0x00,                   // LOGICAL_MINIMUM (0)
      0x26, 0xFF, 0x00,             // LOGICAL_MAXIMUM (255)
      0x95, 0x01,                   // REPORT_COUNT (1)
      0x75, 0x08,                   // REPORT_SIZE (8)
      0x81, 0x02,                   // INPUT (Data,Var,Abs)

      0x09, 0x39,                   // Usage (Hat switch)
      0x15, 0x00,                   // Logical Minimum (0)
      0x25, 0x07,                   // Logical Maximum (7)
      0x35, 0x00,                   // Physical Minimum (0)
      0x46, 0x3B, 0x01,             // Physical Maximum (315)
      0x75, 0x08,                   // Report Size (8)
      0x95, 0x01,                   // Report Count (1)
      0x65, 0x14,                   // Unit (20)
      0x81, 0x42,                   // Input (variable,absolute,null_state)

    0xc0,                           // END_COLLECTION
  // WebHID Configuration Reports (0xE0-0xE6)
  0x06, 0x00, 0xFF,               // Usage Page (Vendor Defined 0xFF00)
  0xA1, 0x02,                     // Collection (Logical)
  0x85, 0xE0,                     // Report ID (0xE0) - Device Info
  0x75, 0x08,                     // Report Size (8)
  0x95, 0x3F,                     // Report Count (63)
  0x09, 0x01,                     // Usage (0x01)
  0xB1, 0x02,                     // Feature (Data,Var,Abs)
  0xC0,                           // End Collection
  0xA1, 0x02,                     // Collection (Logical)
  0x85, 0xE3,                     // Report ID (0xE3) - Commands
  0x75, 0x08,                     // Report Size (8)
  0x95, 0x3F,                     // Report Count (63)
  0x09, 0x01,                     // Usage (0x01)
  0xB1, 0x02,                     // Feature (Data,Var,Abs)
  0xC0,                           // End Collection
  0xA1, 0x02,                     // Collection (Logical)
  0x85, 0xE4,                     // Report ID (0xE4) - Key Status
  0x75, 0x08,                     // Report Size (8)
  0x95, 0x3F,                     // Report Count (63)
  0x09, 0x01,                     // Usage (0x01)
  0xB1, 0x02,                     // Feature (Data,Var,Abs)
  0xC0,                           // End Collection
  0xA1, 0x02,                     // Collection (Logical)
  0x85, 0xE5,                     // Report ID (0xE5) - Key Write
  0x75, 0x08,                     // Report Size (8)
  0x95, 0x3F,                     // Report Count (63)
  0x09, 0x01,                     // Usage (0x01)
  0xB1, 0x02,                     // Feature (Data,Var,Abs)
  0xC0,                           // End Collection
  0xA1, 0x02,                     // Collection (Logical)
  0x85, 0xE6,                     // Report ID (0xE6) - Key Clear
  0x75, 0x08,                     // Report Size (8)
  0x95, 0x3F,                     // Report Count (63)
  0x09, 0x01,                     // Usage (0x01)
  0xB1, 0x02,                     // Feature (Data,Var,Abs)
  0xC0,                           // End Collection
  0xA1, 0x02,                     // Collection (Logical)
  0x85, 0xE7,                     // Report ID (0xE7) - Input State
  0x75, 0x08,                     // Report Size (8)
  0x95, 0x3F,                     // Report Count (63)
  0x09, 0x01,                     // Usage (0x01)
  0xB1, 0x02,                     // Feature (Data,Var,Abs)
  0xC0,                           // End Collection
  0xc0,                             // END_COLLECTION
};

typedef struct TU_ATTR_PACKED {
  // uint8_t f_spn_l_btn : 1; //fake spinner left
  // uint8_t f_spn_r_btn : 1; //fake spinner right

  // uint8_t select_btn : 1;
  // uint8_t start_btn : 1;

  // uint8_t empty_btn : 4;

  // uint8_t l2_btn : 1;
  // uint8_t r2_btn : 1;
  // uint8_t l1_btn : 1;
  // uint8_t r1_btn : 1;

  // uint8_t triangle_btn : 1;
  // uint8_t circle_btn : 1;
  // uint8_t cross_btn : 1;
  // uint8_t square_btn : 1;

  uint16_t buttons;

  int8_t l_x_axis;
  int8_t l_y_axis;
  int8_t spinner_axis;
  int8_t paddle_axis;
  
  // digital direction, use the dir_* constants(enum)
  // 8 = center, 0 = up, 1 = up/right, 2 = right, 3 = right/down
  // 4 = down, 5 = down/left, 6 = left, 7 = left/up

  uint8_t direction;
} usbout_jogconmister_report_t;
