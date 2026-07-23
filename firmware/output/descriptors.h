#pragma once

//buttons bitmask, for hotkeys
#include "../core/controller_state.h"

const uint32_t INPUT_A        = 1UL <<  0;
const uint32_t INPUT_B        = 1UL <<  1;
const uint32_t INPUT_X        = 1UL <<  2;
const uint32_t INPUT_Y        = 1UL <<  3;
const uint32_t INPUT_L1       = 1UL <<  4; // sega Z, ogXbox white
const uint32_t INPUT_R1       = 1UL <<  5; // sega C, ogXbox black
const uint32_t INPUT_L2       = 1UL <<  6;
const uint32_t INPUT_R2       = 1UL <<  7;
const uint32_t INPUT_L3       = 1UL <<  8;
const uint32_t INPUT_R3       = 1UL <<  9;
const uint32_t INPUT_START    = 1UL << 10;
const uint32_t INPUT_SELECT   = 1UL << 11;
const uint32_t INPUT_HOME     = 1UL << 12;
const uint32_t INPUT_CAPTURE  = 1UL << 13;
//const uint32_t INPUT_EXTRA : 14; //
const uint32_t INPUT_PAD_U    = 1UL << 28;
const uint32_t INPUT_PAD_D    = 1UL << 29;
const uint32_t INPUT_PAD_L    = 1UL << 30;
const uint32_t INPUT_PAD_R    = 1UL << 31;

//typedef struct TU_ATTR_PACKED {
//
//  bool connected;
//  bool frame_dirty;
//  
//  union {
//   struct TU_ATTR_PACKED {
//    uint8_t a : 1;
//    uint8_t b : 1;
//    uint8_t x : 1;
//    uint8_t y : 1;
//    uint8_t l1 : 1;
//    uint8_t r1 : 1;
//    uint8_t l2 : 1;
//    uint8_t r2 : 1;
//    uint8_t l3 : 1;
//    uint8_t r3 : 1;
//    uint8_t select : 1;
//    uint8_t start  : 1;
//    uint8_t home : 1;
//    uint8_t capture : 1;
//    uint8_t : 2;
//   };
//   uint16_t buttons;
//  };
//
//  uint8_t hat : 4;
//  uint8_t : 4; 
//
//  uint8_t lx;
//  uint8_t ly;
//  uint8_t rx;
//  uint8_t ry;
//
//  uint8_t analog_l2;
//  uint8_t analog_r2;
//} generic_report_t;








#define TUD_HID_REPORT_DESC_GAMEPAD2(...) \
  HID_USAGE_PAGE ( HID_USAGE_PAGE_DESKTOP     )                 ,\
  HID_USAGE      ( HID_USAGE_DESKTOP_GAMEPAD  )                 ,\
  HID_COLLECTION ( HID_COLLECTION_APPLICATION )                 ,\
    /* Report ID if any */\
    __VA_ARGS__ \
    /* 16 bit X, Y, Z, Rz (min -127, max 127 ) */ \
    HID_USAGE_PAGE     ( HID_USAGE_PAGE_DESKTOP                 ) ,\
    HID_USAGE          ( HID_USAGE_DESKTOP_X                    ) ,\
    HID_USAGE          ( HID_USAGE_DESKTOP_Y                    ) ,\
    HID_USAGE          ( HID_USAGE_DESKTOP_Z                    ) ,\
    HID_USAGE          ( HID_USAGE_DESKTOP_RZ                   ) ,\
    HID_LOGICAL_MIN    ( 0                                      ) ,\
    HID_LOGICAL_MAX_N  ( 65535, 3                               ) ,\
    HID_REPORT_COUNT   ( 4                                      ) ,\
    HID_REPORT_SIZE    ( 16                                     ) ,\
    HID_INPUT          ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
    /* 8 bit Rx, Ry (min 0, max 255 ) */ \
    HID_USAGE          ( HID_USAGE_DESKTOP_RX                   ) ,\
    HID_USAGE          ( HID_USAGE_DESKTOP_RY                   ) ,\
    HID_LOGICAL_MAX_N  ( 255, 2                                 ) ,\
    HID_REPORT_COUNT   ( 2                                      ) ,\
    HID_REPORT_SIZE    ( 8                                      ) ,\
    HID_INPUT          ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
    /* 4 bit DPad/Hat Button Map  */ \
    HID_USAGE_PAGE     ( HID_USAGE_PAGE_DESKTOP                 ) ,\
    HID_USAGE          ( HID_USAGE_DESKTOP_HAT_SWITCH           ) ,\
    HID_LOGICAL_MIN    ( 0                                      ) ,\
    HID_LOGICAL_MAX    ( 7                                      ) ,\
    HID_PHYSICAL_MIN   ( 0                                      ) ,\
    HID_PHYSICAL_MAX_N ( 315, 2                                 ) ,\
    HID_REPORT_COUNT   ( 1                                      ) ,\
    HID_REPORT_SIZE    ( 4                                      ) ,\
    HID_UNIT           ( 14                                     ) ,\
    HID_INPUT          ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
    /* 4 bit DPad/Hat padding  */ \
    HID_REPORT_COUNT   ( 1                                      ) ,\
    HID_UNIT           ( 0                                      ) ,\
    HID_INPUT          ( HID_CONSTANT                           ) ,\
    /* 32 bit Button Map */ \
    HID_USAGE_PAGE     ( HID_USAGE_PAGE_BUTTON                  ) ,\
    HID_USAGE_MIN      ( 1                                      ) ,\
    HID_USAGE_MAX      ( 32                                     ) ,\
    HID_LOGICAL_MIN    ( 0                                      ) ,\
    HID_LOGICAL_MAX    ( 1                                      ) ,\
    HID_REPORT_COUNT   ( 32                                     ) ,\
    HID_REPORT_SIZE    ( 1                                      ) ,\
    HID_INPUT          ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
  HID_COLLECTION_END \









