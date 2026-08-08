#include "../../product_config.h"

#if defined(ADAPT_OUTPUT_USB_DEVICE) && defined(ADAPT_HAS_USB_AUTH_SIDECAR)

#include "auth_msc_ps4_import.h"

#include <Arduino.h>
#include <string.h>

#include "auth_storage.h"
#include "ps4_key_bundle.h"
#include "ps4_key_layout.h"

namespace {

constexpr size_t kPemFileCapacity = 4096;
constexpr size_t kSerialFileCapacity = 128;
constexpr char kKeyPemShortName[11] = {'K','E','Y',' ',' ',' ',' ',' ','P','E','M'};
constexpr char kSerialTxtShortName[11] = {'S','E','R','I','A','L',' ',' ','T','X','T'};
constexpr char kSigBinShortName[11] = {'S','I','G',' ',' ',' ',' ',' ','B','I','N'};

uint8_t g_upload_buffer[AUTH_KEY_PS4_SIZE];
char g_pem_buffer[kPemFileCapacity + 1];
uint8_t g_serial_buffer[kSerialFileCapacity];
uint8_t g_sig_buffer[PS4_KEY_SIG_SZ];
uint32_t g_last_seen_fingerprint = 0;
uint32_t g_last_processed_fingerprint = 0;
uint32_t g_last_processed_size = 0;

uint32_t fnv1a32(const void* data, size_t length) {
  const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data);
  uint32_t hash = 2166136261u;
  for (size_t i = 0; i < length; ++i) {
    hash ^= bytes[i];
    hash *= 16777619u;
  }
  return hash;
}

bool readFile(const AuthMscPs4ImportCallbacks& callbacks,
              const AuthMscPs4File& file,
              uint8_t* buffer,
              size_t capacity,
              void* context) {
  return callbacks.read_file_data != nullptr &&
         callbacks.read_file_data(&file, buffer, capacity, context);
}

void setResultText(const AuthMscPs4ImportCallbacks& callbacks,
                   const char* name,
                   const char* text,
                   void* context) {
  if (callbacks.set_result_text != nullptr) {
    callbacks.set_result_text(name, text, context);
  }
}

void setResultWithName(const AuthMscPs4ImportCallbacks& callbacks,
                       const char* format,
                       const AuthMscPs4File& file,
                       void* context) {
  if (callbacks.set_result_with_name != nullptr) {
    callbacks.set_result_with_name(format, file.name, file.file_size, context);
  }
}

}  // namespace

void auth_msc_ps4_import_reset() {
  memset(g_upload_buffer, 0, sizeof(g_upload_buffer));
  memset(g_pem_buffer, 0, sizeof(g_pem_buffer));
  memset(g_serial_buffer, 0, sizeof(g_serial_buffer));
  memset(g_sig_buffer, 0, sizeof(g_sig_buffer));
  g_last_seen_fingerprint = 0;
  g_last_processed_fingerprint = 0;
  g_last_processed_size = 0;
}

