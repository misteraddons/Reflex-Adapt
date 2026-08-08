#pragma once

#include <stdint.h>

struct XboxOneAuthPacket {
  uint8_t command;
  uint8_t sequence;
  uint16_t length;
  uint8_t data[1024];
};

struct XboxOneAuthSidecarStatus {
  bool supported;
  bool mounted;
  bool dongle_ready;
  bool auth_completed;
  uint8_t dev_addr;
  uint8_t instance;
  uint8_t state;
  uint8_t last_command;
  uint8_t last_sequence;
  uint16_t last_length;
  uint16_t vid;
  uint16_t pid;
  uint32_t mount_count;
  uint32_t input_count;
  uint32_t sent_count;
  uint32_t queue_count;
  uint32_t queue_drop_count;
  uint32_t invalid_count;
  uint32_t console_to_dongle_count;
  uint32_t dongle_to_console_count;
};

#ifdef __cplusplus
extern "C" {
#endif

void xbone_auth_passthrough_reset();
void xbone_auth_passthrough_task();
void xbone_auth_passthrough_mount(uint8_t dev_addr, uint8_t instance, uint8_t controller_type, uint8_t subtype);
void xbone_auth_passthrough_umount(uint8_t dev_addr, uint8_t instance);
void xbone_auth_passthrough_report_received(uint8_t dev_addr, uint8_t instance, const uint8_t* report, uint16_t len);
void xbone_auth_passthrough_report_sent(uint8_t dev_addr, uint8_t instance, const uint8_t* report, uint16_t len);
bool xbone_auth_passthrough_ready();
bool xbone_auth_passthrough_authenticated();
void xbone_auth_passthrough_mark_authenticated();
bool xbone_auth_passthrough_submit_console_packet(uint8_t command, uint8_t sequence, const uint8_t* data, uint16_t len);
bool xbone_auth_passthrough_take_dongle_packet(XboxOneAuthPacket* packet);
XboxOneAuthSidecarStatus xbone_auth_passthrough_status();

#ifdef __cplusplus
}
#endif
