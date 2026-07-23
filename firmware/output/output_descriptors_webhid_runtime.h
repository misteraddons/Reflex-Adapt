#pragma once

// Shared WebHID feature-report block reused by USB descriptors that expose the
// typed Reflex configuration interface.

#define REFLEX_WEBHID_FEATURE_REPORT_DESC() \
  /* WebHID Configuration Reports (0xE0-0xEF) */ \
  /* Set explicit logical range for feature reports */ \
  0x15, 0x00,        /*   Logical Minimum (0) */ \
  0x26, 0xFF, 0x00,  /*   Logical Maximum (255) */ \
  0x85, 0xE0,        /*   Report ID (0xE0) - Device Info */ \
  0x09, 0x10,        /*   Usage (0x10) */ \
  0x75, 0x08,        /*   Report Size (8) */ \
  0x95, 0x3F,        /*   Report Count (63) */ \
  0xB1, 0x02,        /*   Feature (Data,Var,Abs) */ \
  0x85, 0xE3,        /*   Report ID (0xE3) - Commands */ \
  0x09, 0x13,        /*   Usage (0x13) */ \
  0x75, 0x08,        /*   Report Size (8) */ \
  0x95, 0x3F,        /*   Report Count (63) */ \
  0xB1, 0x02,        /*   Feature (Data,Var,Abs) */ \
  0x85, 0xE4,        /*   Report ID (0xE4) - Key Status */ \
  0x09, 0x14,        /*   Usage (0x14) */ \
  0x75, 0x08,        /*   Report Size (8) */ \
  0x95, 0x3F,        /*   Report Count (63) */ \
  0xB1, 0x02,        /*   Feature (Data,Var,Abs) */ \
  0x85, 0xE5,        /*   Report ID (0xE5) - Key Write */ \
  0x09, 0x15,        /*   Usage (0x15) */ \
  0x75, 0x08,        /*   Report Size (8) */ \
  0x95, 0x3F,        /*   Report Count (63) */ \
  0xB1, 0x02,        /*   Feature (Data,Var,Abs) */ \
  0x85, 0xE6,        /*   Report ID (0xE6) - Key Clear */ \
  0x09, 0x16,        /*   Usage (0x16) */ \
  0x75, 0x08,        /*   Report Size (8) */ \
  0x95, 0x3F,        /*   Report Count (63) */ \
  0xB1, 0x02,        /*   Feature (Data,Var,Abs) */ \
  0x85, 0xE7,        /*   Report ID (0xE7) - Input State */ \
  0x09, 0x17,        /*   Usage (0x17) */ \
  0x75, 0x08,        /*   Report Size (8) */ \
  0x95, 0x3F,        /*   Report Count (63) */ \
  0xB1, 0x02,        /*   Feature (Data,Var,Abs) */ \
  0x85, 0xE8,        /*   Report ID (0xE8) - GPIO State */ \
  0x09, 0x18,        /*   Usage (0x18) */ \
  0x75, 0x08,        /*   Report Size (8) */ \
  0x95, 0x3F,        /*   Report Count (63) */ \
  0xB1, 0x02,        /*   Feature (Data,Var,Abs) */ \
  0x85, 0xE9,        /*   Report ID (0xE9) - Raw Data */ \
  0x09, 0x19,        /*   Usage (0x19) */ \
  0x75, 0x08,        /*   Report Size (8) */ \
  0x95, 0x3F,        /*   Report Count (63) */ \
  0xB1, 0x02,        /*   Feature (Data,Var,Abs) */ \
  0x85, 0xEA,        /*   Report ID (0xEA) - Settings */ \
  0x09, 0x1A,        /*   Usage (0x1A) */ \
  0x75, 0x08,        /*   Report Size (8) */ \
  0x95, 0x3F,        /*   Report Count (63) */ \
  0xB1, 0x02,        /*   Feature (Data,Var,Abs) */ \
  0x85, 0xEB,        /*   Report ID (0xEB) - Input Mode */ \
  0x09, 0x1B,        /*   Usage (0x1B) */ \
  0x75, 0x08,        /*   Report Size (8) */ \
  0x95, 0x3F,        /*   Report Count (63) */ \
  0xB1, 0x02,        /*   Feature (Data,Var,Abs) */ \
  0x85, 0xEC,        /*   Report ID (0xEC) - Turbo Settings */ \
  0x09, 0x1C,        /*   Usage (0x1C) */ \
  0x75, 0x08,        /*   Report Size (8) */ \
  0x95, 0x3F,        /*   Report Count (63) */ \
  0xB1, 0x02,        /*   Feature (Data,Var,Abs) */ \
  0x85, 0xED,        /*   Report ID (0xED) - Button Remap */ \
  0x09, 0x1D,        /*   Usage (0x1D) */ \
  0x75, 0x08,        /*   Report Size (8) */ \
  0x95, 0x3F,        /*   Report Count (63) */ \
  0xB1, 0x02,        /*   Feature (Data,Var,Abs) */ \
  0x85, 0xEE,        /*   Report ID (0xEE) - Input History */ \
  0x09, 0x1E,        /*   Usage (0x1E) */ \
  0x75, 0x08,        /*   Report Size (8) */ \
  0x95, 0x3F,        /*   Report Count (63) */ \
  0xB1, 0x02,        /*   Feature (Data,Var,Abs) */ \
  0x85, 0xEF,        /*   Report ID (0xEF) - Button Stats */ \
  0x09, 0x1F,        /*   Usage (0x1F) */ \
  0x75, 0x08,        /*   Report Size (8) */ \
  0x95, 0x3F,        /*   Report Count (63) */ \
  0xB1, 0x02        /*   Feature (Data,Var,Abs) */
