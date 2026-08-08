#include "../../product_config.h"

#if defined(ADAPT_OUTPUT_USB_DEVICE) && defined(ADAPT_HAS_USB_AUTH_SIDECAR)

#include "auth_msc_status_file.h"
#include "../../firmware_build_info.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>

#include "auth_storage.h"

namespace {

constexpr size_t kStatusFileCapacity = 512;

char g_status_file[kStatusFileCapacity];
size_t g_status_file_length = 0;
char g_last_result[160] = "Waiting for key.pem + serial.txt + sig.bin in PS4AUTH.";
char g_last_name[20] = "";

const char* authUploadStatusLabel(uint8_t status) {
  switch (status) {
    case AUTH_UPLOAD_STATUS_NONE:
      return "none";
    case AUTH_UPLOAD_STATUS_IN_PROGRESS:
      return "in progress";
    case AUTH_UPLOAD_STATUS_OK:
      return "accepted";
    case AUTH_UPLOAD_STATUS_CLEARED:
      return "cleared";
    case AUTH_UPLOAD_STATUS_INCOMPLETE:
      return "incomplete";
    case AUTH_UPLOAD_STATUS_INVALID_SERIAL:
      return "invalid serial";
    case AUTH_UPLOAD_STATUS_INVALID_SIGNATURE:
      return "invalid signature";
    case AUTH_UPLOAD_STATUS_INVALID_MODULUS:
      return "invalid modulus";
    case AUTH_UPLOAD_STATUS_INVALID_EXPONENT:
      return "invalid exponent";
    case AUTH_UPLOAD_STATUS_INVALID_PRIVATE:
      return "invalid private exponent";
    case AUTH_UPLOAD_STATUS_INVALID_CRT:
      return "invalid CRT parameters";
    default:
      return "unknown";
  }
}

const char* authBlobRegionLabel(uint8_t state) {
  switch (state) {
    case AUTH_BLOB_REGION_MEANINGFUL:
      return "meaningful";
    case AUTH_BLOB_REGION_ALL_ZERO:
      return "all zero";
    case AUTH_BLOB_REGION_ALL_FF:
      return "all FF";
    case AUTH_BLOB_REGION_UNKNOWN:
    default:
      return "unknown";
  }
}

void setLastName(const char* name) {
  if (name != nullptr && name[0] != '\0') {
    strncpy(g_last_name, name, sizeof(g_last_name) - 1u);
    g_last_name[sizeof(g_last_name) - 1u] = '\0';
  } else {
    g_last_name[0] = '\0';
  }
}

}  // namespace

void auth_msc_status_reset() {
  memset(g_status_file, 0, sizeof(g_status_file));
  g_status_file_length = 0;
  g_last_name[0] = '\0';
  strncpy(g_last_result, "Waiting for key.pem + serial.txt + sig.bin in PS4AUTH.", sizeof(g_last_result) - 1u);
  g_last_result[sizeof(g_last_result) - 1u] = '\0';
}

void auth_msc_status_refresh() {
  const bool hasKey = authStorageHasValidKey(AUTH_KEY_TYPE_PS4);
  const uint8_t uploadStatus = authStorageLastUploadStatus();
  const uint32_t uploadCrc = authStorageLastUploadCrc32();
  const AuthStorageDiagnostics diag = authStorageLastDiagnostics();
  const int written = snprintf(
    g_status_file,
    sizeof(g_status_file),
    "Reflex Adapt Device\r\n"
    "===================\r\n"
    "\r\n"
    "Product: %s\r\n"
    "Firmware: %s\r\n"
    "PS4 key stored: %s\r\n"
    "Last upload status: %s\r\n"
    "Last blob CRC32: 0x%08lX\r\n"
    "Diag: serial=%s chunks=%u/%u miss=0x%04X addr=0x%04X\r\n"
    "Last result: %s\r\n"
    "Last file: %s\r\n"
    "\r\n"
    "Accepted PS4 auth format:\r\n"
    "  - key.pem + serial.txt + sig.bin GP2040-CE bundle\r\n"
    "\r\n"
    "Open PS4AUTH.HTM for live split-bundle upload/clear controls.\r\n",
    PRODUCT_SHORT_NAME,
    FIRMWARE_VERSION_STRING,
    hasKey ? "yes" : "no",
    authUploadStatusLabel(uploadStatus),
    (unsigned long)uploadCrc,
    authBlobRegionLabel(diag.serial_region_state),
    (unsigned)diag.received_chunk_count,
    (unsigned)diag.total_chunk_count,
    (unsigned)diag.first_missing_offset,
    (unsigned)diag.target_payload_address,
    g_last_result,
    g_last_name[0] ? g_last_name : "(none)"
  );

  g_status_file_length = (written < 0) ? 0u :
    ((size_t)written < sizeof(g_status_file) ? (size_t)written : sizeof(g_status_file) - 1u);
}

const char* auth_msc_status_text() {
  return g_status_file;
}

size_t auth_msc_status_len() {
  return g_status_file_length;
}

void auth_msc_status_set_result_text(const char* name, const char* text) {
  setLastName(name);
  strncpy(g_last_result, text, sizeof(g_last_result) - 1u);
  g_last_result[sizeof(g_last_result) - 1u] = '\0';
  auth_msc_status_refresh();
}

void auth_msc_status_set_result_with_name(const char* format, const char* name, uint32_t size) {
  setLastName(name);
  snprintf(g_last_result, sizeof(g_last_result), format, name, size);
  auth_msc_status_refresh();
}

#endif
