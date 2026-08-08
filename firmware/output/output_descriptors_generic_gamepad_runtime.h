#pragma once

// Generic HID gamepad descriptor family extracted from
// output_descriptors_generic_hid_runtime.h.

uint8_t const desc_hidgeneric_report[] =
{
  HID_USAGE_PAGE ( HID_USAGE_PAGE_DESKTOP     ),
  HID_USAGE      ( HID_USAGE_DESKTOP_GAMEPAD  ),
  HID_COLLECTION ( HID_COLLECTION_APPLICATION ),
  HID_REPORT_ID  ( 1                          )
    /* 8 bit X, Y, Z, Rz, Rx, Ry (min 0, max 255 ) */
    HID_USAGE_PAGE     ( HID_USAGE_PAGE_DESKTOP                 ),
    HID_USAGE          ( HID_USAGE_DESKTOP_X                    ),
    HID_USAGE          ( HID_USAGE_DESKTOP_Y                    ),
    HID_USAGE          ( HID_USAGE_DESKTOP_Z                    ),
    HID_USAGE          ( HID_USAGE_DESKTOP_RZ                   ),
    HID_USAGE          ( HID_USAGE_DESKTOP_RX                   ),
    HID_USAGE          ( HID_USAGE_DESKTOP_RY                   ),
    HID_LOGICAL_MIN    ( 0                                      ),
    HID_LOGICAL_MAX_N  ( 255, 2                                 ),
    HID_REPORT_COUNT   ( 6                                      ),
    HID_REPORT_SIZE    ( 8                                      ),
    HID_INPUT          ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ),
    /* 4 bit DPad/Hat Button Map  */
    HID_USAGE_PAGE     ( HID_USAGE_PAGE_DESKTOP                 ),
    HID_USAGE          ( HID_USAGE_DESKTOP_HAT_SWITCH           ),
    HID_LOGICAL_MIN    ( 0                                      ),
    HID_LOGICAL_MAX    ( 7                                      ),
    HID_PHYSICAL_MIN   ( 0                                      ),
    HID_PHYSICAL_MAX_N ( 315, 2                                 ),
    HID_REPORT_COUNT   ( 1                                      ),
    HID_REPORT_SIZE    ( 4                                      ),
    HID_UNIT           ( 14                                     ),
    HID_INPUT          ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ),
    /* 12 bit vendor data  */
    HID_USAGE_PAGE_N   ( HID_USAGE_PAGE_VENDOR, 2               ),
    HID_USAGE          ( 1                                      ),
    HID_LOGICAL_MAX_N  ( 0xFFF, 3                               ),
    HID_REPORT_SIZE    ( 12                                     ),
    HID_INPUT          ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ),
    /* 32 bit Button Map */
    HID_USAGE_PAGE     ( HID_USAGE_PAGE_BUTTON                  ),
    HID_USAGE_MIN      ( 1                                      ),
    HID_USAGE_MAX      ( 32                                     ),
    HID_LOGICAL_MIN    ( 0                                      ),
    HID_LOGICAL_MAX    ( 1                                      ),
    HID_REPORT_COUNT   ( 32                                     ),
    HID_REPORT_SIZE    ( 1                                      ),
    HID_INPUT          ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ),
    /* Output rumble left, right */
    HID_USAGE_PAGE_N   ( HID_USAGE_PAGE_VENDOR, 2               ),
    HID_REPORT_ID      ( 2                                      )
    HID_USAGE          ( 1                                      ),
    HID_LOGICAL_MAX_N  ( 255, 2                                 ),
    HID_REPORT_COUNT   ( 2                                      ),
    HID_REPORT_SIZE    ( 8                                      ),
    HID_OUTPUT         ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ),
    /* Output player index */
    HID_REPORT_ID      ( 3                                      )
    HID_USAGE          ( 2                                      ),
    HID_REPORT_COUNT   ( 1                                      ),
    HID_OUTPUT         ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ),
  REFLEX_WEBHID_FEATURE_REPORT_DESC(),
  HID_COLLECTION_END
};

