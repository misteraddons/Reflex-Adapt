#pragma once

#include <stdint.h>

#include "../../core/settings_store.h"
#include "../runtime/output_boot_bridge.h"

constexpr uint16_t PERSISTED_BLOCK_MAGIC_AUTH = 0x4155; // AU

struct AuthBlobRecord {
  uint8_t data[AUTH_KEY_PS4_SIZE];
};

static_assert(sizeof(AuthBlobRecord) == AUTH_KEY_PS4_SIZE, "AuthBlobRecord size changed unexpectedly");

enum AuthUploadStatus : uint8_t {
  AUTH_UPLOAD_STATUS_NONE = 0,
  AUTH_UPLOAD_STATUS_IN_PROGRESS = 1,
  AUTH_UPLOAD_STATUS_OK = 2,
  AUTH_UPLOAD_STATUS_CLEARED = 3,
  AUTH_UPLOAD_STATUS_INCOMPLETE = 4,
  AUTH_UPLOAD_STATUS_INVALID_SERIAL = 5,
  AUTH_UPLOAD_STATUS_INVALID_SIGNATURE = 6,
  AUTH_UPLOAD_STATUS_INVALID_MODULUS = 7,
  AUTH_UPLOAD_STATUS_INVALID_EXPONENT = 8,
  AUTH_UPLOAD_STATUS_INVALID_PRIVATE = 9,
  AUTH_UPLOAD_STATUS_INVALID_CRT = 10,
};

enum AuthBlobRegionState : uint8_t {
  AUTH_BLOB_REGION_UNKNOWN = 0,
  AUTH_BLOB_REGION_MEANINGFUL = 1,
  AUTH_BLOB_REGION_ALL_ZERO = 2,
  AUTH_BLOB_REGION_ALL_FF = 3,
};

struct AuthStorageDiagnostics {
  uint8_t serial_region_state;
  uint8_t received_chunk_count;
  uint8_t total_chunk_count;
  uint8_t validation_status;
  uint16_t first_missing_offset;
  uint16_t target_payload_address;
};

bool authStorageHasValidKey(uint8_t keyType);
void authStorageInitialize();
uint16_t authStorageRequiredEnd();
uint16_t authStorageActivePayloadAddress(uint8_t keyType);
bool readAuthBlob(uint8_t keyType, AuthBlobRecord& blob);
void clearAuthBlob(uint8_t keyType);
void beginAuthUpload(uint8_t keyType);
void writeAuthChunk(uint8_t keyType, uint16_t offset, const uint8_t* data, uint8_t length);
bool finalizeAuthUpload(uint8_t keyType);
void updateAuthKeyStatus();
uint8_t authStorageKeyStatus();
uint8_t authStorageLastUploadStatus();
uint32_t authStorageLastUploadCrc32();
AuthStorageDiagnostics authStorageLastDiagnostics();
