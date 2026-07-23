#include "../../product_config.h"

#include "auth_storage.h"

#include <EEPROM.h>
#include <string.h>

#if defined(AUTH_KEY_STORAGE_USES_RAW_FLASH)
#include <Arduino.h>
#include <hardware/flash.h>
#include <hardware/sync.h>

extern "C" uint8_t _EEPROM_start[];
extern "C" uint8_t _FS_start[];
extern "C" uint8_t _FS_end[];

#define REFLEX_AUTH_RAW_FLASH 1
#endif

#include "../../core/eeprom_helper.h"
#include "ps4_key_blob.h"
#include "ps4_key_layout.h"

extern uint8_t auth_key_status;

#ifdef ADAPT_OUTPUT_USB_DEVICE

namespace {

// WebHID feature reports expose 63 payload bytes; auth chunks use 5 bytes of
// header and 58 bytes of key data.
constexpr uint8_t kAuthChunkDataSize = 58;
constexpr uint8_t kAuthChunkCount = (AUTH_KEY_PS4_SIZE + kAuthChunkDataSize - 1) / kAuthChunkDataSize;

static_assert(PERSISTED_BLOCK_HEADER_SIZE == AUTH_KEY_RECORD_HEADER_SIZE,
              "AUTH_KEY_RECORD_HEADER_SIZE must match PersistedBlockHeader");

struct AuthUploadState {
  bool active;
  uint8_t key_type;
  uint8_t target_slot;
  uint16_t next_generation;
  uint8_t received_chunks[kAuthChunkCount];
  AuthBlobRecord upload_blob;
};

AuthUploadState g_auth_upload_state = {};
uint8_t g_last_upload_status = AUTH_UPLOAD_STATUS_NONE;
uint32_t g_last_upload_crc32 = 0;
AuthStorageDiagnostics g_last_upload_diag = {
  AUTH_BLOB_REGION_UNKNOWN,
  0,
  kAuthChunkCount,
  AUTH_UPLOAD_STATUS_NONE,
  0xFFFFu,
  0xFFFFu
};

inline uint16_t authRecordAddress(uint8_t slot) {
  return AUTH_KEY_EEPROM_BASE + ((slot & 0x01u) * AUTH_KEY_PS4_SLOT_SIZE);
}

inline uint16_t authPayloadAddress(uint8_t slot) {
  return authRecordAddress(slot) + PERSISTED_BLOCK_HEADER_SIZE;
}

inline uint16_t authDiagnosticPayloadAddress(uint8_t slot) {
#if defined(REFLEX_AUTH_RAW_FLASH)
  (void)slot;
  return 0xFFFEu;
#else
  return authPayloadAddress(slot);
#endif
}

#if defined(REFLEX_AUTH_RAW_FLASH)
constexpr uint32_t kAuthFlashSectorSize = FLASH_SECTOR_SIZE;
static_assert(kAuthFlashSectorSize == 4096,
              "Auth raw flash storage expects a 4KB erase sector");
static_assert(AUTH_KEY_TOTAL_SIZE <= kAuthFlashSectorSize,
              "PS4 auth A/B records must fit in one reserved flash sector");

alignas(4) uint8_t authFlashScratch[kAuthFlashSectorSize];

uintptr_t authFlashSectorAddress() {
  // Classic2USB's many input modes push auth storage beyond the RP2040
  // EEPROM-emulation window. The build reserves one unused 4KB filesystem
  // sector immediately before EEPROM; use it for PS4 auth records.
  (void)_EEPROM_start;
  (void)_FS_end;
  return reinterpret_cast<uintptr_t>(_FS_start);
}

const uint8_t* authFlashSector() {
  return reinterpret_cast<const uint8_t*>(authFlashSectorAddress());
}

uint32_t authFlashOffset() {
  return static_cast<uint32_t>(authFlashSectorAddress() - XIP_BASE);
}

bool readFlashPersistedAuthRecord(uint8_t candidate,
                                  AuthBlobRecord& blob,
                                  uint16_t* generation) {
  if (candidate >= PERSISTED_AB_SLOT_COUNT) {
    return false;
  }

  const uint32_t recordOffset = (uint32_t)candidate * AUTH_KEY_PS4_SLOT_SIZE;
  const uint8_t* record = authFlashSector() + recordOffset;

  PersistedBlockHeader header{};
  memcpy(&header, record, sizeof(header));
  if (!isPersistedBlockHeaderValid(
        header,
        PERSISTED_BLOCK_MAGIC_AUTH,
        (uint8_t)sizeof(AuthBlobRecord))) {
    return false;
  }

  memcpy(&blob, record + PERSISTED_BLOCK_HEADER_SIZE, sizeof(blob));
  if (header.payload_crc16 != calculateEepromBlockCrc(blob)) {
    return false;
  }

  if (generation) {
    *generation = header.generation;
  }
  return true;
}

bool readLatestFlashAuthRecord(uint8_t& slot, uint16_t& generation, AuthBlobRecord& blob) {
  AuthBlobRecord slotValue[PERSISTED_AB_SLOT_COUNT] = {};
  uint16_t slotGeneration[PERSISTED_AB_SLOT_COUNT] = {};
  bool valid[PERSISTED_AB_SLOT_COUNT] = {};

  for (uint8_t candidate = 0; candidate < PERSISTED_AB_SLOT_COUNT; ++candidate) {
    valid[candidate] = readFlashPersistedAuthRecord(
      candidate,
      slotValue[candidate],
      &slotGeneration[candidate]
    );
  }

  if (!valid[0] && !valid[1]) {
    return false;
  }

  slot = 0;
  if (!valid[0]) {
    slot = 1;
  } else if (valid[1] && isGenerationNewer(slotGeneration[1], slotGeneration[0])) {
    slot = 1;
  }

  generation = slotGeneration[slot];
  blob = slotValue[slot];
  return true;
}

void buildFlashAuthSector(uint8_t targetSlot,
                          uint16_t nextGeneration,
                          const AuthBlobRecord& blob) {
  memcpy(authFlashScratch, authFlashSector(), sizeof(authFlashScratch));

  const uint32_t recordOffset = (uint32_t)targetSlot * AUTH_KEY_PS4_SLOT_SIZE;
  memset(authFlashScratch + recordOffset, 0xFF, AUTH_KEY_PS4_SLOT_SIZE);
  memcpy(
    authFlashScratch + recordOffset + PERSISTED_BLOCK_HEADER_SIZE,
    &blob,
    sizeof(blob)
  );

  const PersistedBlockHeader header = buildPersistedBlockHeader(
    PERSISTED_BLOCK_MAGIC_AUTH,
    (uint8_t)sizeof(AuthBlobRecord),
    nextGeneration,
    calculateEepromBlockCrc(blob)
  );
  memcpy(authFlashScratch + recordOffset, &header, sizeof(header));
}

void programFlashAuthSector() {
#ifndef __FREERTOS
  noInterrupts();
#endif
  rp2040.idleOtherCore();
  flash_range_erase(authFlashOffset(), kAuthFlashSectorSize);
  flash_range_program(authFlashOffset(), authFlashScratch, kAuthFlashSectorSize);
  rp2040.resumeOtherCore();
#ifndef __FREERTOS
  interrupts();
#endif
}

bool writeFlashAuthRecord(uint8_t targetSlot,
                          uint16_t nextGeneration,
                          const AuthBlobRecord& blob) {
  buildFlashAuthSector(targetSlot, nextGeneration, blob);
  programFlashAuthSector();

  AuthBlobRecord verify{};
  uint16_t verifyGeneration = 0;
  if (!readFlashPersistedAuthRecord(targetSlot, verify, &verifyGeneration)) {
    return false;
  }
  return verifyGeneration == nextGeneration &&
         memcmp(&verify, &blob, sizeof(blob)) == 0;
}

void clearFlashAuthRecords() {
  memset(authFlashScratch, 0xFF, sizeof(authFlashScratch));
  programFlashAuthSector();
}
#endif

bool readEepromLatestAuthRecord(uint8_t& slot, uint16_t& generation, AuthBlobRecord& blob) {
  return readLatestPersistedRecordAB(
    AUTH_KEY_EEPROM_BASE,
    AUTH_KEY_PS4_SLOT_SIZE,
    PERSISTED_BLOCK_MAGIC_AUTH,
    slot,
    generation,
    blob
  );
}

bool readLatestAuthRecord(uint8_t& slot, uint16_t& generation, AuthBlobRecord& blob) {
#if defined(REFLEX_AUTH_RAW_FLASH)
  return readLatestFlashAuthRecord(slot, generation, blob);
#else
  return readEepromLatestAuthRecord(slot, generation, blob);
#endif
}

bool allChunksReceived() {
  for (uint8_t i = 0; i < kAuthChunkCount; ++i) {
    if (g_auth_upload_state.received_chunks[i] == 0) {
      return false;
    }
  }
  return true;
}

AuthBlobRegionState classifyRegion(const uint8_t* data, size_t length) {
  if (data == nullptr || length == 0) {
    return AUTH_BLOB_REGION_UNKNOWN;
  }
  bool anyNonZero = false;
  bool anyNonFF = false;
  for (size_t i = 0; i < length; ++i) {
    anyNonZero |= (data[i] != 0x00u);
    anyNonFF |= (data[i] != 0xFFu);
  }
  if (!anyNonZero) {
    return AUTH_BLOB_REGION_ALL_ZERO;
  }
  if (!anyNonFF) {
    return AUTH_BLOB_REGION_ALL_FF;
  }
  return AUTH_BLOB_REGION_MEANINGFUL;
}

uint8_t receivedChunkCount() {
  uint8_t count = 0;
  for (uint8_t i = 0; i < kAuthChunkCount; ++i) {
    if (g_auth_upload_state.received_chunks[i] != 0) {
      ++count;
    }
  }
  return count;
}

uint16_t firstMissingOffset() {
  for (uint8_t i = 0; i < kAuthChunkCount; ++i) {
    if (g_auth_upload_state.received_chunks[i] == 0) {
      const uint16_t offset = (uint16_t)i * kAuthChunkDataSize;
      return offset < AUTH_KEY_PS4_SIZE ? offset : AUTH_KEY_PS4_SIZE;
    }
  }
  return 0xFFFFu;
}

void updateUploadDiagnostics(uint8_t validationStatus, AuthBlobRegionState serialRegion) {
  g_last_upload_diag.serial_region_state = serialRegion;
  g_last_upload_diag.received_chunk_count = receivedChunkCount();
  g_last_upload_diag.total_chunk_count = kAuthChunkCount;
  g_last_upload_diag.validation_status = validationStatus;
  g_last_upload_diag.first_missing_offset = firstMissingOffset();
  g_last_upload_diag.target_payload_address =
    g_auth_upload_state.active ? authDiagnosticPayloadAddress(g_auth_upload_state.target_slot) : 0xFFFFu;
}

void resetUploadState() {
  g_auth_upload_state = {};
}

void primeUploadBlob() {
  memset(&g_auth_upload_state.upload_blob, 0xFF, sizeof(g_auth_upload_state.upload_blob));
}

uint8_t mapValidationStatus(PS4KeyBlobValidation validation) {
  switch (validation) {
    case PS4_KEY_BLOB_VALID:
      return AUTH_UPLOAD_STATUS_OK;
    case PS4_KEY_BLOB_INVALID_SERIAL:
      return AUTH_UPLOAD_STATUS_INVALID_SERIAL;
    case PS4_KEY_BLOB_INVALID_SIGNATURE:
      return AUTH_UPLOAD_STATUS_INVALID_SIGNATURE;
    case PS4_KEY_BLOB_INVALID_MODULUS:
      return AUTH_UPLOAD_STATUS_INVALID_MODULUS;
    case PS4_KEY_BLOB_INVALID_EXPONENT:
      return AUTH_UPLOAD_STATUS_INVALID_EXPONENT;
    case PS4_KEY_BLOB_INVALID_PRIVATE:
      return AUTH_UPLOAD_STATUS_INVALID_PRIVATE;
    case PS4_KEY_BLOB_INVALID_CRT:
      return AUTH_UPLOAD_STATUS_INVALID_CRT;
    case PS4_KEY_BLOB_INVALID_LENGTH:
    default:
      return AUTH_UPLOAD_STATUS_INCOMPLETE;
  }
}

}  // namespace

