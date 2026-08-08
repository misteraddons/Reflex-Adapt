#pragma once

#include <stdint.h>

struct XInputAuthRuntimeState {
  bool initialized = false;
  bool authenticated = false;
  uint8_t auth_state = 0;
  uint8_t pending_request = 0;
  uint8_t id_buffer[0x1D] = {};
  uint8_t challenge_buffer[0x24] = {};
  uint8_t challenge_len = 0;
  uint8_t response_buffer[48] = {};
  uint8_t response_len = 0;
  uint8_t misc_buffer[64] = {};

  uint8_t debug_last_request = 0;
  uint8_t debug_last_stage = 0;
  uint16_t debug_request_count = 0;
  uint16_t debug_control_count = 0;
  uint16_t debug_data_stage_count = 0;
  uint8_t debug_req_81 = 0;
  uint8_t debug_req_82 = 0;
  uint8_t debug_req_83 = 0;
  uint8_t debug_req_84 = 0;
  uint8_t debug_req_85 = 0;
  uint8_t debug_req_86 = 0;
  uint8_t debug_req_87 = 0;
  uint8_t debug_req_other = 0;
  uint8_t debug_last_other_req = 0;
  uint8_t debug_last_vendor_request = 0;
  uint8_t debug_last_control_request = 0;
  uint8_t debug_last_control_type = 0;
  uint8_t debug_last_control_recipient = 0;
  uint8_t debug_last_control_desc_type = 0;
  uint8_t debug_last_control_itf = 0;
  uint8_t debug_last_wIndex = 0;
  uint8_t debug_last_wLength = 0;
  uint8_t debug_last_bad_wIndex = 0;
  uint16_t debug_process_count = 0;
  uint16_t debug_security_mismatch_count = 0;
  uint16_t debug_xfer_in_count = 0;
  uint16_t debug_xfer_out_count = 0;
  uint16_t debug_xfer_in_complete_count = 0;
  uint16_t debug_xfer_in_submit_ok_count = 0;
  uint16_t debug_xfer_in_submit_fail_count = 0;
  uint16_t debug_xfer_out_submit_ok_count = 0;
  uint16_t debug_xfer_out_submit_fail_count = 0;
  uint8_t debug_last_out_size = 0;
  uint8_t debug_last_out_packet[8] = {};
  uint8_t debug_last_rumble_left = 0;
  uint8_t debug_last_rumble_right = 0;
  uint16_t debug_reset_count = 0;
  uint8_t debug_interfaces_opened = 0;
  uint8_t debug_ep_open_ok_count = 0;
  uint8_t debug_ep_open_fail_count = 0;
  uint8_t debug_last_opened_ep = 0;
  bool debug_begin_result = false;
  uint8_t debug_standard_control_count = 0;
  uint8_t debug_vendor_control_count = 0;
  uint8_t debug_desc_21_count = 0;
  uint8_t debug_desc_41_count = 0;
};

XInputAuthRuntimeState& xinputAuthRuntimeState();
