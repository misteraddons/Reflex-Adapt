#include "../../../product_config.h"

#include <string.h>

#include "../../../core/controller_frame_state.h"
#include "../../../core/device_runtime_state.h"
#include "../../../firmware_build_info.h"
#include "../../../input/autodetect/input_autodetect_runtime_state.h"
#include "../../../menu/menu_input_mode.h"
#include "../../../menu/menu_mode_labels.h"
#include "../../../menu/menu_mode_state.h"
#include "../../../platform/webhid_runtime.h"
#include "../../auth/auth_storage.h"
#include "../../runtime/output_boot_bridge.h"
#include "../../output_runtime_state.h"
#include "webhid_basic_reports.h"
#include "webhid_input_modes.h"
#include "webhid_protocol.h"

extern uint8_t auth_key_status;

namespace {

constexpr uint8_t kInputCatalogPageEntryCount = 2;
constexpr uint8_t kInputCatalogEntrySize = 14;
constexpr uint8_t kInputCatalogNameSize = kInputCatalogEntrySize - 2;
constexpr uint8_t kOutputCatalogPageEntryCount = 1;
constexpr uint8_t kOutputCatalogEntrySize = 18;
constexpr uint8_t kOutputCatalogNameSize = kOutputCatalogEntrySize - 2;
constexpr uint8_t kInputCatalogOffset = 35;

uint8_t webhid_input_mode_catalog_page = 0;
uint8_t webhid_mode_catalog_kind = 0;

uint8_t countSelectableInputModes() {
  uint8_t count = 0;
  for (uint8_t raw = 1; raw < (uint8_t)RZORD_LAST; ++raw) {
    const DeviceEnum mode = (DeviceEnum)raw;
    if (!should_hide_input_mode(mode)) {
      count++;
    }
  }
  return count;
}

bool getSelectableInputModeAt(uint8_t index, DeviceEnum* outMode) {
  if (!outMode) {
    return false;
  }

  uint8_t seen = 0;
  for (uint8_t raw = 1; raw < (uint8_t)RZORD_LAST; ++raw) {
    const DeviceEnum mode = (DeviceEnum)raw;
    if (should_hide_input_mode(mode)) {
      continue;
    }
    if (seen == index) {
      *outMode = mode;
      return true;
    }
    seen++;
  }
  return false;
}

void writeInputCatalogEntry(uint8_t* buffer, uint8_t entry, DeviceEnum mode) {
  const uint8_t offset = kInputCatalogOffset + entry * kInputCatalogEntrySize;
  buffer[offset + 0] = webhid_input_mode_from_device(mode);
  buffer[offset + 1] = should_hide_input_mode(mode) ? 1 : 0;
  strncpy((char*)&buffer[offset + 2], getInputModeName(mode), kInputCatalogNameSize);
}

outputMode_t firstVisibleOutputMode() {
  return OUTPUT_AUTO;
}

uint8_t countSelectableOutputModes() {
  uint8_t count = 0;
  outputMode_t mode = firstVisibleOutputMode();
  for (uint8_t attempts = 0; attempts < (uint8_t)OUTPUT_LAST; ++attempts) {
    if (!should_hide_output_mode(mode)) {
      count++;
    }
    mode = cycle_visible_output_mode(mode, true);
    if (mode == firstVisibleOutputMode()) {
      break;
    }
  }
  return count;
}

bool getSelectableOutputModeAt(uint8_t index, outputMode_t* outMode) {
  if (!outMode) {
    return false;
  }

  uint8_t seen = 0;
  outputMode_t mode = firstVisibleOutputMode();
  for (uint8_t attempts = 0; attempts < (uint8_t)OUTPUT_LAST; ++attempts) {
    if (!should_hide_output_mode(mode)) {
      if (seen == index) {
        *outMode = mode;
        return true;
      }
      seen++;
    }
    mode = cycle_visible_output_mode(mode, true);
    if (mode == firstVisibleOutputMode()) {
      break;
    }
  }
  return false;
}

void writeOutputCatalogEntry(uint8_t* buffer, uint8_t entry, outputMode_t mode) {
  const uint8_t offset = kInputCatalogOffset + entry * kOutputCatalogEntrySize;
  buffer[offset + 0] = (uint8_t)mode;
  buffer[offset + 1] = should_hide_output_mode(mode) ? 1 : 0;
  const char* name = getOutputShortName(mode);
  if (strlen(name) >= kOutputCatalogNameSize) {
    name = getOutputCompactName(mode);
  }
  strncpy((char*)&buffer[offset + 2], name, kOutputCatalogNameSize);
}

}  // namespace

void webhid_set_input_mode_report_page(uint8_t page, uint8_t catalog_kind) {
  webhid_input_mode_catalog_page = page;
  webhid_mode_catalog_kind = (catalog_kind == 1) ? 1 : 0;
}