void updateAuthKeyStatus() {
  auth_key_status = authStorageHasValidKey(AUTH_KEY_TYPE_PS4) ? 0x01 : 0x00;
}

uint8_t authStorageKeyStatus() {
  return auth_key_status;
}

uint16_t authStorageRequiredEnd() {
#if defined(REFLEX_AUTH_RAW_FLASH)
  return AUTH_KEY_EEPROM_END;
#else
  return AUTH_KEY_EEPROM_END;
#endif
}

bool authStorageHasValidKey(uint8_t keyType) {
  if (keyType != AUTH_KEY_TYPE_PS4) {
    return false;
  }
  uint8_t slot = 0;
  uint16_t generation = 0;
  AuthBlobRecord blob{};
  return readLatestAuthRecord(slot, generation, blob);
}

void authStorageInitialize() {
  if (AUTH_KEY_PS4_SIZE == 0) {
    g_last_upload_status = AUTH_UPLOAD_STATUS_NONE;
    g_last_upload_crc32 = 0;
    updateAuthKeyStatus();
    return;
  }

  uint8_t slot = 0;
  uint16_t generation = 0;
  AuthBlobRecord blob{};
  if (!readLatestAuthRecord(slot, generation, blob)) {
#if defined(REFLEX_AUTH_RAW_FLASH)
    g_last_upload_status = AUTH_UPLOAD_STATUS_NONE;
    g_last_upload_crc32 = 0;
    g_last_upload_diag = {
      AUTH_BLOB_REGION_UNKNOWN,
      0,
      kAuthChunkCount,
      AUTH_UPLOAD_STATUS_NONE,
      0xFFFFu,
      0xFFFFu
    };
#else
    EepromTransaction txn;
    stageEepromRangeFill(txn, AUTH_KEY_EEPROM_BASE, AUTH_KEY_TOTAL_SIZE, 0xFF);
    txn.commit();
    g_last_upload_status = AUTH_UPLOAD_STATUS_NONE;
    g_last_upload_crc32 = 0;
    g_last_upload_diag = {
      AUTH_BLOB_REGION_UNKNOWN,
      0,
      kAuthChunkCount,
      AUTH_UPLOAD_STATUS_NONE,
      0xFFFFu,
      0xFFFFu
    };
#endif
  } else {
    g_last_upload_status = AUTH_UPLOAD_STATUS_OK;
    g_last_upload_crc32 = ps4KeyBlobCrc32(blob.data, sizeof(blob.data));
    g_last_upload_diag = {
      classifyRegion(blob.data + PS4_KEY_SERIAL_OFF, PS4_KEY_SERIAL_SZ),
      kAuthChunkCount,
      kAuthChunkCount,
      AUTH_UPLOAD_STATUS_OK,
      0xFFFFu,
      authDiagnosticPayloadAddress(slot)
    };
  }
  resetUploadState();
  updateAuthKeyStatus();
}

