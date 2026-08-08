#include "../../../product_config.h"

#include "../../../core/button_remap.h"
#include "../../../core/firmware_support.h"
#include "../../../core/settings_store.h"
#include "../../../core/settings_store_internal.h"
#include "webhid_protocol.h"
#include "webhid_write_reports.h"

void webhid_write_turbo_report(const uint8_t* buffer, uint16_t bufsize) {
  if (bufsize < 3 || buffer[0] != WEBHID_MAGIC_BYTE) return;

  if (buffer[1] == 0xFF && bufsize >= 2 + TURBO_BTN_COUNT) {
    uint8_t turbo_rates[TURBO_BTN_COUNT];
    for (uint8_t i = 0; i < TURBO_BTN_COUNT; ++i) {
      const uint8_t rate = buffer[2 + i];
      turbo_rates[i] = (rate < TURBO_RATE_LAST) ? rate : (uint8_t)TURBO_OFF;
    }
    turbo.setAllRates(turbo_rates);
    saveTurboSettingsForMode(deviceMode, turbo_rates);
    return;
  }

  uint8_t btn_index = buffer[1];
  uint8_t rate = buffer[2];
  if (btn_index >= TURBO_BTN_COUNT || rate >= TURBO_RATE_LAST) return;

  webhid_set_turbo_rate(btn_index, rate);
  uint8_t turbo_rates[TURBO_BTN_COUNT];
  for (uint8_t i = 0; i < TURBO_BTN_COUNT; ++i) {
    turbo_rates[i] = (uint8_t)turbo.getButtonRate((TurboButton)i);
  }
  saveTurboSettingsForMode(deviceMode, turbo_rates);
}

void webhid_write_remap_report(const uint8_t* buffer, uint16_t bufsize) {
  if (bufsize < 3 || buffer[0] != WEBHID_MAGIC_BYTE) return;

  if (buffer[1] == 0xFF && bufsize >= 2 + REMAP_MAX_BUTTONS) {
    for (uint8_t i = 0; i < REMAP_MAX_BUTTONS; i++) {
      uint8_t val = buffer[2 + i];
      active_remaps[i] = (val < REMAP_MAX_BUTTONS) ? val : i;
    }
    saveButtonRemaps();
    return;
  }

  uint8_t btn_index = buffer[1];
  uint8_t target = buffer[2];
  if (btn_index >= REMAP_MAX_BUTTONS || target >= REMAP_MAX_BUTTONS) return;

  active_remaps[btn_index] = target;
  saveButtonRemaps();
}
