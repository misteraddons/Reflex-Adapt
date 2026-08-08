#pragma once

// PS3 WebHID-expanded descriptor payload extracted from
// output_descriptors_ps3_hid_runtime.h.

static const uint8_t ps3_report_descriptor[] =
{
0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
0x09, 0x04,        // Usage (Joystick)
0xA1, 0x01,        // Collection (Application)
0xA1, 0x02,        //   Collection (Logical)
0x85, 0x01,        //     Report ID (1)
0x75, 0x08,        //     Report Size (8)
0x95, 0x01,        //     Report Count (1)
0x15, 0x00,        //     Logical Minimum (0)
0x26, 0xFF, 0x00,  //     Logical Maximum (255)
0x81, 0x03,        //     Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
0x75, 0x01,        //     Report Size (1)
0x95, 0x13,        //     Report Count (19)
0x15, 0x00,        //     Logical Minimum (0)
0x25, 0x01,        //     Logical Maximum (1)
0x35, 0x00,        //     Physical Minimum (0)
0x45, 0x01,        //     Physical Maximum (1)
0x05, 0x09,        //     Usage Page (Button)
0x19, 0x01,        //     Usage Minimum (0x01)
0x29, 0x13,        //     Usage Maximum (0x13)
0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
0x75, 0x01,        //     Report Size (1)
0x95, 0x0D,        //     Report Count (13)
0x06, 0x00, 0xFF,  //     Usage Page (Vendor Defined 0xFF00)
0x81, 0x03,        //     Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
0x15, 0x00,        //     Logical Minimum (0)
0x26, 0xFF, 0x00,  //     Logical Maximum (255)
0x05, 0x01,        //     Usage Page (Generic Desktop Ctrls)
0x09, 0x01,        //     Usage (Pointer)
0xA1, 0x00,        //     Collection (Physical)
0x75, 0x08,        //       Report Size (8)
0x95, 0x04,        //       Report Count (4)
0x35, 0x00,        //       Physical Minimum (0)
0x46, 0xFF, 0x00,  //       Physical Maximum (255)
0x09, 0x30,        //       Usage (X)
0x09, 0x31,        //       Usage (Y)
0x09, 0x32,        //       Usage (Z)
0x09, 0x35,        //       Usage (Rz)
0x81, 0x02,        //       Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
0xC0,              //     End Collection
0x05, 0x01,        //     Usage Page (Generic Desktop Ctrls)
0x75, 0x08,        //     Report Size (8)
0x95, 0x27,        //     Report Count (39)
0x09, 0x01,        //     Usage (Pointer)
0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
0x75, 0x08,        //     Report Size (8)
0x95, 0x30,        //     Report Count (48)
0x09, 0x01,        //     Usage (Pointer)
0x91, 0x02,        //     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
0x75, 0x08,        //     Report Size (8)
0x95, 0x30,        //     Report Count (48)
0x09, 0x01,        //     Usage (Pointer)
0xB1, 0x02,        //     Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
0xC0,              //   End Collection
0xA1, 0x02,        //   Collection (Logical)
0x85, 0x02,        //     Report ID (2)
0x75, 0x08,        //     Report Size (8)
0x95, 0x30,        //     Report Count (48)
0x09, 0x01,        //     Usage (Pointer)
0xB1, 0x02,        //     Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
0xC0,              //   End Collection
0xA1, 0x02,        //   Collection (Logical)
0x85, 0xEE,        //     Report ID (-18)
0x75, 0x08,        //     Report Size (8)
0x95, 0x30,        //     Report Count (48)
0x09, 0x01,        //     Usage (Pointer)
0xB1, 0x02,        //     Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
0xC0,              //   End Collection
0xA1, 0x02,        //   Collection (Logical)
0x85, 0xEF,        //     Report ID (-17)
0x75, 0x08,        //     Report Size (8)
0x95, 0x30,        //     Report Count (48)
0x09, 0x01,        //     Usage (Pointer)
0xB1, 0x02,        //     Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
0xC0,              //   End Collection
// === END OF PURE PS3 DESCRIPTOR (for real PS3 console) ===
// The bytes up to here match a real DualShock 3
// PS4 detection: feature report 0x03 with Usage 0x2721 (Sony proprietary)
// PS4 scans HID descriptors for this usage and sends GET_REPORT(Feature, 0x03)
0xA1, 0x02,        //   Collection (Logical)
0x85, 0x03,        //     Report ID (3) - PS4 auth challenge
0x75, 0x08,        //     Report Size (8)
0x95, 0x2F,        //     Report Count (47)
0x0A, 0x21, 0x27,  //     Usage (0x2721) - Sony PS4 config (triggers PS4 detection)
0xB1, 0x02,        //     Feature (Data,Var,Abs)
0xC0,              //   End Collection
// WebHID Configuration Reports (0xE0-0xE6) - NOT compatible with real PS3
0xA1, 0x02,        //   Collection (Logical)
0x85, 0xE0,        //     Report ID (0xE0) - Device Info
0x75, 0x08,        //     Report Size (8)
0x95, 0x3F,        //     Report Count (63)
0x09, 0x01,        //     Usage (Pointer)
0xB1, 0x02,        //     Feature (Data,Var,Abs)
0xC0,              //   End Collection
0xA1, 0x02,        //   Collection (Logical)
0x85, 0xE3,        //     Report ID (0xE3) - Commands
0x75, 0x08,        //     Report Size (8)
0x95, 0x3F,        //     Report Count (63)
0x09, 0x01,        //     Usage (Pointer)
0xB1, 0x02,        //     Feature (Data,Var,Abs)
0xC0,              //   End Collection
0xA1, 0x02,        //   Collection (Logical)
0x85, 0xE4,        //     Report ID (0xE4) - Key Status
0x75, 0x08,        //     Report Size (8)
0x95, 0x3F,        //     Report Count (63)
0x09, 0x01,        //     Usage (Pointer)
0xB1, 0x02,        //     Feature (Data,Var,Abs)
0xC0,              //   End Collection
0xA1, 0x02,        //   Collection (Logical)
0x85, 0xE5,        //     Report ID (0xE5) - Key Write
0x75, 0x08,        //     Report Size (8)
0x95, 0x3F,        //     Report Count (63)
0x09, 0x01,        //     Usage (Pointer)
0xB1, 0x02,        //     Feature (Data,Var,Abs)
0xC0,              //   End Collection
0xA1, 0x02,        //   Collection (Logical)
0x85, 0xE6,        //     Report ID (0xE6) - Key Clear
0x75, 0x08,        //     Report Size (8)
0x95, 0x3F,        //     Report Count (63)
0x09, 0x01,        //     Usage (Pointer)
0xB1, 0x02,        //     Feature (Data,Var,Abs)
0xC0,              //   End Collection
0xA1, 0x02,        //   Collection (Logical)
0x85, 0xE7,        //     Report ID (0xE7) - Input State
0x75, 0x08,        //     Report Size (8)
0x95, 0x3F,        //     Report Count (63)
0x09, 0x01,        //     Usage (Pointer)
0xB1, 0x02,        //     Feature (Data,Var,Abs)
0xC0,              //   End Collection
0xA1, 0x02,        //   Collection (Logical)
0x85, 0xE8,        //     Report ID (0xE8) - GPIO State
0x75, 0x08,        //     Report Size (8)
0x95, 0x3F,        //     Report Count (63)
0x09, 0x01,        //     Usage (Pointer)
0xB1, 0x02,        //     Feature (Data,Var,Abs)
0xC0,              //   End Collection
0xA1, 0x02,        //   Collection (Logical)
0x85, 0xE9,        //     Report ID (0xE9) - Raw Data
0x75, 0x08,        //     Report Size (8)
0x95, 0x3F,        //     Report Count (63)
0x09, 0x01,        //     Usage (Pointer)
0xB1, 0x02,        //     Feature (Data,Var,Abs)
0xC0,              //   End Collection
0xA1, 0x02,        //   Collection (Logical)
0x85, 0xEA,        //     Report ID (0xEA) - Settings
0x75, 0x08,        //     Report Size (8)
0x95, 0x3F,        //     Report Count (63)
0x09, 0x01,        //     Usage (Pointer)
0xB1, 0x02,        //     Feature (Data,Var,Abs)
0xC0,              //   End Collection
0xA1, 0x02,        //   Collection (Logical)
0x85, 0xEB,        //     Report ID (0xEB) - Input Mode
0x75, 0x08,        //     Report Size (8)
0x95, 0x3F,        //     Report Count (63)
0x09, 0x01,        //     Usage (Pointer)
0xB1, 0x02,        //     Feature (Data,Var,Abs)
0xC0,              //   End Collection
0xA1, 0x02,        //   Collection (Logical)
0x85, 0xEC,        //     Report ID (0xEC) - Turbo Settings
0x75, 0x08,        //     Report Size (8)
0x95, 0x3F,        //     Report Count (63)
0x09, 0x01,        //     Usage (Pointer)
0xB1, 0x02,        //     Feature (Data,Var,Abs)
0xC0,              //   End Collection
0xA1, 0x02,        //   Collection (Logical)
0x85, 0xED,        //     Report ID (0xED) - Button Remap
0x75, 0x08,        //     Report Size (8)
0x95, 0x3F,        //     Report Count (63)
0x09, 0x01,        //     Usage (Pointer)
0xB1, 0x02,        //     Feature (Data,Var,Abs)
0xC0,              //   End Collection
// Note: 0xEE and 0xEF are used by PS3 protocol above, not available for WebHID
0xC0,              // End Collection
};

// Pure minimal HID gamepad descriptor - buttons, hat, sticks only (no feature reports)
// This works on real PS3 as a generic gamepad