uint16_t authStorageActivePayloadAddress(uint8_t keyType) {
  if (keyType != AUTH_KEY_TYPE_PS4) {
    return 0xFFFFu;
  }

  uint8_t slot = 0;
  uint16_t generation = 0;
  AuthBlobRecord blob{};
  if (!readLatestAuthRecord(slot, generation, blob)) {
    return 0xFFFFu;
  }
  return authPayloadAddress(slot);
}

bool readAuthBlob(uint8_t keyType, AuthBlobRecord& blob) {
  if (keyType != AUTH_KEY_TYPE_PS4) {
    return false;
  }

  uint8_t slot = 0;
  uint16_t generation = 0;
  return readLatestAuthRecord(slot, generation, blob);
}

void clearAuthBlob(uint8_t keyType) {
  if (keyType != AUTH_KEY_TYPE_PS4 && keyType != 0xFF) {
    return;
  }

#if defined(REFLEX_AUTH_RAW_FLASH)
  clearFlashAuthRecords();
#else
  EepromTransaction txn;
  stageEepromRangeFill(txn, AUTH_KEY_EEPROM_BASE, AUTH_KEY_TOTAL_SIZE, 0xFF);
  txn.commit();
#endif
  resetUploadState();
  g_last_upload_status = AUTH_UPLOAD_STATUS_CLEARED;
  g_last_upload_crc32 = 0;
  g_last_upload_diag = {
    AUTH_BLOB_REGION_UNKNOWN,
    0,
    kAuthChunkCount,
    AUTH_UPLOAD_STATUS_CLEARED,
    0xFFFFu,
    0xFFFFu
  };
  updateAuthKeyStatus();
}

