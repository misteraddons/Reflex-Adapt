#include "../../product_config.h"

#if defined(ADAPT_OUTPUT_USB_DEVICE) && defined(ADAPT_HAS_USB_AUTH_SIDECAR)

#include "auth_msc_runtime.h"

#include <Adafruit_TinyUSB.h>
#include <Arduino.h>

#include "../../core/device_mode.h"
#include "../../core/device_runtime_state.h"
#include "../output_capabilities.h"
#include "../output_runtime_state.h"
#include "auth_msc_content.h"
#include "auth_msc_virtual_drive.h"
#include "auth_storage.h"

namespace {

Adafruit_USBD_MSC g_auth_msc;
bool g_auth_msc_enabled = false;
bool g_auth_msc_dirty = false;
bool g_auth_msc_import_pending = false;

// RP2040 TinyUSB services full 512-byte logical sectors through 64-byte USB
// endpoint chunks. Adafruit's MSC callback drops TinyUSB's byte offset, so keep
// the current logical-sector cursor here and reconstruct each transfer.
struct MscSectorCursor {
  uint32_t lba = 0;
  uint16_t offset = 0;
  bool active = false;
  uint8_t data[512] __attribute__((aligned(4)));
};

MscSectorCursor g_auth_msc_read_cursor;
MscSectorCursor g_auth_msc_write_cursor;

void resetMscTransferCursors() {
  g_auth_msc_read_cursor.active = false;
  g_auth_msc_read_cursor.offset = 0;
  g_auth_msc_write_cursor.active = false;
  g_auth_msc_write_cursor.offset = 0;
}

int32_t mscReadCallback(uint32_t lba, void* buffer, uint32_t bufsize) {
  if (!g_auth_msc_enabled) {
    return -1;
  }

  const uint16_t sectorSize = auth_msc_virtual_drive_sector_size();
  const uint32_t sectorCount = auth_msc_virtual_drive_sector_count();
  if (lba >= sectorCount || sectorSize > sizeof(g_auth_msc_read_cursor.data)) {
    return -1;
  }

  if (!g_auth_msc_read_cursor.active || g_auth_msc_read_cursor.lba != lba) {
    auth_msc_virtual_drive_read_sector(lba, g_auth_msc_read_cursor.data);
    g_auth_msc_read_cursor.lba = lba;
    g_auth_msc_read_cursor.offset = 0;
    g_auth_msc_read_cursor.active = true;
  }

  const uint32_t remaining = sectorSize - g_auth_msc_read_cursor.offset;
  const uint32_t copied = min(bufsize, remaining);
  memcpy(
    buffer,
    g_auth_msc_read_cursor.data + g_auth_msc_read_cursor.offset,
    copied
  );
  g_auth_msc_read_cursor.offset += copied;
  if (g_auth_msc_read_cursor.offset >= sectorSize) {
    g_auth_msc_read_cursor.active = false;
    g_auth_msc_read_cursor.offset = 0;
  }
  return (int32_t)copied;
}

int32_t mscWriteCallback(uint32_t lba, uint8_t* buffer, uint32_t bufsize) {
  if (!g_auth_msc_enabled) {
    return -1;
  }

  const uint16_t sectorSize = auth_msc_virtual_drive_sector_size();
  const uint32_t sectorCount = auth_msc_virtual_drive_sector_count();
  if (lba >= sectorCount || sectorSize > sizeof(g_auth_msc_write_cursor.data)) {
    return -1;
  }

  if (!g_auth_msc_write_cursor.active || g_auth_msc_write_cursor.lba != lba) {
    // Preserve bytes outside a short host write, even though normal WRITE10
    // commands arrive as a complete sector split across endpoint packets.
    auth_msc_virtual_drive_read_sector(lba, g_auth_msc_write_cursor.data);
    g_auth_msc_write_cursor.lba = lba;
    g_auth_msc_write_cursor.offset = 0;
    g_auth_msc_write_cursor.active = true;
  }

  const uint32_t remaining = sectorSize - g_auth_msc_write_cursor.offset;
  const uint32_t copied = min(bufsize, remaining);
  memcpy(
    g_auth_msc_write_cursor.data + g_auth_msc_write_cursor.offset,
    buffer,
    copied
  );
  g_auth_msc_write_cursor.offset += copied;
  if (g_auth_msc_write_cursor.offset >= sectorSize) {
    auth_msc_virtual_drive_write_sector(lba, g_auth_msc_write_cursor.data);
    g_auth_msc_write_cursor.active = false;
    g_auth_msc_write_cursor.offset = 0;
    g_auth_msc_dirty = true;
  }
  return (int32_t)copied;
}

void mscFlushCallback() {
  if (!g_auth_msc_enabled || !g_auth_msc_dirty) {
    return;
  }
  g_auth_msc_dirty = false;
  g_auth_msc_import_pending = true;
}

bool mscReadyCallback() {
  return g_auth_msc_enabled;
}

}  // namespace


extern "C" bool auth_msc_should_enable(outputMode_t effectiveOutputMode) {
  if (AUTH_KEY_PS4_SIZE == 0) {
    return false;
  }
  if (deviceMode == RZORD_MEMCARD) {
    return false;
  }
  return output_allows_management_msc(effectiveOutputMode);
}

extern "C" void auth_msc_configure(outputMode_t effectiveOutputMode) {
  g_auth_msc_enabled = auth_msc_should_enable(effectiveOutputMode);
  if (!g_auth_msc_enabled) {
    g_auth_msc_dirty = false;
    g_auth_msc_import_pending = false;
    resetMscTransferCursors();
    return;
  }

  g_auth_msc_import_pending = false;
  resetMscTransferCursors();
  auth_msc_virtual_drive_build();
  g_auth_msc.setID("Adapt", auth_msc_product_name(), "1.0");
  g_auth_msc.setCapacity(auth_msc_virtual_drive_sector_count(), auth_msc_virtual_drive_sector_size());
  g_auth_msc.setReadWriteCallback(mscReadCallback, mscWriteCallback, mscFlushCallback);
  g_auth_msc.setReadyCallback(mscReadyCallback);
  g_auth_msc.setUnitReady(true);
  g_auth_msc.begin();
}

extern "C" bool auth_msc_is_enabled() {
  return g_auth_msc_enabled;
}

extern "C" void auth_msc_task() {
  if (!g_auth_msc_enabled || !g_auth_msc_import_pending) {
    return;
  }
  g_auth_msc_import_pending = false;
  auth_msc_virtual_drive_process_import();
}

#else

#include "auth_msc_runtime.h"

extern "C" bool auth_msc_should_enable(outputMode_t) {
  return false;
}

extern "C" void auth_msc_configure(outputMode_t) {}

extern "C" bool auth_msc_is_enabled() {
  return false;
}

extern "C" void auth_msc_task() {}

#endif

