#pragma once

#include <stdint.h>

#include "../output_mode.h"

#ifdef ENABLE_USB_AUTH_SERIAL_TRACE
#include <Arduino.h>
#endif

struct PsAuthDongleStatus {
  bool supported;
  bool connected;
  bool signature_ready;
  bool busy;
  uint8_t dev_addr;
  uint8_t instance;
  uint16_t vid;
  uint16_t pid;
  uint16_t last_vid;
  uint16_t last_pid;
  uint16_t last_desc_len;
  uint8_t last_report_count;
  bool last_descriptor_candidate;
  uint8_t protocol;
  uint8_t last_error;
  uint8_t state;
  uint8_t nonce_part;
  uint8_t signature_part;
  uint32_t get_report_count;
  uint32_t set_report_count;
  uint32_t sent_report_count;
  uint32_t input_report_count;
  uint8_t last_host_get_id;
  uint8_t last_host_set_id;
  uint16_t last_host_get_len;
  uint16_t last_host_set_len;
  uint8_t last_p5_f0_command;
  uint8_t last_p5_f0_detail;
  uint8_t last_dongle_queue_id;
  uint8_t last_dongle_queue_type;
  uint16_t last_dongle_queue_len;
  uint8_t last_dongle_get_done_id;
  uint16_t last_dongle_get_done_len;
  uint8_t last_dongle_set_done_id;
  uint16_t last_dongle_sent_len;
  uint16_t last_dongle_input_len;
  uint8_t ps5_raw_buttons0;
  uint8_t ps5_raw_buttons1;
  uint8_t ps5_raw_buttons2;
  uint8_t ps5_signed_buttons0;
  uint8_t ps5_signed_buttons1;
  uint8_t ps5_signed_buttons2;
  uint8_t ps5_f2_status0;
  uint8_t ps5_f2_status1;
  uint8_t ps5_f2_status2;
  uint8_t ps5_hash_flags;
  uint32_t ps5_hash_mismatch_count;
  uint32_t ps5_prepare_count;
  uint32_t ps5_host_submit_count;
  uint32_t ps5_queue_busy_count;
  uint32_t ps5_last_host_delta_us;
  uint32_t ps5_last_dongle_delta_us;
};

#ifdef __cplusplus
extern "C" {
#endif

void ps_auth_dongle_set_output_mode(outputMode_t mode);
void ps_auth_dongle_device_connected(uint8_t dev_addr,
                                     uint8_t instance,
                                     uint16_t vid,
                                     uint16_t pid,
                                     const uint8_t* report_desc,
                                     uint16_t desc_len);
void ps_auth_dongle_device_disconnected(uint8_t dev_addr, uint8_t instance);
bool ps_auth_dongle_is_auth_device(uint16_t vid, uint16_t pid);
bool ps_auth_dongle_is_auth_instance(uint8_t dev_addr, uint8_t instance);
bool ps_auth_dongle_has_provider_for_mode(outputMode_t mode);
void ps_auth_dongle_task();

uint16_t ps_auth_dongle_handle_get_report(uint8_t report_id,
                                          uint8_t* buffer,
                                          uint16_t reqlen);
bool ps_auth_dongle_handle_set_report(uint8_t report_id,
                                      const uint8_t* buffer,
                                      uint16_t reqlen);
void ps_auth_dongle_handle_get_report_response(uint8_t dev_addr,
                                               uint8_t instance,
                                               uint8_t report_id,
                                               const uint8_t* report,
                                               uint16_t len);
void ps_auth_dongle_handle_set_report_complete(uint8_t dev_addr,
                                               uint8_t instance,
                                               uint8_t report_id);
void ps_auth_dongle_handle_sent_report_response(uint8_t dev_addr,
                                                uint8_t instance,
                                                const uint8_t* report,
                                                uint16_t len);
void ps_auth_dongle_handle_input_report_received(uint8_t dev_addr,
                                                 uint8_t instance,
                                                 const uint8_t* report,
                                                 uint16_t len);
bool ps_auth_dongle_prepare_ps5_output_report(const uint8_t* report,
                                              uint16_t len,
                                              uint8_t* out,
                                              uint16_t* out_len,
                                              uint16_t out_capacity);
void ps_auth_dongle_complete_ps5_output_report();

bool ps_auth_dongle_has_provider();
PsAuthDongleStatus ps_auth_dongle_status();

#ifdef __cplusplus
}
#endif

#ifdef ENABLE_USB_AUTH_SERIAL_TRACE
void ps_auth_dongle_trace_flush(Print& out);
void ps_auth_dongle_debug_ps5_simulate(Print& out);
#endif