void beginAuthUpload(uint8_t keyType) {
  if (keyType != AUTH_KEY_TYPE_PS4) {
    return;
  }

  uint8_t currentSlot = 0;
  uint16_t currentGeneration = 0;
  AuthBlobRecord currentBlob{};
  const bool hasCurrent = readLatestAuthRecord(currentSlot, currentGeneration, currentBlob);

  resetUploadState();
  g_auth_upload_state.active = true;
  g_auth_upload_state.key_type = keyType;
  g_auth_upload_state.target_slot = hasCurrent ? (uint8_t)(currentSlot ^ 0x01u) : 0;
  g_auth_upload_state.next_generation = hasCurrent ? (uint16_t)(currentGeneration + 1u) : 1u;
  primeUploadBlob();
  g_last_upload_status = AUTH_UPLOAD_STATUS_IN_PROGRESS;
  g_last_upload_crc32 = 0;
  updateUploadDiagnostics(AUTH_UPLOAD_STATUS_IN_PROGRESS, AUTH_BLOB_REGION_UNKNOWN);

#if !defined(REFLEX_AUTH_RAW_FLASH)
  EepromTransaction txn;
  stageEepromRangeFill(txn, authRecordAddress(g_auth_upload_state.target_slot), AUTH_KEY_PS4_SLOT_SIZE, 0xFF);
  txn.commit();
#endif
}