void auth_msc_ps4_import_process(const AuthMscPs4ImportCallbacks& callbacks, void* context) {
  if (callbacks.read_named_file == nullptr || callbacks.read_last_file == nullptr ||
      callbacks.read_file_data == nullptr) {
    return;
  }

  AuthMscPs4File pemFile{};
  AuthMscPs4File serialFile{};
  AuthMscPs4File sigFile{};

  const bool hasPem = callbacks.read_named_file(kKeyPemShortName, &pemFile, context);
  const bool hasSerial = callbacks.read_named_file(kSerialTxtShortName, &serialFile, context);
  const bool hasSig = callbacks.read_named_file(kSigBinShortName, &sigFile, context);

  const uint32_t metaFingerprint =
    pemFile.meta_fingerprint ^
    (serialFile.meta_fingerprint * 16777619u) ^
    (sigFile.meta_fingerprint * 2166136261u);

  if (!hasPem && !hasSerial && !hasSig) {
    AuthMscPs4File lastFile{};
    if (callbacks.read_last_file(&lastFile, context) &&
        lastFile.meta_fingerprint != g_last_seen_fingerprint) {
      g_last_seen_fingerprint = lastFile.meta_fingerprint;
      setResultText(callbacks, lastFile.name, "Split bundle only. Put key.pem + serial.txt + sig.bin in PS4AUTH.", context);
    }
    return;
  }

  if (!hasPem || !hasSerial || !hasSig) {
    if (metaFingerprint != g_last_seen_fingerprint) {
      g_last_seen_fingerprint = metaFingerprint;
      setResultText(callbacks, "PS4AUTH", "Waiting for key.pem + serial.txt + sig.bin in PS4AUTH.", context);
    }
    return;
  }

  memset(g_upload_buffer, 0, sizeof(g_upload_buffer));
  memset(g_pem_buffer, 0, sizeof(g_pem_buffer));
  memset(g_serial_buffer, 0, sizeof(g_serial_buffer));
  memset(g_sig_buffer, 0, sizeof(g_sig_buffer));

  if (!readFile(callbacks, pemFile, reinterpret_cast<uint8_t*>(g_pem_buffer), kPemFileCapacity, context)) {
    g_last_seen_fingerprint = metaFingerprint;
    setResultWithName(callbacks, "Failed to read %s (%lu bytes). Copy it again.", pemFile, context);
    return;
  }
  g_pem_buffer[pemFile.file_size] = '\0';

  if (!readFile(callbacks, serialFile, g_serial_buffer, kSerialFileCapacity, context)) {
    g_last_seen_fingerprint = metaFingerprint;
    setResultWithName(callbacks, "Failed to read %s (%lu bytes). Copy it again.", serialFile, context);
    return;
  }

  if (!readFile(callbacks, sigFile, g_sig_buffer, sizeof(g_sig_buffer), context)) {
    g_last_seen_fingerprint = metaFingerprint;
    setResultWithName(callbacks, "Failed to read %s (%lu bytes). Copy it again.", sigFile, context);
    return;
  }

  const uint32_t dataFingerprint =
    metaFingerprint ^
    fnv1a32(reinterpret_cast<const uint8_t*>(g_pem_buffer), pemFile.file_size) ^
    fnv1a32(g_serial_buffer, serialFile.file_size) ^
    fnv1a32(g_sig_buffer, sigFile.file_size);
  if (dataFingerprint == g_last_processed_fingerprint &&
      g_last_processed_size == AUTH_KEY_PS4_SIZE) {
    return;
  }

  PS4SplitBundleStatus bundleStatus = PS4_SPLIT_BUNDLE_OK;
  if (!ps4BuildAuthBlobFromSplitBundle(
        g_pem_buffer,
        pemFile.file_size,
        g_serial_buffer,
        serialFile.file_size,
        g_sig_buffer,
        sigFile.file_size,
        g_upload_buffer,
        sizeof(g_upload_buffer),
        &bundleStatus)) {
    g_last_seen_fingerprint = metaFingerprint;
    setResultText(callbacks, "PS4AUTH", ps4SplitBundleStatusLabel(bundleStatus), context);
    return;
  }

  beginAuthUpload(AUTH_KEY_TYPE_PS4);
  for (uint16_t offset = 0; offset < AUTH_KEY_PS4_SIZE; offset += 58u) {
    const uint8_t chunk = (uint8_t)(((AUTH_KEY_PS4_SIZE - offset) > 58u) ? 58u : (AUTH_KEY_PS4_SIZE - offset));
    writeAuthChunk(AUTH_KEY_TYPE_PS4, offset, &g_upload_buffer[offset], chunk);
  }

  if (finalizeAuthUpload(AUTH_KEY_TYPE_PS4)) {
    g_last_seen_fingerprint = metaFingerprint;
    g_last_processed_fingerprint = dataFingerprint;
    g_last_processed_size = AUTH_KEY_PS4_SIZE;
    setResultText(callbacks, "PS4AUTH", "Imported key.pem + serial.txt + sig.bin.", context);
  } else {
    g_last_seen_fingerprint = metaFingerprint;
    setResultText(callbacks, "PS4AUTH", "Rejected key.pem + serial.txt + sig.bin.", context);
  }
}

#endif
