#include <EEPROM.h>

#include "device_runtime_state.h"
#include "classic_dual_merge_config.h"
#include "settings_store.h"
#include "settings_store_per_mode_internal.h"
#include "turbo.h"

void loadPerModeSettings() {
  bool commit_needed = false;
  selectPerModeRemapSlot(deviceMode);
  commit_needed |= loadAndApplyPerModeSettingsForMode(deviceMode);

  uint8_t turbo_rates[TURBO_BTN_COUNT];
  commit_needed |= loadTurboRatesForMode(deviceMode, turbo_rates);
  turbo.setAllRates(turbo_rates);

  turbo.setInputMode(getTurboInputModeForDeviceMode(deviceMode));
  commit_needed |= loadRemapsForMode(deviceMode);

#ifdef PRODUCT_CLASSIC2USB
  loadClassicDualMergeConfigForMode(deviceMode);
#endif

  if (commit_needed) {
    EEPROM.commit();
  }
}
