#pragma once

#include <arduino/Adafruit_USBD_Device.h>
#include <device/usbd_pvt.h>
#include <tusb_option.h>

#include "out_xinput.h"

#define XINPUT_MULTI_CONTROLLERS 2
#define XINPUT_MULTI_SECURITY_BLOCK_LEN 15
#ifndef XINPUT_MULTI_SHARED_SECURITY_INTERFACE
#define XINPUT_MULTI_SHARED_SECURITY_INTERFACE 0
#endif
#if XINPUT_MULTI_SHARED_SECURITY_INTERFACE
#define XINPUT_MULTI_INTERFACES_PER_CONTROLLER 3
#define XINPUT_MULTI_SECURITY_DESC_LEN XINPUT_MULTI_SECURITY_BLOCK_LEN
#define XINPUT_MULTI_CONTROLLER_DESC_LEN (TUD_XINPUT_FULL_DESC_LEN - 9 - XINPUT_MULTI_SECURITY_BLOCK_LEN)
#define TUD_XINPUT_MULTI_DESC_LEN ((XINPUT_MULTI_CONTROLLER_DESC_LEN * XINPUT_MULTI_CONTROLLERS) + XINPUT_MULTI_SECURITY_BLOCK_LEN)
#else
#define XINPUT_MULTI_INTERFACES_PER_CONTROLLER 4
#define XINPUT_MULTI_SECURITY_DESC_LEN 0
#define XINPUT_MULTI_CONTROLLER_DESC_LEN (TUD_XINPUT_FULL_DESC_LEN - 9)
#define TUD_XINPUT_MULTI_DESC_LEN (XINPUT_MULTI_CONTROLLER_DESC_LEN * XINPUT_MULTI_CONTROLLERS)
#endif
#define XINPUT_MULTI_ALT_OUT_ENDPOINTS 2

typedef struct {
    uint8_t endpoint_in;
    uint8_t endpoint_out;
    uint8_t out_buffer[EPSIZE];
    uint8_t endpoint_out_alt[XINPUT_MULTI_ALT_OUT_ENDPOINTS];
    uint8_t out_alt_buffer[XINPUT_MULTI_ALT_OUT_ENDPOINTS][EPSIZE];
    uint8_t interface_base;
    uint8_t descriptor[XINPUT_MULTI_CONTROLLER_DESC_LEN];
} xinput_multi_itf_t;

struct XInputMultiDiagInfo {
    uint8_t controller_count;
    bool console_auth_observed;
    uint8_t interface_base[XINPUT_MULTI_CONTROLLERS];
    uint8_t opened_part_mask[XINPUT_MULTI_CONTROLLERS];
    uint16_t auth_request_count[XINPUT_MULTI_CONTROLLERS];
    uint16_t data_stage_count[XINPUT_MULTI_CONTROLLERS];
    uint16_t desc_21_count[XINPUT_MULTI_CONTROLLERS];
    uint16_t desc_41_count[XINPUT_MULTI_CONTROLLERS];
    uint16_t in_xfer_count[XINPUT_MULTI_CONTROLLERS];
    uint16_t out_xfer_count[XINPUT_MULTI_CONTROLLERS];
    uint16_t out_arm_attempt_count[XINPUT_MULTI_CONTROLLERS];
    uint16_t out_arm_ok_count[XINPUT_MULTI_CONTROLLERS];
    uint16_t out_arm_busy_count[XINPUT_MULTI_CONTROLLERS];
    uint16_t out_arm_fail_count[XINPUT_MULTI_CONTROLLERS];
    uint16_t capability_count[XINPUT_MULTI_CONTROLLERS];
    uint16_t serial_count;
    uint16_t control_setup_count;
    uint16_t control_out_count;
    uint16_t xfer_callback_count;
    uint16_t xfer_override_count;
    uint16_t unmatched_xfer_count;
    uint8_t last_control_request;
    uint8_t last_control_type;
    uint8_t last_control_recipient;
    uint8_t last_control_direction;
    uint8_t last_control_wIndex;
    uint8_t last_control_wValue;
    uint8_t last_xfer_endpoint;
    uint8_t last_xfer_driver;
    uint8_t last_xfer_result;
    uint8_t last_xfer_size;
    uint8_t last_out_size[XINPUT_MULTI_CONTROLLERS];
    uint8_t last_out_endpoint[XINPUT_MULTI_CONTROLLERS];
    uint8_t last_out_packet[XINPUT_MULTI_CONTROLLERS][8];
    uint8_t last_rumble_left[XINPUT_MULTI_CONTROLLERS];
    uint8_t last_rumble_right[XINPUT_MULTI_CONTROLLERS];
    uint16_t rumble_parse_count[XINPUT_MULTI_CONTROLLERS];
    uint16_t rumble_nonzero_count[XINPUT_MULTI_CONTROLLERS];
    uint8_t last_nonzero_rumble_left[XINPUT_MULTI_CONTROLLERS];
    uint8_t last_nonzero_rumble_right[XINPUT_MULTI_CONTROLLERS];
    uint8_t last_request[XINPUT_MULTI_CONTROLLERS];
    uint8_t last_stage[XINPUT_MULTI_CONTROLLERS];
    uint8_t last_wIndex[XINPUT_MULTI_CONTROLLERS];
    uint8_t last_part[XINPUT_MULTI_CONTROLLERS];
    uint8_t endpoint_in[XINPUT_MULTI_CONTROLLERS];
    uint8_t endpoint_out[XINPUT_MULTI_CONTROLLERS];
    uint8_t endpoint_out_alt[XINPUT_MULTI_CONTROLLERS][XINPUT_MULTI_ALT_OUT_ENDPOINTS];
};

