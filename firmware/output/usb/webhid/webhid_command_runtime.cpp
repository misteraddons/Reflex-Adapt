#include "../../../product_config.h"

#include <EEPROM.h>

#include "../../../firmware_platform_config.h"
#include "../../../core/controller_settings_state.h"
#include "../../../core/rumble_test_runtime.h"
#include "../../../core/settings_store.h"
#include "../../../platform/webhid_runtime.h"
#include "webhid_command_runtime.h"
#include "webhid_protocol.h"

static volatile uint8_t webhid_pending_command = 0;

void webhid_handle_command_report(const uint8_t* buffer, uint16_t bufsize) {
  if (bufsize < 2 || buffer[0] != WEBHID_MAGIC_BYTE) return;

  uint8_t cmd = buffer[1];

  if (cmd == WEBHID_CMD_RUMBLE_TEST && bufsize >= 5) {
    uint16_t durationTicks = buffer[4];
    if (bufsize >= 6) {
      durationTicks |= (uint16_t)buffer[5] << 8;
    }
    uint32_t duration_ms = (uint32_t)durationTicks * 10u;
    if (duration_ms == 0) duration_ms = 500;
    if (duration_ms > 5000) duration_ms = 5000;
    rumbleTestStart(buffer[2], buffer[3], (uint16_t)duration_ms);
  } else {
    webhid_pending_command = cmd;
  }
}

void webhid_handle_stats_report(const uint8_t* buffer, uint16_t bufsize) {
  // Clear statistics
  // Format: magic(1), command(1)
  // command: 0x01 = clear button counts, 0x02 = clear history, 0xFF = clear all
  if (bufsize < 2 || buffer[0] != WEBHID_MAGIC_BYTE) return;

  uint8_t cmd = buffer[1];
  if (cmd == 0x01 || cmd == 0xFF) {
    webhid_clear_stats();
  }
  if (cmd == 0x02 || cmd == 0xFF) {
    input_history_index = 0;
    input_history_count = 0;
  }
}

void webhid_process_commands() {
  if (webhid_pending_command == 0) return;

  uint8_t cmd = webhid_pending_command;
  webhid_pending_command = 0;

  switch (cmd) {
    case WEBHID_CMD_REBOOT:
      EEPROM.commit();
      delay(100);
      rp2040.reboot();
      break;

    case WEBHID_CMD_BOOTLOADER:
      EEPROM.commit();
      delay(100);
      rp2040.rebootToBootloader();
      break;

    case WEBHID_CMD_FACTORY_RESET:
      factoryResetSettings();
      delay(100);
      rp2040.reboot();
      break;
  }
}

bool webhid_process_rumble() {
  rumbleTestUpdate();
  return rumbleTestActive();
}