void writeAuthChunk(uint8_t keyType, uint16_t offset, const uint8_t* data, uint8_t length) {
  if (keyType != AUTH_KEY_TYPE_PS4 || length == 0 || data == nullptr) {
    return;
  }
  if (offset >= AUTH_KEY_PS4_SIZE) {
    return;
  }
  if (offset + length > AUTH_KEY_PS4_SIZE) {
    length = (uint8_t)(AUTH_KEY_PS4_SIZE - offset);
  }

  if (!g_auth_upload_state.active || g_auth_upload_state.key_type != keyType) {
    beginAuthUpload(keyType);
  }

  memcpy(g_auth_upload_state.upload_blob.data + offset, data, length);

#if !defined(REFLEX_AUTH_RAW_FLASH)
  for (uint8_t i = 0; i < length; ++i) {
    EEPROM.write(authPayloadAddress(g_auth_upload_state.target_slot) + offset + i, data[i]);
  }
  EEPROM.commit();
#endif

  const uint8_t firstChunk = (uint8_t)(offset / kAuthChunkDataSize);
  const uint8_t lastChunk = (uint8_t)((offset + length - 1u) / kAuthChunkDataSize);
  for (uint8_t chunk = firstChunk; chunk <= lastChunk && chunk < kAuthChunkCount; ++chunk) {
    g_auth_upload_state.received_chunks[chunk] = 1;
  }
}

