#pragma once

#include <stdint.h>

// Report IDs 0xE0-0xEF are reserved for WebHID configuration.
#define WEBHID_REPORT_DEVICE_INFO   0xE0
#define WEBHID_REPORT_COMMAND       0xE3
#define WEBHID_REPORT_KEY_STATUS    0xE4
#define WEBHID_REPORT_KEY_WRITE     0xE5
#define WEBHID_REPORT_KEY_CLEAR     0xE6
#define WEBHID_REPORT_INPUT_STATE   0xE7
#define WEBHID_REPORT_GPIO_STATE    0xE8
#define WEBHID_REPORT_RAW_DATA      0xE9
#define WEBHID_REPORT_SETTINGS      0xEA
#define WEBHID_REPORT_INPUT_MODE    0xEB
#define WEBHID_REPORT_TURBO         0xEC
#define WEBHID_REPORT_REMAP         0xED
#define WEBHID_REPORT_INPUT_HISTORY 0xEE
#define WEBHID_REPORT_STATS         0xEF

#define WEBHID_CMD_REBOOT           0x01
#define WEBHID_CMD_BOOTLOADER       0x02
#define WEBHID_CMD_FACTORY_RESET    0x04
#define WEBHID_CMD_RUMBLE_TEST      0x05

#define WEBHID_MAGIC_BYTE           0xAD
#define WEBHID_REPORT_SIZE          63
#define WEBHID_SETTINGS_HOTKEY_FLAGS_INDEX 44
#define WEBHID_SETTINGS_GUNCON_X_INDEX 61
#define WEBHID_SETTINGS_GUNCON_Y_INDEX 62

struct __attribute__((packed)) WebHIDDeviceInfo {
  uint8_t magic;
  uint8_t protocol_version;
  uint8_t firmware_major;
  uint8_t firmware_minor;
  uint8_t firmware_patch;
  uint8_t device_type;
  uint8_t output_mode;
  uint8_t max_players;
  uint8_t config_size_lo;
  uint8_t config_size_hi;
  char controller_name[20];
  char device_serial[20];
  uint8_t debug_report_count_lo;
  uint8_t debug_report_count_hi;
  uint8_t debug_last_report_id;
  uint8_t debug_last_report_type;
  uint8_t reserved[8];
};

static_assert(sizeof(WebHIDDeviceInfo) <= WEBHID_REPORT_SIZE,
              "WebHIDDeviceInfo must fit in one feature report");
