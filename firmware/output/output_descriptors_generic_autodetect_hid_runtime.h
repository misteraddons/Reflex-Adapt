#pragma once

// Generic HID autodetect descriptor family extracted from
// output_descriptors_generic_hid_runtime.h.

uint8_t const desc_hidgeneric_autodetect_report[] =
{
  0x05, 0x01,        // Usage Page (Generic Desktop)
  0x09, 0x05,        // Usage (Gamepad)
  0xA1, 0x01,        // Collection (Application)
  0x15, 0x00,        //   Logical Minimum (0)
  0x25, 0x01,        //   Logical Maximum (1)
  0x35, 0x00,        //   Physical Minimum (0)
  0x45, 0x01,        //   Physical Maximum (1)

  // Input Report ID 1: basic gamepad.
  0x85, 0x01,        //   Report ID (1)
  0x75, 0x01,        //   Report Size (1)
  0x95, 0x10,        //   Report Count (16)
  0x05, 0x09,        //   Usage Page (Button)
  0x19, 0x01,        //   Usage Minimum (1)
  0x29, 0x10,        //   Usage Maximum (16)
  0x81, 0x02,        //   Input (Data,Var,Abs)

  // Hat switch (4 bits).
  0x05, 0x01,        //   Usage Page (Generic Desktop)
  0x25, 0x07,        //   Logical Maximum (7)
  0x46, 0x3B, 0x01,  //   Physical Maximum (315)
  0x75, 0x04,        //   Report Size (4)
  0x95, 0x01,        //   Report Count (1)
  0x65, 0x14,        //   Unit (Degrees)
  0x09, 0x39,        //   Usage (Hat Switch)
  0x81, 0x42,        //   Input (Data,Var,Abs,Null)

  // 4 padding bits.
  0x65, 0x00,        //   Unit (None)
  0x95, 0x01,        //   Report Count (1)
  0x81, 0x01,        //   Input (Const)

  // 4 axes (X, Y, Z, Rz), 8-bit each.
  0x26, 0xFF, 0x00,  //   Logical Maximum (255)
  0x46, 0xFF, 0x00,  //   Physical Maximum (255)
  0x09, 0x30,        //   Usage (X)
  0x09, 0x31,        //   Usage (Y)
  0x09, 0x32,        //   Usage (Z)
  0x09, 0x35,        //   Usage (Rz)
  0x75, 0x08,        //   Report Size (8)
  0x95, 0x04,        //   Report Count (4)
  0x81, 0x02,        //   Input (Data,Var,Abs)

  // PS3 vendor data and output trigger.
  0x06, 0x00, 0xFF,  //   Usage Page (Vendor 0xFF00)
  0x09, 0x20,        //   Usage (0x20)
  0x95, 0x01,        //   Report Count (1)
  0x81, 0x02,        //   Input (Data,Var,Abs)
  0x0A, 0x21, 0x26,  //   Usage (0x2621)
  0x95, 0x08,        //   Report Count (8)
  0x91, 0x02,        //   Output (Data,Var,Abs)

  // PS4 feature report trigger.
  0xA1, 0x02,        //   Collection (Logical)
    0x85, 0x03,      //     Report ID (3)
    0x75, 0x08,      //     Report Size (8)
    0x95, 0x2F,      //     Report Count (47)
    0x0A, 0x21, 0x27,//     Usage (0x2721)
    0xB1, 0x02,      //     Feature (Data,Var,Abs)
  0xC0,              //   End Collection

  // PS3 feature reports.
  0xA1, 0x02,
    0x85, 0xF2,
    0x75, 0x08,
    0x95, 0x11,
    0x09, 0x01,
    0xB1, 0x02,
  0xC0,
  0xA1, 0x02,
    0x85, 0xF5,
    0x75, 0x08,
    0x95, 0x08,
    0x09, 0x01,
    0xB1, 0x02,
  0xC0,

  0xC0               // End Collection (Application)
};

uint8_t const desc_hidgeneric_autodetect_donor_report[] =
{
  // Donor host_detect_reflex2 generic probe shape: small gamepad plus PS4
  // feature trigger. Used by switchdiag to compare dock behavior byte-for-byte.
  0x05, 0x01,        // Usage Page (Generic Desktop)
  0x09, 0x05,        // Usage (Gamepad)
  0xA1, 0x01,        // Collection (Application)
  0x85, 0x01,        //   Report ID (1)
  0x15, 0x00,        //   Logical Minimum (0)
  0x26, 0xFF, 0x00,  //   Logical Maximum (255)
  0x95, 0x02,        //   Report Count (2)
  0x75, 0x08,        //   Report Size (8)
  0x09, 0x30,        //   Usage (X)
  0x09, 0x31,        //   Usage (Y)
  0x81, 0x02,        //   Input (Data,Var,Abs)
  0x05, 0x09,        //   Usage Page (Button)
  0x19, 0x01,        //   Usage Minimum (1)
  0x29, 0x08,        //   Usage Maximum (8)
  0x15, 0x00,        //   Logical Minimum (0)
  0x25, 0x01,        //   Logical Maximum (1)
  0x95, 0x08,        //   Report Count (8)
  0x75, 0x01,        //   Report Size (1)
  0x81, 0x02,        //   Input (Data,Var,Abs)
  0x06, 0x00, 0xFF,  //   Usage Page (Vendor 0xFF00)
  0x85, 0x03,        //   Report ID (3)
  0x0A, 0x21, 0x27,  //   Usage (0x2721)
  0x75, 0x08,        //   Report Size (8)
  0x95, 0x2F,        //   Report Count (47)
  0xB1, 0x02,        //   Feature (Data,Var,Abs)
  0xC0               // End Collection (Application)
};
