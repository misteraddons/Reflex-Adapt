#pragma once

// AUTO probe descriptor family extracted from
// output_descriptors_generic_runtime.h.

static const uint8_t desc_autodetect_report[] = {
  0x05, 0x01,        // Usage Page (Generic Desktop)
  0x09, 0x05,        // Usage (Gamepad)
  0xA1, 0x01,        // Collection (Application)
  0x15, 0x00,        //   Logical Minimum (0)
  0x25, 0x01,        //   Logical Maximum (1)
  0x35, 0x00,        //   Physical Minimum (0)
  0x45, 0x01,        //   Physical Maximum (1)

  // --- Input Report ID 1: Basic gamepad ---
  0x85, 0x01,        //   Report ID (1)
  0x75, 0x01,        //   Report Size (1)
  0x95, 0x10,        //   Report Count (16)
  0x05, 0x09,        //   Usage Page (Button)
  0x19, 0x01,        //   Usage Minimum (1)
  0x29, 0x10,        //   Usage Maximum (16)
  0x81, 0x02,        //   Input (Data,Var,Abs)

  // Hat switch (4 bits)
  0x05, 0x01,        //   Usage Page (Generic Desktop)
  0x25, 0x07,        //   Logical Maximum (7)
  0x46, 0x3B, 0x01,  //   Physical Maximum (315)
  0x75, 0x04,        //   Report Size (4)
  0x95, 0x01,        //   Report Count (1)
  0x65, 0x14,        //   Unit (Degrees)
  0x09, 0x39,        //   Usage (Hat Switch)
  0x81, 0x42,        //   Input (Data,Var,Abs,Null)

  // 4 padding bits
  0x65, 0x00,        //   Unit (None)
  0x95, 0x01,        //   Report Count (1)
  0x81, 0x01,        //   Input (Const)

  // 4 axes (X, Y, Z, Rz) - 8-bit each
  0x26, 0xFF, 0x00,  //   Logical Maximum (255)
  0x46, 0xFF, 0x00,  //   Physical Maximum (255)
  0x09, 0x30,        //   Usage (X)
  0x09, 0x31,        //   Usage (Y)
  0x09, 0x32,        //   Usage (Z)
  0x09, 0x35,        //   Usage (Rz)
  0x75, 0x08,        //   Report Size (8)
  0x95, 0x04,        //   Report Count (4)
  0x81, 0x02,        //   Input (Data,Var,Abs)

  // PS3 vendor data (matches DS3/Pokken format)
  0x06, 0x00, 0xFF,  //   Usage Page (Vendor 0xFF00)
  0x09, 0x20,        //   Usage (0x20) - 1 byte vendor input
  0x95, 0x01,        //   Report Count (1)
  0x81, 0x02,        //   Input (Data,Var,Abs)

  // PS3 Detection: Output with Usage 0x2621
  // PS3 scans HID descriptors for this usage to identify compatible controllers
  0x0A, 0x21, 0x26,  //   Usage (0x2621) - PS3 controller trigger
  0x95, 0x08,        //   Report Count (8)
  0x91, 0x02,        //   Output (Data,Var,Abs)

  // PS4 Detection: Feature Report 0x03 with Usage 0x2721
  // PS4 scans HID descriptors for this usage and sends GET_REPORT(Feature, 0x03)
  0xA1, 0x02,        //   Collection (Logical)
    0x85, 0x03,        //     Report ID (3)
    0x75, 0x08,        //     Report Size (8)
    0x95, 0x2F,        //     Report Count (47)
    0x0A, 0x21, 0x27,  //     Usage (0x2721) - PS4 config trigger
    0xB1, 0x02,        //     Feature (Data,Var,Abs)
  0xC0,              //   End Collection

  // PS3 Feature Report 0xF2 (BD address response)
  // PS3 requests this after recognizing usage 0x2621
  0xA1, 0x02,        //   Collection (Logical)
    0x85, 0xF2,        //     Report ID (0xF2)
    0x75, 0x08,        //     Report Size (8)
    0x95, 0x11,        //     Report Count (17)
    0x09, 0x01,        //     Usage (0x01)
    0xB1, 0x02,        //     Feature (Data,Var,Abs)
  0xC0,              //   End Collection

  // PS3 Feature Report 0xF5 (Master BD address response)
  0xA1, 0x02,        //   Collection (Logical)
    0x85, 0xF5,        //     Report ID (0xF5)
    0x75, 0x08,        //     Report Size (8)
    0x95, 0x08,        //     Report Count (8)
    0x09, 0x01,        //     Usage (0x01)
    0xB1, 0x02,        //     Feature (Data,Var,Abs)
  0xC0,              //   End Collection

  0xC0               // End Collection (Application)
};