// Shared by HID generic and HID mister
#define TUD_HID_REPORT_DESC_GAMEPAD_HID_GENERIC() \
  HID_USAGE_PAGE ( HID_USAGE_PAGE_DESKTOP     )                 ,\
  HID_USAGE      ( HID_USAGE_DESKTOP_GAMEPAD  )                 ,\
  HID_COLLECTION ( HID_COLLECTION_APPLICATION )                 ,\
  HID_REPORT_ID  ( 1                          )  \
    /* 8 bit X, Y, Z, Rx, Rz, Ry (min 0, max 255 ) */ \
    HID_USAGE_PAGE     ( HID_USAGE_PAGE_DESKTOP                 ) ,\
    HID_USAGE          ( HID_USAGE_DESKTOP_X                    ) ,\
    HID_USAGE          ( HID_USAGE_DESKTOP_Y                    ) ,\
    HID_USAGE          ( HID_USAGE_DESKTOP_Z                    ) ,\
    HID_USAGE          ( HID_USAGE_DESKTOP_RX                   ) ,\
    HID_USAGE          ( HID_USAGE_DESKTOP_RZ                   ) ,\
    HID_USAGE          ( HID_USAGE_DESKTOP_RY                   ) ,\
    HID_LOGICAL_MIN    ( 0                                      ) ,\
    HID_LOGICAL_MAX_N  ( 255, 2                                 ) ,\
    HID_REPORT_COUNT   ( 6                                      ) ,\
    HID_REPORT_SIZE    ( 8                                      ) ,\
    HID_INPUT          ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
    /* 4 bit DPad/Hat Button Map  */ \
    HID_USAGE_PAGE     ( HID_USAGE_PAGE_DESKTOP                 ) ,\
    HID_USAGE          ( HID_USAGE_DESKTOP_HAT_SWITCH           ) ,\
    HID_LOGICAL_MIN    ( 0                                      ) ,\
    HID_LOGICAL_MAX    ( 7                                      ) ,\
    HID_PHYSICAL_MIN   ( 0                                      ) ,\
    HID_PHYSICAL_MAX_N ( 315, 2                                 ) ,\
    HID_REPORT_COUNT   ( 1                                      ) ,\
    HID_REPORT_SIZE    ( 4                                      ) ,\
    HID_UNIT           ( 14                                     ) ,\
    HID_INPUT          ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
    /* 12 bit vendor data  */ \
    HID_USAGE_PAGE_N   ( HID_USAGE_PAGE_VENDOR, 2               ) ,\
    HID_USAGE          ( 1                                      ) ,\
    HID_LOGICAL_MAX_N  ( 0xFFF, 3                               ) ,\
    HID_REPORT_SIZE    ( 12                                     ) ,\
    HID_INPUT          ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
    /* 32 bit Button Map */ \
    HID_USAGE_PAGE     ( HID_USAGE_PAGE_BUTTON                  ) ,\
    HID_USAGE_MIN      ( 1                                      ) ,\
    HID_USAGE_MAX      ( 32                                     ) ,\
    HID_LOGICAL_MIN    ( 0                                      ) ,\
    HID_LOGICAL_MAX    ( 1                                      ) ,\
    HID_REPORT_COUNT   ( 32                                     ) ,\
    HID_REPORT_SIZE    ( 1                                      ) ,\
    HID_INPUT          ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
    /* Output rumble left, right */ \
    HID_USAGE_PAGE_N   ( HID_USAGE_PAGE_VENDOR, 2               ) ,\
    HID_REPORT_ID      ( 2                                      )  \
    HID_USAGE          ( 1                                      ) ,\
    HID_LOGICAL_MAX_N  ( 255, 2                                 ) ,\
    HID_REPORT_COUNT   ( 2                                      ) ,\
    HID_REPORT_SIZE    ( 8                                      ) ,\
    HID_OUTPUT         ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
    /* Output player index */ \
    HID_REPORT_ID      ( 3                                      )  \
    HID_USAGE          ( 2                                      ) ,\
    HID_REPORT_COUNT   ( 1                                      ) ,\
    HID_OUTPUT         ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
  HID_COLLECTION_END \

// HID report descriptor with WebHID feature reports

// Descriptor families
// Keep descriptors.h as the umbrella include so existing output code and verifiers
// can treat it as the descriptor source of truth while the actual families live
// in smaller owned headers.

#include "output_descriptors_generic_runtime.h"
#include "playstation/output_descriptors_playstation_runtime.h"
#include "specialized/output_descriptors_specialized_runtime.h"

