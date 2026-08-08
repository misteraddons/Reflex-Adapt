#include "../../product_config.h"

#if defined(ADAPT_OUTPUT_USB_DEVICE) && defined(ADAPT_HAS_USB_AUTH_SIDECAR)

#include "auth_msc_fat12.h"

#include <stddef.h>
#include <string.h>

bool auth_msc_lba_in_range(uint32_t lba, uint32_t start, uint32_t count) {
  return lba >= start && lba < (start + count);
}

bool auth_msc_cluster_in_range(uint16_t cluster, uint16_t start, uint16_t count) {
  return cluster >= start && cluster < (uint16_t)(start + count);
}

uint16_t auth_msc_fat12_get_entry(const uint8_t* fat, size_t fat_len, uint16_t cluster) {
  const uint32_t offset = cluster + (cluster / 2u);
  if (fat == nullptr || offset + 1u >= fat_len) {
    return AUTH_MSC_FAT12_EOC;
  }
  if (cluster & 0x01u) {
    return (uint16_t)(((fat[offset] >> 4) | (fat[offset + 1u] << 4)) & 0x0FFFu);
  }
  return (uint16_t)((fat[offset] | ((fat[offset + 1u] & 0x0Fu) << 8)) & 0x0FFFu);
}

void auth_msc_fat12_set_entry(uint8_t* fat, size_t fat_len, uint16_t cluster, uint16_t value) {
  const uint32_t offset = cluster + (cluster / 2u);
  if (fat == nullptr || offset + 1u >= fat_len) {
    return;
  }
  value &= 0x0FFFu;
  if (cluster & 0x01u) {
    fat[offset] = (uint8_t)((fat[offset] & 0x0Fu) | ((value << 4) & 0xF0u));
    fat[offset + 1u] = (uint8_t)(value >> 4);
  } else {
    fat[offset] = (uint8_t)(value & 0xFFu);
    fat[offset + 1u] = (uint8_t)((fat[offset + 1u] & 0xF0u) | ((value >> 8) & 0x0Fu));
  }
}

void auth_msc_fat12_set_cluster_chain(uint8_t* fat, size_t fat_len, uint16_t start_cluster, uint16_t cluster_count) {
  for (uint16_t i = 0; i < cluster_count; ++i) {
    const uint16_t cluster = (uint16_t)(start_cluster + i);
    const uint16_t next = (i + 1u < cluster_count) ? (uint16_t)(cluster + 1u) : AUTH_MSC_FAT12_EOC;
    auth_msc_fat12_set_entry(fat, fat_len, cluster, next);
  }
}

static void write_dir_le16(AuthMscDirEntry& entry, size_t offset, uint16_t value) {
  uint8_t* raw = reinterpret_cast<uint8_t*>(&entry);
  raw[offset] = (uint8_t)(value & 0xFFu);
  raw[offset + 1u] = (uint8_t)(value >> 8);
}

static void write_dir_le32(AuthMscDirEntry& entry, size_t offset, uint32_t value) {
  uint8_t* raw = reinterpret_cast<uint8_t*>(&entry);
  raw[offset] = (uint8_t)(value & 0xFFu);
  raw[offset + 1u] = (uint8_t)((value >> 8) & 0xFFu);
  raw[offset + 2u] = (uint8_t)((value >> 16) & 0xFFu);
  raw[offset + 3u] = (uint8_t)((value >> 24) & 0xFFu);
}

void auth_msc_dir_entry_init(AuthMscDirEntry& entry, const char short_name[11], uint8_t attr, uint16_t first_cluster, uint32_t file_size) {
  memset(&entry, 0, sizeof(entry));
  memcpy(entry.name, short_name, 11);
  entry.attr = attr;
  write_dir_le16(entry, offsetof(AuthMscDirEntry, crtTime), AUTH_MSC_TIMESTAMP_TIME);
  write_dir_le16(entry, offsetof(AuthMscDirEntry, crtDate), AUTH_MSC_TIMESTAMP_DATE);
  write_dir_le16(entry, offsetof(AuthMscDirEntry, lstAccDate), AUTH_MSC_TIMESTAMP_DATE);
  write_dir_le16(entry, offsetof(AuthMscDirEntry, wrtTime), AUTH_MSC_TIMESTAMP_TIME);
  write_dir_le16(entry, offsetof(AuthMscDirEntry, wrtDate), AUTH_MSC_TIMESTAMP_DATE);
  write_dir_le16(entry, offsetof(AuthMscDirEntry, fstClusLO), first_cluster);
  write_dir_le32(entry, offsetof(AuthMscDirEntry, fileSize), file_size);
}

void auth_msc_short_name_to_string(const AuthMscDirEntry& entry, char* out, size_t out_size) {
  if (out == nullptr || out_size == 0) {
    return;
  }
  size_t pos = 0;
  for (uint8_t i = 0; i < 8 && pos + 1 < out_size; ++i) {
    if (entry.name[i] == ' ') break;
    out[pos++] = (char)entry.name[i];
  }
  const bool has_ext = entry.name[8] != ' ';
  if (has_ext && pos + 1 < out_size) {
    out[pos++] = '.';
  }
  for (uint8_t i = 8; i < 11 && pos + 1 < out_size; ++i) {
    if (entry.name[i] == ' ') break;
    out[pos++] = (char)entry.name[i];
  }
  out[pos] = '\0';
}

bool auth_msc_short_name_equals(const AuthMscDirEntry& entry, const char expected[11]) {
  return memcmp(entry.name, expected, 11) == 0;
}

bool auth_msc_dir_entry_is_real_file(const AuthMscDirEntry& entry) {
  if (entry.name[0] == 0x00 || entry.name[0] == 0xE5) {
    return false;
  }
  if ((entry.attr & AUTH_MSC_DIR_ATTR_LFN) == AUTH_MSC_DIR_ATTR_LFN) {
    return false;
  }
  if (entry.attr & (AUTH_MSC_DIR_ATTR_VOLUME | AUTH_MSC_DIR_ATTR_DIRECTORY)) {
    return false;
  }
  if (entry.name[0] == '.') {
    return false;
  }
  return true;
}

#endif
