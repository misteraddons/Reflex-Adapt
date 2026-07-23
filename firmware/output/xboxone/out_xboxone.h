/*
 * SPDX-License-Identifier: MIT
 *
 * Xbox One/XGIP output for Reflex Adapt. Transport and auth sequencing are
 * donor-aligned with the GP2040-CE Xbox One implementation, which documents
 * Santroller/GIMX-derived XGIP behavior. Do not copy Santroller GPL source
 * here without an explicit license decision.
 */

#pragma once

#include <arduino/Adafruit_USBD_Device.h>
#include <device/usbd_pvt.h>
#include <tusb_option.h>

struct XboxOneOutputStatus {
  bool supported;
  bool mounted;
  bool ready;
  bool auth_ready;
  uint8_t state;
  uint8_t ep_in;
  uint8_t ep_out;
  uint8_t last_command;
  uint8_t last_sequence;
  uint16_t last_len;
  uint32_t out_count;
  uint32_t in_count;
  uint32_t queue_count;
  uint32_t queue_drop_count;
  uint32_t auth_console_count;
  uint32_t auth_dongle_count;
};

class Adafruit_USBD_XboxOne : public Adafruit_USBD_Interface {
 public:
  Adafruit_USBD_XboxOne(uint8_t interval_ms = 1);

  bool begin(void);
  bool ready(void);
  bool sendReport(uint8_t port);

  uint16_t getInterfaceDescriptor(uint8_t itfnum, uint8_t* buf, uint16_t bufsize) override;

 private:
  uint8_t _interval_ms;
  uint8_t _endpoint_in = 0;
  uint8_t _endpoint_out = 0;
};

#ifdef __cplusplus
extern "C" {
#endif

const usbd_class_driver_t* xboxone_get_driver();
bool xboxone_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, const tusb_control_request_t* request);
bool xboxone_send_report_for_port(uint8_t port);
XboxOneOutputStatus xboxone_output_status();

#ifdef __cplusplus
}
#endif
