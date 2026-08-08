#include "button_remap.h"
#include "button_chord_remap.h"
#include "classic_dual_merge_config.h"
#include "device_runtime_state.h"
#include "eeprom_helper.h"
#include "hotkey_combo.h"
#include "settings_layout.h"
#include "settings_registry.h"
#include "settings_store.h"

namespace {

GlobalSettingsRecord buildDefaultGlobalSettings() {
  GlobalSettingsRecord settings = {};
  for (uint8_t i = 0; i < static_cast<uint8_t>(SettingId::Count); ++i) {
    const SettingId id = (SettingId)i;
    if (!settingIsGlobal(id)) {
      continue;
    }
    writeRecordSettingValue(settings, id, defaultSettingValue(id, RZORD_NONE));
  }
  sanitizeGlobalSettings(settings);
  return settings;
}

void writeValidatedGlobalSettings(const GlobalSettingsRecord& settings) {
  writePersistedRecordAB(
    SETTINGS_GLOBAL_RECORD_BASE,
    GLOBAL_SETTINGS_RECORD_SIZE,
    PERSISTED_BLOCK_MAGIC_GLOBAL,
    settings
  );
}

DeviceEnum effectiveClassicDualMergeSettingsMode(DeviceEnum mode) {
  if (isConcreteModeForPerSettings(mode)) {
    return mode;
  }
  if (isConcreteModeForPerSettings(deviceMode)) {
    return deviceMode;
  }
  for (uint8_t rawMode = 0; rawMode < (uint8_t)RZORD_LAST; ++rawMode) {
    const DeviceEnum candidate = (DeviceEnum)rawMode;
    if (isConcreteModeForPerSettings(candidate)) {
      return candidate;
    }
  }
  return RZORD_NONE;
}

}  // namespace

void sanitizeGlobalSettings(GlobalSettingsRecord& settings) {
  for (uint8_t i = 0; i < static_cast<uint8_t>(SettingId::Count); ++i) {
    const SettingId id = (SettingId)i;
    if (!settingIsGlobal(id)) {
      continue;
    }
    const int32_t sanitized = sanitizeSettingValue(id, RZORD_NONE, readRecordSettingValue(settings, id));
    writeRecordSettingValue(settings, id, sanitized);
  }
}

bool readGlobalSettings(GlobalSettingsRecord& settings) {
  uint8_t slot = 0;
  uint16_t generation = 0;
  if (!readLatestPersistedRecordAB(
        SETTINGS_GLOBAL_RECORD_BASE,
        GLOBAL_SETTINGS_RECORD_SIZE,
        PERSISTED_BLOCK_MAGIC_GLOBAL,
        slot,
        generation,
        settings)) {
    settings = buildDefaultGlobalSettings();
    return false;
  }

  sanitizeGlobalSettings(settings);
  return true;
}

void writeGlobalSettings(const GlobalSettingsRecord& settings) {
  GlobalSettingsRecord sanitized = settings;
  sanitizeGlobalSettings(sanitized);
  writeValidatedGlobalSettings(sanitized);
}

int32_t loadSettingValue(SettingId id, DeviceEnum mode) {
  if (id == SettingId::ClassicDualMerge) {
    const DeviceEnum effectiveMode = effectiveClassicDualMergeSettingsMode(mode);
    return classicDualMergeEnabledForMode(effectiveMode) ? 1 : 0;
  }

  if (settingIsGlobal(id)) {
    GlobalSettingsRecord globalSettings{};
    readGlobalSettings(globalSettings);
    return readRecordSettingValue(globalSettings, id);
  }

  PerModeSettingsRecord perModeSettings{};
  readPerModeSettings(mode, perModeSettings);
  return readRecordSettingValue(perModeSettings, id);
}

void saveSettingValue(SettingId id, int32_t value, DeviceEnum mode) {
  if (id == SettingId::ClassicDualMerge) {
    const DeviceEnum effectiveMode = effectiveClassicDualMergeSettingsMode(mode);
    const int32_t sanitized = sanitizeSettingValue(id, effectiveMode, value);
    saveClassicDualMergeEnabledForMode(effectiveMode, sanitized != 0);
    return;
  }

  if (settingIsGlobal(id)) {
    GlobalSettingsRecord globalSettings{};
    readGlobalSettings(globalSettings);
    writeRecordSettingValue(
      globalSettings,
      id,
      sanitizeSettingValue(id, RZORD_NONE, value)
    );
    writeGlobalSettings(globalSettings);
    return;
  }

  if (!isConcreteModeForPerSettings(mode)) {
    return;
  }

  PerModeSettingsRecord perModeSettings{};
  readPerModeSettings(mode, perModeSettings);
  writeRecordSettingValue(
    perModeSettings,
    id,
    sanitizeSettingValue(id, mode, value)
  );
  sanitizePerModeSettings(mode, perModeSettings);
  writePerModeSettings(mode, perModeSettings);
}