bool tud_xinput_multi_ready(uint8_t itf);
void receive_xinput_multi_report(uint8_t itf);
bool send_xinput_multi_report(uint8_t itf, xinput_report_t *report);
bool xinput_multi_auth_handle_control(
    uint8_t rhport,
    uint8_t stage,
    const tusb_control_request_t *request
);
bool xinput_multi_console_auth_observed();
uint8_t xinput_multi_controller_count();
void xinput_multi_get_diag_info(XInputMultiDiagInfo *info);
uint8_t xinput_multi_target_slot_for_source_port(uint8_t source_port);
uint8_t xinput_multi_source_port_for_target_slot(uint8_t target_slot);

uint16_t xinput_multi_open(
    uint8_t rhport,
    const tusb_desc_interface_t *itf_descriptor,
    uint16_t max_length
);
bool xinput_multi_xfer_callback(
    uint8_t rhport,
    uint8_t ep_addr,
    xfer_result_t result,
    uint32_t xferred_bytes
);
bool xinput_multi_handle_xfer_complete(
    uint8_t rhport,
    uint8_t ep_addr,
    xfer_result_t result,
    uint32_t xferred_bytes,
    uint8_t driver_id,
    bool from_override
);

class Adafruit_USBD_XInputMulti : public Adafruit_USBD_Interface {
  public:
    Adafruit_USBD_XInputMulti(uint8_t interval_ms = 1, uint8_t controller_count = XINPUT_MULTI_CONTROLLERS);

    bool begin(void);
    uint8_t controllerCount() const;
    bool interfaceToSlot(uint8_t itf, uint8_t *slot) const;
    bool interfaceToSlotPart(uint8_t itf, uint8_t *slot, uint8_t *part) const;
    bool isSecurityInterface(uint8_t itf) const;
    const uint8_t *descriptorForSlot(uint8_t slot) const;
    const uint8_t *securityDescriptor() const;
    bool ready(uint8_t itf);
    bool sendReport(uint8_t itf, xinput_report_t *report);

    virtual uint16_t getInterfaceDescriptor(uint8_t itfnum, uint8_t *buf, uint16_t bufsize);

  private:
    uint8_t _interval_ms;
    uint8_t _controller_count;
    uint8_t _itf_base = 0xFF;
    uint8_t _security_itfnum = 0xFF;
    xinput_multi_itf_t interfaces[XINPUT_MULTI_CONTROLLERS] = {};
    uint8_t security_descriptor[XINPUT_MULTI_SECURITY_BLOCK_LEN] = {};

    friend bool tud_xinput_multi_ready(uint8_t itf);
    friend void receive_xinput_multi_report(uint8_t itf);
    friend bool send_xinput_multi_report(uint8_t itf, xinput_report_t *report);
    friend void xinput_multi_get_diag_info(XInputMultiDiagInfo *info);
    friend uint16_t xinput_multi_open(
        uint8_t rhport,
        const tusb_desc_interface_t *itf_descriptor,
        uint16_t max_length
    );
    friend bool xinput_multi_xfer_callback(
        uint8_t rhport,
        uint8_t ep_addr,
        xfer_result_t result,
        uint32_t xferred_bytes
    );
    friend bool xinput_multi_handle_xfer_complete(
        uint8_t rhport,
        uint8_t ep_addr,
        xfer_result_t result,
        uint32_t xferred_bytes,
        uint8_t driver_id,
        bool from_override
    );
    friend bool xinput_multi_control_xfer_callback(
        uint8_t rhport,
        uint8_t stage,
        const tusb_control_request_t *request
    );
};

#ifdef __cplusplus
extern "C" {
#endif
  const usbd_class_driver_t *xinput_multi_get_driver();
#ifdef __cplusplus
}
#endif