uint16_t webhid_get_device_info_report(uint8_t* buffer) {
  WebHIDDeviceInfo info = {};
  info.magic = WEBHID_MAGIC_BYTE;
  info.protocol_version = 2;
  info.firmware_major = FIRMWARE_VERSION_MAJOR;
  info.firmware_minor = FIRMWARE_VERSION_MINOR;
  info.firmware_patch = FIRMWARE_VERSION_PATCH;
  info.device_type = webhid_input_mode_from_device((DeviceEnum)webhid_cached_device_mode);
  info.output_mode = configuredOutputMode;
  info.max_players = max_devices;
  info.config_size_lo = 0;
  info.config_size_hi = 0;
  strncpy(info.controller_name, controllerFrameConst(0).controller_type_name, sizeof(info.controller_name) - 1);
  strncpy(info.device_serial, "REFLEX-001", sizeof(info.device_serial) - 1);
  info.debug_report_count_lo = webhid_get_report_count & 0xFF;
  info.debug_report_count_hi = (webhid_get_report_count >> 8) & 0xFF;
  info.debug_last_report_id = webhid_last_report_id;
  info.debug_last_report_type = webhid_last_report_type;
#ifdef ENABLE_INPUT_AUTODETECT
  info.reserved[0] = input_autodetect_last_ms & 0xFF;
  info.reserved[1] = (input_autodetect_last_ms >> 8) & 0xFF;
  info.reserved[2] = input_autodetect_last_flags;
  info.reserved[3] = input_autodetect_last_port;
#endif
  info.reserved[4] = output_autodetect_last_ms & 0xFF;
  info.reserved[5] = (output_autodetect_last_ms >> 8) & 0xFF;
  info.reserved[6] = output_autodetect_last_flags;
  info.reserved[7] = (uint8_t)autoDetectState;

  memcpy(buffer, &info, sizeof(info));
  return WEBHID_REPORT_SIZE;
}

uint16_t webhid_get_key_status_report(uint8_t* buffer) {
  buffer[0] = WEBHID_MAGIC_BYTE;
  buffer[1] = auth_key_status;
  buffer[2] = AUTH_KEY_PS4_SIZE & 0xFF;
  buffer[3] = (AUTH_KEY_PS4_SIZE >> 8) & 0xFF;
  buffer[4] = AUTH_KEY_XB360_SIZE & 0xFF;
  buffer[5] = (AUTH_KEY_XB360_SIZE >> 8) & 0xFF;
  buffer[6] = authStorageLastUploadStatus();
  buffer[7] = 0;
  const uint32_t lastUploadCrc = authStorageLastUploadCrc32();
  buffer[8] = lastUploadCrc & 0xFF;
  buffer[9] = (lastUploadCrc >> 8) & 0xFF;
  buffer[10] = (lastUploadCrc >> 16) & 0xFF;
  buffer[11] = (lastUploadCrc >> 24) & 0xFF;
  const AuthStorageDiagnostics diag = authStorageLastDiagnostics();
  buffer[12] = diag.serial_region_state;
  buffer[13] = diag.received_chunk_count;
  buffer[14] = diag.total_chunk_count;
  buffer[15] = diag.validation_status;
  buffer[16] = diag.first_missing_offset & 0xFF;
  buffer[17] = (diag.first_missing_offset >> 8) & 0xFF;
  buffer[18] = diag.target_payload_address & 0xFF;
  buffer[19] = (diag.target_payload_address >> 8) & 0xFF;
  return WEBHID_REPORT_SIZE;
}

uint16_t webhid_get_input_mode_report(uint8_t* buffer) {
  buffer[0] = WEBHID_MAGIC_BYTE;
  buffer[1] = webhid_input_mode_from_device(deviceMode);
  buffer[2] = webhid_input_mode_from_device(savedDeviceMode);

  uint8_t connected = 0;
  for (uint8_t i = 0; i < max_devices; i++) {
    if (controllerFrameConst(i).connected) connected++;
  }
  buffer[3] = connected;

  strncpy((char*)&buffer[4], controllerFrameConst(0).controller_type_name, 12);
  buffer[16] = controllerFrameConst(0).HAS_ANALOG_STICK_MAIN ? 1 : 0;
  buffer[17] = controllerFrameConst(0).HAS_ANALOG_TRIGGERS ? 1 : 0;
  buffer[18] = (rumble_level > 0) ? 1 : 0;

  strncpy((char*)&buffer[19], getInputModeName(deviceMode), 12);

  const bool outputCatalog = (webhid_mode_catalog_kind == 1);
  const uint8_t selectableCount = outputCatalog
    ? countSelectableOutputModes()
    : countSelectableInputModes();
  const uint8_t pageEntryCount = outputCatalog
    ? kOutputCatalogPageEntryCount
    : kInputCatalogPageEntryCount;
  const uint8_t pageStart = webhid_input_mode_catalog_page * pageEntryCount;
  buffer[30] = webhid_mode_catalog_kind;
  buffer[31] = outputCatalog ? kOutputCatalogEntrySize : kInputCatalogEntrySize;
  buffer[32] = selectableCount;
  buffer[33] = webhid_input_mode_catalog_page;
  uint8_t entries = 0;
  for (; entries < pageEntryCount; ++entries) {
    if (outputCatalog) {
      outputMode_t mode = OUTPUT_MISTER;
      if (!getSelectableOutputModeAt(pageStart + entries, &mode)) {
        break;
      }
      writeOutputCatalogEntry(buffer, entries, mode);
    } else {
      DeviceEnum mode = RZORD_NONE;
      if (!getSelectableInputModeAt(pageStart + entries, &mode)) {
        break;
      }
      writeInputCatalogEntry(buffer, entries, mode);
    }
  }
  buffer[34] = entries;

  return WEBHID_REPORT_SIZE;
}
