#pragma once

#include <stddef.h>
#include <stdint.h>

#include "settings_store.h"

constexpr uint16_t GLOBAL_SETTINGS_RECORD_SIZE =
  PERSISTED_BLOCK_HEADER_SIZE + sizeof(GlobalSettingsRecord);
constexpr uint16_t PER_MODE_SETTINGS_RECORD_SIZE =
  PERSISTED_BLOCK_HEADER_SIZE + sizeof(PerModeSettingsRecord);

constexpr uint16_t SETTINGS_STORAGE_REGION_BASE = 0;
constexpr uint16_t SETTINGS_GLOBAL_RECORD_BASE = SETTINGS_STORAGE_REGION_BASE;
constexpr uint16_t SETTINGS_PER_MODE_RECORD_BASE =
  SETTINGS_GLOBAL_RECORD_BASE + (PERSISTED_AB_SLOT_COUNT * GLOBAL_SETTINGS_RECORD_SIZE);
constexpr uint16_t SETTINGS_STORAGE_REGION_SIZE =
  SETTINGS_PER_MODE_RECORD_BASE +
  (MAX_INPUT_MODES * PERSISTED_AB_SLOT_COUNT * PER_MODE_SETTINGS_RECORD_SIZE);

inline constexpr uint16_t getBootSettingsRecordAddress(uint8_t slot) {
  return SETTINGS_GLOBAL_RECORD_BASE + ((slot & 0x01u) * GLOBAL_SETTINGS_RECORD_SIZE);
}

inline constexpr uint16_t getGlobalSettingsPayloadAddress(uint8_t slot = 0) {
  return getBootSettingsRecordAddress(slot) + PERSISTED_BLOCK_HEADER_SIZE;
}

inline constexpr uint16_t getPerModeSettingsRecordBase(DeviceEnum mode) {
  return SETTINGS_PER_MODE_RECORD_BASE +
         ((uint16_t)mode * PERSISTED_AB_SLOT_COUNT * PER_MODE_SETTINGS_RECORD_SIZE);
}

inline constexpr uint16_t getPerModeSettingsRecordAddress(DeviceEnum mode, uint8_t slot) {
  return getPerModeSettingsRecordBase(mode) + ((slot & 0x01u) * PER_MODE_SETTINGS_RECORD_SIZE);
}

inline constexpr uint16_t getPerModeSettingsPayloadAddress(DeviceEnum mode, uint8_t slot) {
  return getPerModeSettingsRecordAddress(mode, slot) + PERSISTED_BLOCK_HEADER_SIZE;
}

inline constexpr uint16_t getPerModeQuickSettingsAddress(DeviceEnum mode) {
  return getPerModeSettingsPayloadAddress(mode, 0);
}

inline constexpr uint16_t getPerModeQuickSettingsHeaderAddress(DeviceEnum mode) {
  return getPerModeSettingsRecordAddress(mode, 0);
}

inline constexpr uint16_t getPerModeQuickSettingsRecordBase(DeviceEnum mode) {
  return getPerModeSettingsRecordBase(mode);
}

inline constexpr uint16_t getPerModeQuickSettingsRecordAddress(DeviceEnum mode, uint8_t slot) {
  return getPerModeSettingsRecordAddress(mode, slot);
}

inline constexpr uint16_t getPerModeTurboAddress(DeviceEnum mode) {
  return getPerModeSettingsPayloadAddress(mode, 0) +
         (uint16_t)offsetof(PerModeSettingsRecord, turbo_rates);
}

inline constexpr uint16_t getPerModeTurboHeaderAddress(DeviceEnum mode) {
  return getPerModeSettingsRecordAddress(mode, 0);
}

inline constexpr uint16_t getPerModeTurboRecordBase(DeviceEnum mode) {
  return getPerModeSettingsRecordBase(mode);
}

inline constexpr uint16_t getPerModeTurboRecordAddress(DeviceEnum mode, uint8_t slot) {
  return getPerModeSettingsRecordAddress(mode, slot);
}

inline constexpr uint16_t getPerModeRemapAddress(DeviceEnum mode) {
  return getPerModeSettingsPayloadAddress(mode, 0) +
         (uint16_t)offsetof(PerModeSettingsRecord, remaps);
}

inline constexpr uint16_t getPerModeRemapHeaderAddress(DeviceEnum mode) {
  return getPerModeSettingsRecordAddress(mode, 0);
}

inline constexpr uint16_t getPerModeRemapRecordBase(DeviceEnum mode) {
  return getPerModeSettingsRecordBase(mode);
}

inline constexpr uint16_t getPerModeRemapRecordAddress(DeviceEnum mode, uint8_t slot) {
  return getPerModeSettingsRecordAddress(mode, slot);
}
