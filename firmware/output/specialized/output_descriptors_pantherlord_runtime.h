#pragma once
// PantherLord PS3 descriptor family extracted from
// output_descriptors_specialized_runtime.h.
static const uint8_t pantherlord_report_descriptor[] =
{
  0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
  0x09, 0x04,        // Usage (Joystick)
  0xA1, 0x01,        // Collection (Application)
  0x85, 0x01,        //   Report ID (1)
  0xA1, 0x02,        //   Collection (Logical)
  0x75, 0x08,        //     Report Size (8)
  0x95, 0x04,        //     Report Count (4)
  0x15, 0x00,        //     Logical Minimum (0)
  0x26, 0xFF, 0x00,  //     Logical Maximum (255)
  0x35, 0x00,        //     Physical Minimum (0)
  0x46, 0xFF, 0x00,  //     Physical Maximum (255)
  0x09, 0x32,        //     Usage (Z)
  0x09, 0x35,        //     Usage (Rz)
  0x09, 0x30,        //     Usage (X)
  0x09, 0x31,        //     Usage (Y)
  0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0x75, 0x04,        //     Report Size (4)
  0x95, 0x01,        //     Report Count (1)
  0x25, 0x07,        //     Logical Maximum (7)
  0x46, 0x3B, 0x01,  //     Physical Maximum (315)
  0x65, 0x14,        //     Unit (System: English Rotation, Length: Centimeter)
  0x09, 0x39,        //     Usage (Hat switch)
  0x81, 0x42,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,Null State)
  0x65, 0x00,        //     Unit (None)
  0x75, 0x01,        //     Report Size (1)
  0x95, 0x0C,        //     Report Count (12)
  0x25, 0x01,        //     Logical Maximum (1)
  0x45, 0x01,        //     Physical Maximum (1)
  0x05, 0x09,        //     Usage Page (Button)
  0x19, 0x01,        //     Usage Minimum (0x01)
  0x29, 0x0C,        //     Usage Maximum (0x0C)
  0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0x06, 0x00, 0xFF,  //     Usage Page (Vendor Defined 0xFF00)
  0x75, 0x01,        //     Report Size (1)
  0x95, 0x08,        //     Report Count (8)
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

//  0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
//  0x09, 0x04,        // Usage (Joystick)
//  0xA1, 0x01,        // Collection (Application)
//  0x85, 0x02,        //   Report ID (2)
//  0xA1, 0x02,        //   Collection (Logical)
//  0x75, 0x08,        //     Report Size (8)
//  0x95, 0x04,        //     Report Count (4)
//  0x15, 0x00,        //     Logical Minimum (0)
//  0x26, 0xFF, 0x00,  //     Logical Maximum (255)
//  0x35, 0x00,        //     Physical Minimum (0)
//  0x46, 0xFF, 0x00,  //     Physical Maximum (255)
//  0x09, 0x32,        //     Usage (Z)
//  0x09, 0x35,        //     Usage (Rz)
//  0x09, 0x30,        //     Usage (X)
//  0x09, 0x31,        //     Usage (Y)
//  0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
//  0x75, 0x04,        //     Report Size (4)
//  0x95, 0x01,        //     Report Count (1)
//  0x25, 0x07,        //     Logical Maximum (7)
//  0x46, 0x3B, 0x01,  //     Physical Maximum (315)
//  0x65, 0x14,        //     Unit (System: English Rotation, Length: Centimeter)
//  0x09, 0x39,        //     Usage (Hat switch)
//  0x81, 0x42,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,Null State)
//  0x65, 0x00,        //     Unit (None)
//  0x75, 0x01,        //     Report Size (1)
//  0x95, 0x0C,        //     Report Count (12)
//  0x25, 0x01,        //     Logical Maximum (1)
//  0x45, 0x01,        //     Physical Maximum (1)
//  0x05, 0x09,        //     Usage Page (Button)
//  0x19, 0x01,        //     Usage Minimum (0x01)
//  0x29, 0x0C,        //     Usage Maximum (0x0C)
//  0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
//  0x06, 0x00, 0xFF,  //     Usage Page (Vendor Defined 0xFF00)
//  0x75, 0x01,        //     Report Size (1)
//  0x95, 0x08,        //     Report Count (8)
//  0x25, 0x01,        //     Logical Maximum (1)
//  0x45, 0x01,        //     Physical Maximum (1)
//  0x09, 0x01,        //     Usage (0x01)
//  0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
//  0xC0,              //   End Collection
//  0xA1, 0x02,        //   Collection (Logical)
//  0x75, 0x08,        //     Report Size (8)
//  0x95, 0x04,        //     Report Count (4)
//  0x46, 0xFF, 0x00,  //     Physical Maximum (255)
//  0x26, 0xFF, 0x00,  //     Logical Maximum (255)
//  0x09, 0x02,        //     Usage (0x02)
//  0x91, 0x02,        //     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
//  0xC0,              //   End Collection
//  0xC0,              // End Collection
};


typedef struct TU_ATTR_PACKED {
  uint8_t report_id; // 01 or 02
  
  uint8_t ry; //(Z)
  uint8_t rx; //(RZ)
  
  uint8_t lx; //(X)
  uint8_t ly; //(Y)
  
  uint8_t hat : 4;
  uint8_t triangle : 1;
  uint8_t circle : 1;
  uint8_t cross : 1;
  uint8_t square : 1;

  uint8_t L2 : 1;
  uint8_t R2 : 1;
  uint8_t L1 : 1;
  uint8_t R1 : 1;
  uint8_t select : 1;
  uint8_t start : 1;
  uint8_t L3 : 1;
  uint8_t R3 : 1;
  
  uint8_t : 8;

} usbout_pantherlord_twinusb_report_t;







// GC Wii-U adapter (WUP-028)
