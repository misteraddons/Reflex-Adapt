#ifndef _USB_DETECT_RUNTIME_STATE_H_
#define _USB_DETECT_RUNTIME_STATE_H_

#include <Adafruit_TinyUSB.h>
#include <stdint.h>

const uint8_t DETECT_NONE     = (0);
// Bit 0 is repurposed on this branch for PS3 autodetect. Sonik's optional Wii U
// path remains disabled here, which is why AUTO can use this slot safely.
const uint8_t DETECT_PS3      = (1UL << 0);
const uint8_t DETECT_WIIU     = DETECT_PS3;
const uint8_t DETECT_WINDOWS  = (1UL << 1);
const uint8_t DETECT_PS4      = (1UL << 2);
const uint8_t DETECT_OGXBOX   = (1UL << 3);
const uint8_t DETECT_XBOX360  = (1UL << 4);
const uint8_t DETECT_SWITCH1  = (1UL << 5);
const uint8_t DETECT_SWITCH2  = (1UL << 6);
const uint8_t DETECT_TIMEOUT  = (1UL << 7);

typedef struct TU_ATTR_PACKED {
  uint8_t any;
  uint8_t ps3_feature_mask;
  bool not_wiiu : 1;
  bool cleared_input : 1;
  bool cleared_output : 1;
  bool string_manufacturer_read : 1;
  bool hid_descriptor_report_read : 1;
  bool ms_os_string_read : 1;
  bool vendor_setup_seen : 1;
  bool ps3_feature_seen : 1;
} host_detection_t;

extern host_detection_t host_detection;
extern uint32_t host_detection_last_signal_ms;

void usb_detect_mark_result(uint8_t detect_flag);

#endif  // _USB_DETECT_RUNTIME_STATE_H_
