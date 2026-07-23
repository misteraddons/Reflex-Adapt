#include "../../product_config.h"

#include "auth_storage.h"
#include "../runtime/output_boot_bridge.h"
#include "../usb/webhid/webhid_protocol.h"
#include "webhid_auth_reports.h"

void webhid_write_auth_key_report(const uint8_t* buffer, uint16_t bufsize) {
  // Write auth key chunk
  // Format: magic(1), key_type(1), offset_lo(1), offset_hi(1), length(1), data(58)
  if (bufsize < 5 || buffer[0] != WEBHID_MAGIC_BYTE) return;

  uint8_t key_type = buffer[1];
  uint16_t offset = buffer[2] | (buffer[3] << 8);
  uint8_t length = buffer[4];
  if (length > 58) length = 58;

  if (key_type != AUTH_KEY_TYPE_PS4) return;
  if (offset + length > AUTH_KEY_PS4_SIZE) return;

  writeAuthChunk(key_type, offset, &buffer[5], (uint8_t)min<uint16_t>(length, bufsize - 5));
  finalizeAuthUpload(key_type);
}

void webhid_clear_auth_key_report(const uint8_t* buffer, uint16_t bufsize) {
  // Clear auth key
  // Format: magic(1), key_type(1)
  if (bufsize < 2 || buffer[0] != WEBHID_MAGIC_BYTE) return;

  uint8_t key_type = buffer[1];
  if (key_type != AUTH_KEY_TYPE_PS4 && key_type != 0xFF) {
    return;
  }
  clearAuthBlob(key_type);
}
