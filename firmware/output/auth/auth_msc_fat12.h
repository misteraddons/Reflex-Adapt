#pragma once

#include <stddef.h>
#include <stdint.h>

constexpr uint16_t AUTH_MSC_FAT12_RESERVED0 = 0x0FF8;
constexpr uint16_t AUTH_MSC_FAT12_RESERVED1 = 0x0FFF;
constexpr uint16_t AUTH_MSC_FAT12_EOC = 0x0FFF;
constexpr uint8_t AUTH_MSC_DIR_ATTR_READ_ONLY = 0x01;
constexpr uint8_t AUTH_MSC_DIR_ATTR_HIDDEN = 0x02;
constexpr uint8_t AUTH_MSC_DIR_ATTR_SYSTEM = 0x04;
constexpr uint8_t AUTH_MSC_DIR_ATTR_VOLUME = 0x08;
constexpr uint8_t AUTH_MSC_DIR_ATTR_DIRECTORY = 0x10;
constexpr uint8_t AUTH_MSC_DIR_ATTR_ARCHIVE = 0x20;
constexpr uint8_t AUTH_MSC_DIR_ATTR_LFN = 0x0F;
constexpr uint16_t AUTH_MSC_TIMESTAMP_DATE = (uint16_t)(((2026 - 1980) << 9) | (4 << 5) | 18);
constexpr uint16_t AUTH_MSC_TIMESTAMP_TIME = (uint16_t)((12 << 11) | (0 << 5));

struct __attribute__((packed)) AuthMscDirEntry {
  uint8_t name[11];
  uint8_t attr;
  uint8_t ntres;
  uint8_t crtTimeTenth;
  uint16_t crtTime;
  uint16_t crtDate;
  uint16_t lstAccDate;
  uint16_t fstClusHI;
  uint16_t wrtTime;
  uint16_t wrtDate;
  uint16_t fstClusLO;
  uint32_t fileSize;
};

static_assert(sizeof(AuthMscDirEntry) == 32, "FAT directory entry size mismatch");

bool auth_msc_lba_in_range(uint32_t lba, uint32_t start, uint32_t count);
bool auth_msc_cluster_in_range(uint16_t cluster, uint16_t start, uint16_t count);
uint16_t auth_msc_fat12_get_entry(const uint8_t* fat, size_t fat_len, uint16_t cluster);
void auth_msc_fat12_set_entry(uint8_t* fat, size_t fat_len, uint16_t cluster, uint16_t value);
void auth_msc_fat12_set_cluster_chain(uint8_t* fat, size_t fat_len, uint16_t start_cluster, uint16_t cluster_count);
void auth_msc_dir_entry_init(AuthMscDirEntry& entry, const char short_name[11], uint8_t attr, uint16_t first_cluster, uint32_t file_size);
void auth_msc_short_name_to_string(const AuthMscDirEntry& entry, char* out, size_t out_size);
bool auth_msc_short_name_equals(const AuthMscDirEntry& entry, const char expected[11]);
bool auth_msc_dir_entry_is_real_file(const AuthMscDirEntry& entry);