uint8_t const desc_hidgeneric_clean_report[] =
{
  HID_USAGE_PAGE ( HID_USAGE_PAGE_DESKTOP     ),
  HID_USAGE      ( HID_USAGE_DESKTOP_GAMEPAD  ),
  HID_COLLECTION ( HID_COLLECTION_APPLICATION ),
  HID_REPORT_ID  ( 1                          )
    /* 8 bit X, Y, Z, Rz, Rx, Ry (min 0, max 255 ) */
    HID_USAGE_PAGE     ( HID_USAGE_PAGE_DESKTOP                 ),
    HID_USAGE          ( HID_USAGE_DESKTOP_X                    ),
    HID_USAGE          ( HID_USAGE_DESKTOP_Y                    ),
    HID_USAGE          ( HID_USAGE_DESKTOP_Z                    ),
    HID_USAGE          ( HID_USAGE_DESKTOP_RZ                   ),
    HID_USAGE          ( HID_USAGE_DESKTOP_RX                   ),
    HID_USAGE          ( HID_USAGE_DESKTOP_RY                   ),
    HID_LOGICAL_MIN    ( 0                                      ),
    HID_LOGICAL_MAX_N  ( 255, 2                                 ),
    HID_REPORT_COUNT   ( 6                                      ),
    HID_REPORT_SIZE    ( 8                                      ),
    HID_INPUT          ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ),
    /* 4 bit DPad/Hat Button Map  */
    HID_USAGE_PAGE     ( HID_USAGE_PAGE_DESKTOP                 ),
    HID_USAGE          ( HID_USAGE_DESKTOP_HAT_SWITCH           ),
    HID_LOGICAL_MIN    ( 0                                      ),
    HID_LOGICAL_MAX    ( 7                                      ),
    HID_PHYSICAL_MIN   ( 0                                      ),
    HID_PHYSICAL_MAX_N ( 315, 2                                 ),
    HID_REPORT_COUNT   ( 1                                      ),
    HID_REPORT_SIZE    ( 4                                      ),
    HID_UNIT           ( 14                                     ),
    HID_INPUT          ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ),
    /* 12 bit vendor data  */
    HID_USAGE_PAGE_N   ( HID_USAGE_PAGE_VENDOR, 2               ),
    HID_USAGE          ( 1                                      ),
    HID_LOGICAL_MAX_N  ( 0xFFF, 3                               ),
    HID_REPORT_SIZE    ( 12                                     ),
    HID_INPUT          ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ),
    /* 32 bit Button Map */
    HID_USAGE_PAGE     ( HID_USAGE_PAGE_BUTTON                  ),
    HID_USAGE_MIN      ( 1                                      ),
    HID_USAGE_MAX      ( 32                                     ),
    HID_LOGICAL_MIN    ( 0                                      ),
    HID_LOGICAL_MAX    ( 1                                      ),
    HID_REPORT_COUNT   ( 32                                     ),
    HID_REPORT_SIZE    ( 1                                      ),
    HID_INPUT          ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ),
  HID_COLLECTION_END
};

// Sonik's generic HID probe descriptor used during AUTO Stage 0 host detection.
// Keep this byte-for-byte aligned with PicoZordXinputDEV so Windows/Xbox/PS4
// probe behavior matches the reference implementation instead of the local
// WebHID-expanded generic descriptor.

typedef struct TU_ATTR_PACKED
{
  uint8_t id;          ///< Report ID
  uint8_t x;           ///< Delta x  movement of left analog-stick (axis 0)
  uint8_t y;           ///< Delta y  movement of left analog-stick (axis 1)
  uint8_t z;           ///< Right analog-stick X on Windows axis 2
  uint8_t rz;          ///< Analog right trigger on Windows axis 5
  uint8_t rx;          ///< Right analog-stick Y on Windows axis 3
  uint8_t ry;          ///< Analog left trigger on Windows axis 4
  uint8_t hat : 4;     ///< Buttons mask for currently pressed buttons in the DPad/hat
  uint16_t vendor : 12; ///< Vendor data
//  uint8_t  connected : 1;
//  uint8_t  dpad_as_buttons : 1;
//  uint8_t  mode : 4;
//  uint8_t  : 6; // reserved
  uint32_t buttons;     ///< Buttons mask for currently pressed buttons
} usbout_hid_generic_report_t;

// =============================================================================
// Auto-Detect HID Report Descriptor
// =============================================================================
// HID-primary composite descriptor for auto-detect Stage 0.
// Combines: gamepad input (host polling), PS3 trigger (usage 0x2621),
// PS4 trigger (feature 0x03 / usage 0x2721), PS3 feature responses (0xF2, 0xF5).
// Used with generic VID/PID (0x16D0:0x1460) + Xbox 360 Security interface.