void saveSystemSettingByte(SettingId id, uint8_t value) {
  saveSettingValue(id, value, deviceMode);
}

void saveSystemSettingWord(SettingId id, uint16_t value) {
  saveSettingValue(id, value, deviceMode);
}

void saveSoundSettings(uint8_t buzzerModeValue, uint16_t soundEvents) {
  GlobalSettingsRecord globalSettings{};
  readGlobalSettings(globalSettings);
  globalSettings.buzzer_mode = (uint8_t)sanitizeSettingValue(SettingId::BuzzerMode, RZORD_NONE, buzzerModeValue);
  globalSettings.sound_events = (uint16_t)sanitizeSettingValue(SettingId::SoundEvents, RZORD_NONE, soundEvents);
  writeGlobalSettings(globalSettings);
}

void saveSoundEventSettings(uint16_t soundEvents) {
  GlobalSettingsRecord globalSettings{};
  readGlobalSettings(globalSettings);
  globalSettings.sound_events = (uint16_t)sanitizeSettingValue(SettingId::SoundEvents, RZORD_NONE, soundEvents);
  writeGlobalSettings(globalSettings);
}

void saveScreensaverSettings(uint8_t screensaverModeValue, uint8_t screensaverAnimationValue) {
  GlobalSettingsRecord globalSettings{};
  readGlobalSettings(globalSettings);
  globalSettings.screensaver_mode =
    (uint8_t)sanitizeSettingValue(SettingId::ScreensaverMode, RZORD_NONE, screensaverModeValue);
  globalSettings.screensaver_animation =
    (uint8_t)sanitizeSettingValue(SettingId::ScreensaverAnimation, RZORD_NONE, screensaverAnimationValue);
  writeGlobalSettings(globalSettings);
}

void saveGunconOffsets(int8_t offsetX, int8_t offsetY) {
  if (!isConcreteModeForPerSettings(deviceMode)) {
    return;
  }

  PerModeSettingsRecord settings{};
  readPerModeSettings(deviceMode, settings);
  settings.guncon_offset_x = normalizeGunconAlignmentOffset(offsetX);
  settings.guncon_offset_y = normalizeGunconAlignmentOffset(offsetY);
  sanitizePerModeSettings(deviceMode, settings);
  writePerModeSettings(deviceMode, settings);
}

void saveButtonRemaps() {
  if (!isConcreteModeForPerSettings(deviceMode)) {
    return;
  }

  PerModeSettingsRecord settings{};
  readPerModeSettings(deviceMode, settings);
  for (uint8_t i = 0; i < REMAP_MAX_BUTTONS; ++i) {
    settings.remaps[i] = active_remaps[i];
  }
  sanitizePerModeSettings(deviceMode, settings);
  writePerModeSettings(deviceMode, settings);
}

void factoryResetSettings() {
  EepromTransaction txn;
  stageEepromRangeFill(txn, SETTINGS_STORAGE_REGION_BASE, SETTINGS_STORAGE_REGION_SIZE, 0xFF);
  txn.commit();

  writeGlobalSettings(buildDefaultGlobalSettings());
  HotkeyBindingSet hotkeys{};
  setDefaultHotkeyBindings(hotkeys);
  writeHotkeyBindings(hotkeys);
  clearButtonChordRemaps();
  #ifdef PRODUCT_CLASSIC2USB
  clearClassicDualMergeConfig();
  #endif

  for (uint8_t modeValue = 0; modeValue < MAX_INPUT_MODES; ++modeValue) {
    const DeviceEnum mode = (DeviceEnum)modeValue;
    if (!isConcreteModeForPerSettings(mode)) {
      continue;
    }
    PerModeSettingsRecord settings{};
    readPerModeSettings(mode, settings);
    sanitizePerModeSettings(mode, settings);
    writePerModeSettings(mode, settings);
  }
}