bool finalizeAuthUpload(uint8_t keyType) {
  if (keyType != AUTH_KEY_TYPE_PS4 ||
      !g_auth_upload_state.active ||
      g_auth_upload_state.key_type != keyType ||
      !allChunksReceived()) {
    g_last_upload_status = AUTH_UPLOAD_STATUS_INCOMPLETE;
    updateUploadDiagnostics(AUTH_UPLOAD_STATUS_INCOMPLETE, AUTH_BLOB_REGION_UNKNOWN);
    return false;
  }

  AuthBlobRecord blob{};
#if defined(REFLEX_AUTH_RAW_FLASH)
  blob = g_auth_upload_state.upload_blob;
#else
  EEPROM.get(authPayloadAddress(g_auth_upload_state.target_slot), blob);
#endif
  g_last_upload_crc32 = ps4KeyBlobCrc32(blob.data, sizeof(blob.data));
  const AuthBlobRegionState serialRegion =
    classifyRegion(blob.data + PS4_KEY_SERIAL_OFF, PS4_KEY_SERIAL_SZ);

  const PS4KeyBlobValidation validation = ps4ValidateAuthBlob(blob.data, sizeof(blob.data));
  if (validation != PS4_KEY_BLOB_VALID) {
    g_last_upload_status = mapValidationStatus(validation);
    updateUploadDiagnostics(g_last_upload_status, serialRegion);
    resetUploadState();
    updateAuthKeyStatus();
    return false;
  }

#if defined(REFLEX_AUTH_RAW_FLASH)
  if (!writeFlashAuthRecord(
        g_auth_upload_state.target_slot,
        g_auth_upload_state.next_generation,
        blob)) {
    g_last_upload_status = AUTH_UPLOAD_STATUS_INCOMPLETE;
    updateUploadDiagnostics(g_last_upload_status, serialRegion);
    resetUploadState();
    updateAuthKeyStatus();
    return false;
  }
#else
  writePersistedRecord(
    authRecordAddress(g_auth_upload_state.target_slot),
    PERSISTED_BLOCK_MAGIC_AUTH,
    blob,
    g_auth_upload_state.next_generation
  );
  EEPROM.commit();
#endif
  const uint8_t committedSlot = g_auth_upload_state.target_slot;
  resetUploadState();
  g_last_upload_status = AUTH_UPLOAD_STATUS_OK;
  g_last_upload_diag.serial_region_state = serialRegion;
  g_last_upload_diag.received_chunk_count = kAuthChunkCount;
  g_last_upload_diag.total_chunk_count = kAuthChunkCount;
  g_last_upload_diag.validation_status = AUTH_UPLOAD_STATUS_OK;
  g_last_upload_diag.first_missing_offset = 0xFFFFu;
  g_last_upload_diag.target_payload_address = authDiagnosticPayloadAddress(committedSlot);
  updateAuthKeyStatus();
  return true;
}

uint8_t authStorageLastUploadStatus() {
  return g_last_upload_status;
}

uint32_t authStorageLastUploadCrc32() {
  return g_last_upload_crc32;
}

AuthStorageDiagnostics authStorageLastDiagnostics() {
  return g_last_upload_diag;
}

extern "C" uint16_t auth_storage_active_payload_address(uint8_t keyType) {
  return authStorageActivePayloadAddress(keyType);
}

#else  // !ADAPT_OUTPUT_USB_DEVICE

void updateAuthKeyStatus() {
  auth_key_status = 0;
}

uint8_t authStorageKeyStatus() {
  return 0;
}

uint16_t authStorageRequiredEnd() {
  return AUTH_KEY_EEPROM_BASE;
}

bool authStorageHasValidKey(uint8_t keyType) {
  (void)keyType;
  return false;
}

void authStorageInitialize() {
  updateAuthKeyStatus();
}

uint16_t authStorageActivePayloadAddress(uint8_t keyType) {
  (void)keyType;
  return 0xFFFFu;
}

bool readAuthBlob(uint8_t keyType, AuthBlobRecord& blob) {
  (void)keyType;
  (void)blob;
  return false;
}

void clearAuthBlob(uint8_t keyType) {
  (void)keyType;
  updateAuthKeyStatus();
}

void beginAuthUpload(uint8_t keyType) {
  (void)keyType;
}

void writeAuthChunk(uint8_t keyType, uint16_t offset, const uint8_t* data, uint8_t length) {
  (void)keyType;
  (void)offset;
  (void)data;
  (void)length;
}

bool finalizeAuthUpload(uint8_t keyType) {
  (void)keyType;
  return false;
}

uint8_t authStorageLastUploadStatus() {
  return AUTH_UPLOAD_STATUS_NONE;
}

uint32_t authStorageLastUploadCrc32() {
  return 0;
}

AuthStorageDiagnostics authStorageLastDiagnostics() {
  return {
    AUTH_BLOB_REGION_UNKNOWN,
    0,
    0,
    AUTH_UPLOAD_STATUS_NONE,
    0xFFFFu,
    0xFFFFu
  };
}

extern "C" uint16_t auth_storage_active_payload_address(uint8_t keyType) {
  return authStorageActivePayloadAddress(keyType);
}

#endif  // ADAPT_OUTPUT_USB_DEVICE