// =============================================================================
// Auto-Detect: PS3 Probe HID Report Descriptor (Stage 0.5)
// =============================================================================
// Used only during AUTO "Try PS" probe stage to split PS3 vs PS4.
//
// Design goals:
// - Include the PS3 trigger usage (0x2621) and PS3 feature report IDs (0xF2/0xF5)
//   on vendor page 0xFF00 so a real PS3 console will perform its PS3-specific probes.
// - EXCLUDE the PS4 trigger usage (0x2721 / Feature Report ID 0x03) so PS3 is not
//   baited into the PS4 probe path.
//
// The input report layout intentionally matches the Stage 0 composite's basic gamepad
// report (Report ID 1, 8-byte payload) so the firmware can reuse the same dummy report
// logic during probing.
static const uint8_t desc_ps3_probe_report[] = {
  0x05, 0x01,        // Usage Page (Generic Desktop)
  0x09, 0x05,        // Usage (Gamepad)
  0xA1, 0x01,        // Collection (Application)
  0x15, 0x00,        //   Logical Minimum (0)
  0x25, 0x01,        //   Logical Maximum (1)
  0x35, 0x00,        //   Physical Minimum (0)
  0x45, 0x01,        //   Physical Maximum (1)

  // --- Input Report ID 1: Basic gamepad (8-byte payload) ---
  0x85, 0x01,        //   Report ID (1)
  0x75, 0x01,        //   Report Size (1)
  0x95, 0x10,        //   Report Count (16)
  0x05, 0x09,        //   Usage Page (Button)
  0x19, 0x01,        //   Usage Minimum (1)
  0x29, 0x10,        //   Usage Maximum (16)
  0x81, 0x02,        //   Input (Data,Var,Abs)

  // Hat switch (4 bits)
  0x05, 0x01,        //   Usage Page (Generic Desktop)
  0x25, 0x07,        //   Logical Maximum (7)
  0x46, 0x3B, 0x01,  //   Physical Maximum (315)
  0x75, 0x04,        //   Report Size (4)
  0x95, 0x01,        //   Report Count (1)
  0x65, 0x14,        //   Unit (Degrees)
  0x09, 0x39,        //   Usage (Hat Switch)
  0x81, 0x42,        //   Input (Data,Var,Abs,Null)

  // 4 padding bits
  0x65, 0x00,        //   Unit (None)
  0x95, 0x01,        //   Report Count (1)
  0x81, 0x01,        //   Input (Const)

  // 4 axes (X, Y, Z, Rz) - 8-bit each
  0x26, 0xFF, 0x00,  //   Logical Maximum (255)
  0x46, 0xFF, 0x00,  //   Physical Maximum (255)
  0x09, 0x30,        //   Usage (X)
  0x09, 0x31,        //   Usage (Y)
  0x09, 0x32,        //   Usage (Z)
  0x09, 0x35,        //   Usage (Rz)
  0x75, 0x08,        //   Report Size (8)
  0x95, 0x04,        //   Report Count (4)
  0x81, 0x02,        //   Input (Data,Var,Abs)

  // Vendor data + PS3 trigger (Vendor Page 0xFF00)
  0x06, 0x00, 0xFF,  //   Usage Page (Vendor 0xFF00)
  0x09, 0x20,        //   Usage (0x20) - 1 byte vendor input
  0x95, 0x01,        //   Report Count (1)
  0x81, 0x02,        //   Input (Data,Var,Abs)

  // PS3 Detection: Output with Usage 0x2621
  0x0A, 0x21, 0x26,  //   Usage (0x2621) - PS3 controller trigger
  0x95, 0x08,        //   Report Count (8)
  0x91, 0x02,        //   Output (Data,Var,Abs)

  // PS3 Feature Report 0xF2 (BD address response)
  0xA1, 0x02,        //   Collection (Logical)
    0x85, 0xF2,        //     Report ID (0xF2)
    0x75, 0x08,        //     Report Size (8)
    0x95, 0x11,        //     Report Count (17)
    0x09, 0x01,        //     Usage (0x01)
    0xB1, 0x02,        //     Feature (Data,Var,Abs)
  0xC0,              //   End Collection

  // PS3 Feature Report 0xF5 (Master BD address response)
  0xA1, 0x02,        //   Collection (Logical)
    0x85, 0xF5,        //     Report ID (0xF5)
    0x75, 0x08,        //     Report Size (8)
    0x95, 0x08,        //     Report Count (8)
    0x09, 0x01,        //     Usage (0x01)
    0xB1, 0x02,        //     Feature (Data,Var,Abs)
  0xC0,              //   End Collection

  0xC0               // End Collection (Application)
};














//
