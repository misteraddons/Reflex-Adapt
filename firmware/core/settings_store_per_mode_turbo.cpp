#include "button_remap.h"
#include "settings_store_per_mode_internal.h"
#include "turbo.h"

namespace {

bool sanitizeRemapValues(uint8_t* remaps) {
  bool changed = false;
  for (uint8_t i = 0; i < REMAP_MAX_BUTTONS; ++i) {
    if (remaps[i] >= REMAP_MAX_BUTTONS) {
      remaps[i] = i;
      changed = true;
    }
  }
  return changed;
}

PerModeSettingsRecord loadSettingsOrDefault(DeviceEnum mode, bool* found = nullptr) {
  PerModeSettingsRecord settings{};
  const bool hasSettings = readPerModeSettings(mode, settings);
  if (found != nullptr) {
    *found = hasSettings;
  }
  return settings;
}

}  // namespace

bool sanitizeTurboRatesForMode(DeviceEnum mode, uint8_t* turbo_rates) {
  bool allowed[TURBO_BTN_COUNT] = {false};
  const TurboButtonConfig& config = getTurboButtonConfig(getTurboInputModeForDeviceMode(mode));
  for (uint8_t i = 0; i < config.count; ++i) {
    if (config.indices[i] < TURBO_BTN_COUNT) {
      allowed[config.indices[i]] = true;
    }
  }

  bool changed = false;
  for (uint8_t i = 0; i < TURBO_BTN_COUNT; ++i) {
    if (turbo_rates[i] >= TURBO_RATE_LAST) {
      turbo_rates[i] = TURBO_OFF;
      changed = true;
    }
    if (!allowed[i] && turbo_rates[i] != TURBO_OFF) {
      turbo_rates[i] = TURBO_OFF;
      changed = true;
    }
  }
  return changed;
}

void selectPerModeRemapSlot(DeviceEnum mode) {
  g_eeprom_remap_index = getPerModeRemapAddress(mode);
}

void saveTurboSettingsForMode(DeviceEnum mode, const uint8_t* rates) {
  if (!isConcreteModeForPerSettings(mode)) {
    return;
  }

  PerModeSettingsRecord settings = loadSettingsOrDefault(mode);
  for (uint8_t i = 0; i < TURBO_BTN_COUNT; ++i) {
    settings.turbo_rates[i] = (rates[i] < TURBO_RATE_LAST) ? rates[i] : (uint8_t)TURBO_OFF;
  }
  sanitizePerModeSettings(mode, settings);
  writePerModeSettings(mode, settings);
}

void loadStoredTurboRatesForMode(DeviceEnum mode, uint8_t* turbo_rates) {
  PerModeSettingsRecord settings = loadSettingsOrDefault(mode);
  for (uint8_t i = 0; i < TURBO_BTN_COUNT; ++i) {
    turbo_rates[i] = settings.turbo_rates[i];
  }
  sanitizeTurboRatesForMode(mode, turbo_rates);
}

bool loadTurboRatesForMode(DeviceEnum mode, uint8_t* turbo_rates) {
  bool found = false;
  PerModeSettingsRecord settings = loadSettingsOrDefault(mode, &found);

  bool changed = !found;
  for (uint8_t i = 0; i < TURBO_BTN_COUNT; ++i) {
    turbo_rates[i] = settings.turbo_rates[i];
  }
  if (sanitizeTurboRatesForMode(mode, turbo_rates)) {
    changed = true;
  }
  if (changed && isConcreteModeForPerSettings(mode)) {
    for (uint8_t i = 0; i < TURBO_BTN_COUNT; ++i) {
      settings.turbo_rates[i] = turbo_rates[i];
    }
    sanitizePerModeSettings(mode, settings);
    writePerModeSettings(mode, settings);
  }
  return changed;
}

bool loadRemapsForMode(DeviceEnum mode) {
  bool found = false;
  PerModeSettingsRecord settings = loadSettingsOrDefault(mode, &found);

  bool changed = !found;
  for (uint8_t i = 0; i < REMAP_MAX_BUTTONS; ++i) {
    active_remaps[i] = settings.remaps[i];
  }
  if (sanitizeRemapValues(active_remaps)) {
    changed = true;
  }

  if (changed && isConcreteModeForPerSettings(mode)) {
    for (uint8_t i = 0; i < REMAP_MAX_BUTTONS; ++i) {
      settings.remaps[i] = active_remaps[i];
    }
    sanitizePerModeSettings(mode, settings);
    writePerModeSettings(mode, settings);
  }
  return changed;
}
