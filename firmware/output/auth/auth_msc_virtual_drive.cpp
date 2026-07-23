#include "../../product_config.h"

#if defined(ADAPT_OUTPUT_USB_DEVICE) && defined(ADAPT_HAS_USB_AUTH_SIDECAR)

#include "auth_msc_virtual_drive.h"

#include <Arduino.h>
#include <string.h>

#include "auth_msc_assets.h"
#include "auth_msc_content.h"
#include "auth_msc_fat12.h"
#include "auth_msc_latency_export.h"
#include "auth_msc_ps4_import.h"
#include "auth_msc_status_file.h"

namespace {


constexpr uint16_t kSectorSize = 512;
#ifndef AUTH_MSC_SECTOR_COUNT
  #if defined(PRODUCT_CLASSIC2USB)
    #define AUTH_MSC_SECTOR_COUNT 1024
  #else
    #define AUTH_MSC_SECTOR_COUNT 254
  #endif
#endif

constexpr uint32_t kSectorCount = AUTH_MSC_SECTOR_COUNT;
constexpr uint16_t kReservedSectors = 1;
constexpr uint16_t kFatSectors = (AUTH_MSC_SECTOR_COUNT > 340) ? 4 : 1;
constexpr uint16_t kRootDirSectors = 4;
constexpr uint16_t kRootEntryCount = (kRootDirSectors * kSectorSize) / 32;
constexpr uint32_t kFatStartSector = kReservedSectors;
constexpr uint32_t kRootStartSector = kFatStartSector + kFatSectors;
constexpr uint32_t kDataStartSector = kRootStartSector + kRootDirSectors;
constexpr uint16_t kClusterSize = kSectorSize;
constexpr uint16_t kClusterCount = (uint16_t)(kSectorCount - kDataStartSector);
constexpr uint16_t kPs4AuthDirClusterCount = 4;
constexpr uint16_t kPs4UploadClusterCount = (AUTH_MSC_SECTOR_COUNT > 340) ? 32 : 20;

constexpr char kPs4AuthHtmlShortName[11] = {'P','S','4','A','U','T','H',' ','H','T','M'};
constexpr char kOledShortName[11] = {'O','L','E','D',' ',' ',' ',' ','H','T','M'};
constexpr char kReadmeShortName[11] = {'R','E','A','D','M','E',' ',' ','T','X','T'};
constexpr char kDeviceShortName[11] = {'D','E','V','I','C','E',' ',' ','T','X','T'};
constexpr char kDownloaderShortName[11] = {'A','D','A','P','T','D','L',' ','I','N','I'};
constexpr char kLatencyStatusShortName[11] = {'L','A','T','S','T','A','T',' ','T','X','T'};
constexpr char kPs4AuthShortName[11] = {'P','S','4','A','U','T','H',' ',' ',' ',' '};
constexpr char kDotShortName[11] = {'.',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '};
constexpr char kDotDotShortName[11] = {'.','.',' ',' ',' ',' ',' ',' ',' ',' ',' '};

const uint16_t kWebhidClusterCount = (uint16_t)((auth_msc_webhid_html_len() + kClusterSize - 1) / kClusterSize);
const uint16_t kOledClusterCount = (uint16_t)((auth_msc_oled_html_len() + kClusterSize - 1) / kClusterSize);
const uint16_t kReadmeClusterCount = (uint16_t)((auth_msc_readme_text_len() + kClusterSize - 1) / kClusterSize);
const uint16_t kDownloaderClusterCount = (uint16_t)((auth_msc_downloader_ini_text_len() + kClusterSize - 1) / kClusterSize);
const uint16_t kLatencyCsvClusterCount = auth_msc_latency_csv_cluster_count();
const uint16_t kOledCluster = (uint16_t)(2 + kWebhidClusterCount);
const uint16_t kReadmeCluster = (uint16_t)(kOledCluster + kOledClusterCount);
const uint16_t kDownloaderCluster = (uint16_t)(kReadmeCluster + kReadmeClusterCount);
const uint16_t kStatusCluster = (uint16_t)(kDownloaderCluster + kDownloaderClusterCount);
const uint16_t kLatencyStatusCluster = (uint16_t)(kStatusCluster + 1);
const uint16_t kLatencyCsvStartCluster = (uint16_t)(kLatencyStatusCluster + 1);
const uint16_t kPs4AuthDirStartCluster = (uint16_t)(kLatencyCsvStartCluster + kLatencyCsvClusterCount);
const uint16_t kFirstFreeCluster = (uint16_t)(kPs4AuthDirStartCluster + kPs4AuthDirClusterCount);
const uint16_t kPs4UploadStartCluster = kFirstFreeCluster;
const uint16_t kFirstReservedCluster = (uint16_t)(kPs4UploadStartCluster + kPs4UploadClusterCount);

static_assert(kRootEntryCount >= 9, "Auth drive root directory is too small");

uint8_t g_fat[kFatSectors * kSectorSize];
uint8_t g_root[kRootDirSectors * kSectorSize];
uint8_t g_ps4auth_dir[kPs4AuthDirClusterCount * kClusterSize];
uint8_t g_ps4_upload_data[kPs4UploadClusterCount * kClusterSize];

inline uint32_t clusterToSector(uint16_t cluster) {
  return kDataStartSector + (uint32_t)(cluster - 2u);
}

inline uint8_t* ps4AuthDirClusterPtr(uint16_t cluster) {
  if (!auth_msc_cluster_in_range(cluster, kPs4AuthDirStartCluster, kPs4AuthDirClusterCount)) {
    return nullptr;
  }
  return &g_ps4auth_dir[(uint16_t)(cluster - kPs4AuthDirStartCluster) * kClusterSize];
}

inline const uint8_t* ps4AuthDirClusterPtrConst(uint16_t cluster) {
  return ps4AuthDirClusterPtr(cluster);
}

inline uint8_t* ps4UploadClusterPtr(uint16_t cluster) {
  if (!auth_msc_cluster_in_range(cluster, kPs4UploadStartCluster, kPs4UploadClusterCount)) {
    return nullptr;
  }
  return &g_ps4_upload_data[(uint16_t)(cluster - kPs4UploadStartCluster) * kClusterSize];
}

inline const uint8_t* ps4UploadClusterPtrConst(uint16_t cluster) {
  return ps4UploadClusterPtr(cluster);
}

inline AuthMscDirEntry* rootEntries() {
  return reinterpret_cast<AuthMscDirEntry*>(g_root);
}

bool copyClusteredFileSector(uint16_t cluster,
                             uint16_t startCluster,
                             uint16_t clusterCount,
                             const uint8_t* data,
                             size_t length,
                             uint8_t* out) {
  if (!auth_msc_cluster_in_range(cluster, startCluster, clusterCount)) {
    return false;
  }

  memset(out, 0, kSectorSize);
  const size_t offset = (size_t)(cluster - startCluster) * kClusterSize;
  if (offset < length) {
    const size_t remaining = length - offset;
    const size_t chunk = (remaining > kSectorSize) ? kSectorSize : remaining;
    memcpy(out, data + offset, chunk);
  }
  return true;
}

uint32_t fnv1a32(const void* data, size_t length) {
  const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data);
  uint32_t hash = 2166136261u;
  for (size_t i = 0; i < length; ++i) {
    hash ^= bytes[i];
    hash *= 16777619u;
  }
  return hash;
}

void virtualDriveRefreshStatus() {
  auth_msc_status_refresh();
  rootEntries()[4].fileSize = (uint32_t)auth_msc_status_len();
}

void virtualDriveRefreshLatency() {
  if (!auth_msc_latency_export_enabled()) {
    return;
  }
  auth_msc_latency_refresh();
  rootEntries()[6].fileSize = (uint32_t)auth_msc_latency_status_len();
  rootEntries()[5].fileSize = (uint32_t)auth_msc_latency_csv_len();
  memcpy(rootEntries()[5].name, auth_msc_latency_csv_short_name(), 11);
}

void setLastResultWithName(const char* format, const char* name, uint32_t size) {
  auth_msc_status_set_result_with_name(format, name, size);
  rootEntries()[4].fileSize = (uint32_t)auth_msc_status_len();
}

void setLastResultText(const char* name, const char* text) {
  auth_msc_status_set_result_text(name, text);
  rootEntries()[4].fileSize = (uint32_t)auth_msc_status_len();
}

void buildBootSector(uint8_t* boot) {
  memset(boot, 0, kSectorSize);
  boot[0] = 0xEB;
  boot[1] = 0x3C;
  boot[2] = 0x90;
  memcpy(&boot[3], "RFLXAUTH", 8);
  boot[11] = (uint8_t)(kSectorSize & 0xFFu);
  boot[12] = (uint8_t)(kSectorSize >> 8);
  boot[13] = 1;
  boot[14] = (uint8_t)(kReservedSectors & 0xFFu);
  boot[15] = (uint8_t)(kReservedSectors >> 8);
  boot[16] = 1;
  boot[17] = (uint8_t)(kRootEntryCount & 0xFFu);
  boot[18] = (uint8_t)(kRootEntryCount >> 8);
  boot[19] = (uint8_t)(kSectorCount & 0xFFu);
  boot[20] = (uint8_t)(kSectorCount >> 8);
  boot[21] = 0xF8;
  boot[22] = (uint8_t)(kFatSectors & 0xFFu);
  boot[23] = (uint8_t)(kFatSectors >> 8);
  boot[24] = 1;
  boot[26] = 1;
  boot[36] = 0x80;
  boot[38] = 0x29;
  boot[39] = 0x52;
  boot[40] = 0x41;
  boot[41] = 0x32;
  boot[42] = 0x10;
  memcpy(&boot[43], auth_msc_volume_label(), auth_msc_volume_label_len());
  memcpy(&boot[54], "FAT12   ", 8);
  boot[510] = 0x55;
  boot[511] = 0xAA;
}

void buildFat() {
  memset(g_fat, 0, sizeof(g_fat));
  auth_msc_fat12_set_entry(g_fat, sizeof(g_fat), 0, AUTH_MSC_FAT12_RESERVED0);
  auth_msc_fat12_set_entry(g_fat, sizeof(g_fat), 1, AUTH_MSC_FAT12_RESERVED1);

  for (uint16_t cluster = 2; cluster < (uint16_t)(kClusterCount + 2u); ++cluster) {
    auth_msc_fat12_set_entry(g_fat, sizeof(g_fat), cluster, 0x0FF7);
  }

  auth_msc_fat12_set_cluster_chain(g_fat, sizeof(g_fat), 2, kWebhidClusterCount);
  auth_msc_fat12_set_cluster_chain(g_fat, sizeof(g_fat), kOledCluster, kOledClusterCount);
  auth_msc_fat12_set_cluster_chain(g_fat, sizeof(g_fat), kReadmeCluster, kReadmeClusterCount);
  auth_msc_fat12_set_cluster_chain(g_fat, sizeof(g_fat), kDownloaderCluster, kDownloaderClusterCount);
  auth_msc_fat12_set_cluster_chain(g_fat, sizeof(g_fat), kStatusCluster, 1);
  if (auth_msc_latency_export_enabled()) {
    auth_msc_fat12_set_cluster_chain(g_fat, sizeof(g_fat), kLatencyStatusCluster, 1);
    auth_msc_fat12_set_cluster_chain(g_fat, sizeof(g_fat), kLatencyCsvStartCluster, kLatencyCsvClusterCount);
  }
  auth_msc_fat12_set_cluster_chain(g_fat, sizeof(g_fat), kPs4AuthDirStartCluster, kPs4AuthDirClusterCount);

  for (uint16_t i = 0; i < kPs4UploadClusterCount; ++i) {
    auth_msc_fat12_set_entry(g_fat, sizeof(g_fat), (uint16_t)(kPs4UploadStartCluster + i), 0);
  }
}

void buildRootDirectory() {
  uint8_t* root = g_root;
  memset(root, 0, sizeof(g_root));

  AuthMscDirEntry* entries = reinterpret_cast<AuthMscDirEntry*>(root);
  auth_msc_dir_entry_init(entries[0], auth_msc_volume_label(), AUTH_MSC_DIR_ATTR_VOLUME, 0, 0);
  auth_msc_dir_entry_init(entries[1], kPs4AuthHtmlShortName, AUTH_MSC_DIR_ATTR_ARCHIVE | AUTH_MSC_DIR_ATTR_READ_ONLY, 2, (uint32_t)(auth_msc_webhid_html_len()));
  auth_msc_dir_entry_init(entries[2], kOledShortName, AUTH_MSC_DIR_ATTR_ARCHIVE | AUTH_MSC_DIR_ATTR_READ_ONLY, kOledCluster, (uint32_t)(auth_msc_oled_html_len()));
  auth_msc_dir_entry_init(entries[3], kReadmeShortName, AUTH_MSC_DIR_ATTR_ARCHIVE | AUTH_MSC_DIR_ATTR_READ_ONLY, kReadmeCluster, (uint32_t)(auth_msc_readme_text_len()));
  auth_msc_dir_entry_init(entries[4], kDeviceShortName, AUTH_MSC_DIR_ATTR_ARCHIVE | AUTH_MSC_DIR_ATTR_READ_ONLY, kStatusCluster, 0);
  if (auth_msc_latency_export_enabled()) {
    auth_msc_dir_entry_init(entries[5], auth_msc_latency_csv_short_name(), AUTH_MSC_DIR_ATTR_ARCHIVE | AUTH_MSC_DIR_ATTR_READ_ONLY, kLatencyCsvStartCluster, 0);
    auth_msc_dir_entry_init(entries[6], kLatencyStatusShortName, AUTH_MSC_DIR_ATTR_ARCHIVE | AUTH_MSC_DIR_ATTR_READ_ONLY, kLatencyStatusCluster, 0);
  }
  auth_msc_dir_entry_init(entries[7], kPs4AuthShortName, AUTH_MSC_DIR_ATTR_DIRECTORY, kPs4AuthDirStartCluster, 0);
  auth_msc_dir_entry_init(entries[8], kDownloaderShortName, AUTH_MSC_DIR_ATTR_ARCHIVE | AUTH_MSC_DIR_ATTR_READ_ONLY, kDownloaderCluster, (uint32_t)(auth_msc_downloader_ini_text_len()));
}

void buildPs4AuthDirectory() {
  memset(g_ps4auth_dir, 0, sizeof(g_ps4auth_dir));

  AuthMscDirEntry* entries = reinterpret_cast<AuthMscDirEntry*>(g_ps4auth_dir);
  auth_msc_dir_entry_init(entries[0], kDotShortName, AUTH_MSC_DIR_ATTR_DIRECTORY, kPs4AuthDirStartCluster, 0);
  auth_msc_dir_entry_init(entries[1], kDotDotShortName, AUTH_MSC_DIR_ATTR_DIRECTORY, 0, 0);
}

void virtualDriveBuild() {
  memset(g_ps4_upload_data, 0, sizeof(g_ps4_upload_data));
  auth_msc_status_reset();
  auth_msc_ps4_import_reset();
  auth_msc_latency_reset();

  buildFat();
  buildRootDirectory();
  buildPs4AuthDirectory();
  virtualDriveRefreshStatus();
  virtualDriveRefreshLatency();
}

bool readFileToBuffer(uint16_t firstCluster, uint32_t fileSize, uint8_t* buffer, size_t capacity) {
  if (buffer == nullptr || fileSize == 0 || fileSize > capacity) {
    return false;
  }

  uint16_t cluster = firstCluster;
  uint32_t remaining = fileSize;
  uint32_t offset = 0;
  uint16_t guard = 0;

  while (remaining > 0 && cluster >= 2u && cluster < AUTH_MSC_FAT12_EOC && guard++ < kClusterCount) {
    const uint8_t* src = ps4UploadClusterPtrConst(cluster);
    if (src == nullptr) {
      return false;
    }
    const uint32_t chunk = (remaining > kClusterSize) ? kClusterSize : remaining;
    memcpy(buffer + offset, src, chunk);
    offset += chunk;
    remaining -= chunk;
    if (remaining == 0) {
      return true;
    }
    const uint16_t next = auth_msc_fat12_get_entry(g_fat, sizeof(g_fat), cluster);
    if (next >= AUTH_MSC_FAT12_EOC) {
      break;
    }
    cluster = next;
  }

  return false;
}

void fillPs4ImportFile(const AuthMscDirEntry& entry,
                       const char* name,
                       uint32_t metaFingerprint,
                       AuthMscPs4File& outFile) {
  outFile.first_cluster = entry.fstClusLO;
  outFile.file_size = entry.fileSize;
  outFile.meta_fingerprint = metaFingerprint;
  strncpy(outFile.name, name, sizeof(outFile.name) - 1u);
  outFile.name[sizeof(outFile.name) - 1u] = '\0';
}

bool readLastPs4AuthFile(AuthMscPs4File& outFile) {
  bool found = false;
  uint16_t cluster = kPs4AuthDirStartCluster;
  uint16_t guard = 0;
  AuthMscDirEntry lastEntry = {};
  char lastName[20] = "";

  while (cluster >= 2u && cluster < AUTH_MSC_FAT12_EOC && guard++ < kClusterCount) {
    const uint8_t* dirCluster = ps4AuthDirClusterPtrConst(cluster);
    if (dirCluster == nullptr) {
      break;
    }
    const AuthMscDirEntry* entries = reinterpret_cast<const AuthMscDirEntry*>(dirCluster);
    for (uint16_t i = 0; i < (kClusterSize / sizeof(AuthMscDirEntry)); ++i) {
      const AuthMscDirEntry& entry = entries[i];
      if (entry.name[0] == 0x00) {
        break;
      }
      if (!auth_msc_dir_entry_is_real_file(entry)) {
        continue;
      }
      lastEntry = entry;
      auth_msc_short_name_to_string(entry, lastName, sizeof(lastName));
      found = true;
    }

    const uint16_t next = auth_msc_fat12_get_entry(g_fat, sizeof(g_fat), cluster);
    if (next >= AUTH_MSC_FAT12_EOC) {
      break;
    }
    cluster = next;
  }

  if (!found) {
    return false;
  }

  const uint32_t metaFingerprint = fnv1a32(lastName, strlen(lastName)) ^
                                   ((uint32_t)lastEntry.fstClusLO << 16) ^
                                   lastEntry.fileSize;
  fillPs4ImportFile(lastEntry, lastName, metaFingerprint, outFile);
  return true;
}

bool readPs4AuthFileByName(const char expected[11], AuthMscPs4File& outFile) {
  uint16_t cluster = kPs4AuthDirStartCluster;
  uint16_t guard = 0;

  while (cluster >= 2u && cluster < AUTH_MSC_FAT12_EOC && guard++ < kClusterCount) {
    const uint8_t* dirCluster = ps4AuthDirClusterPtrConst(cluster);
    if (dirCluster == nullptr) {
      break;
    }
    const AuthMscDirEntry* entries = reinterpret_cast<const AuthMscDirEntry*>(dirCluster);
    for (uint16_t i = 0; i < (kClusterSize / sizeof(AuthMscDirEntry)); ++i) {
      const AuthMscDirEntry& entry = entries[i];
      if (entry.name[0] == 0x00) {
        break;
      }
      if (!auth_msc_dir_entry_is_real_file(entry) || !auth_msc_short_name_equals(entry, expected)) {
        continue;
      }

      char name[20] = "";
      auth_msc_short_name_to_string(entry, name, sizeof(name));
      const uint32_t metaFingerprint = fnv1a32(name, strlen(name)) ^
                                       ((uint32_t)entry.fstClusLO << 16) ^
                                       entry.fileSize;
      fillPs4ImportFile(entry, name, metaFingerprint, outFile);
      return true;
    }

    const uint16_t next = auth_msc_fat12_get_entry(g_fat, sizeof(g_fat), cluster);
    if (next >= AUTH_MSC_FAT12_EOC) {
      break;
    }
    cluster = next;
  }

  return false;
}

bool ps4ImportReadNamedFile(const char shortName[11], AuthMscPs4File* outFile, void*) {
  return outFile != nullptr && readPs4AuthFileByName(shortName, *outFile);
}

bool ps4ImportReadLastFile(AuthMscPs4File* outFile, void*) {
  return outFile != nullptr && readLastPs4AuthFile(*outFile);
}

bool ps4ImportReadFileData(const AuthMscPs4File* file, uint8_t* buffer, size_t capacity, void*) {
  return file != nullptr && readFileToBuffer(file->first_cluster, file->file_size, buffer, capacity);
}

void ps4ImportSetResultText(const char* name, const char* text, void*) {
  setLastResultText(name, text);
}

void ps4ImportSetResultWithName(const char* format, const char* name, uint32_t size, void*) {
  setLastResultWithName(format, name, size);
}

void virtualDriveProcessImport() {
  const AuthMscPs4ImportCallbacks callbacks = {
    ps4ImportReadNamedFile,
    ps4ImportReadLastFile,
    ps4ImportReadFileData,
    ps4ImportSetResultText,
    ps4ImportSetResultWithName,
  };
  auth_msc_ps4_import_process(callbacks, nullptr);
}

void virtualDriveReadSector(uint32_t lba, uint8_t* out) {
  memset(out, 0, kSectorSize);

  if (lba == 0) {
    buildBootSector(out);
    return;
  }

  if (auth_msc_lba_in_range(lba, kFatStartSector, kFatSectors)) {
    memcpy(out, &g_fat[(lba - kFatStartSector) * kSectorSize], kSectorSize);
    return;
  }

  if (auth_msc_lba_in_range(lba, kRootStartSector, kRootDirSectors)) {
    memcpy(out, &g_root[(lba - kRootStartSector) * kSectorSize], kSectorSize);
    return;
  }

  if (lba < kDataStartSector) {
    return;
  }

  const uint16_t cluster = (uint16_t)(2u + (lba - kDataStartSector));
  if (copyClusteredFileSector(cluster, 2, kWebhidClusterCount,
                              reinterpret_cast<const uint8_t*>(auth_msc_webhid_html()),
                              auth_msc_webhid_html_len(), out)) {
    return;
  }
  if (copyClusteredFileSector(cluster, kOledCluster, kOledClusterCount,
                              reinterpret_cast<const uint8_t*>(auth_msc_oled_html()),
                              auth_msc_oled_html_len(), out)) {
    return;
  }
  if (copyClusteredFileSector(cluster, kReadmeCluster, kReadmeClusterCount,
                              reinterpret_cast<const uint8_t*>(auth_msc_readme_text()),
                              auth_msc_readme_text_len(), out)) {
    return;
  }
  if (copyClusteredFileSector(cluster, kDownloaderCluster, kDownloaderClusterCount,
                              reinterpret_cast<const uint8_t*>(auth_msc_downloader_ini_text()),
                              auth_msc_downloader_ini_text_len(), out)) {
    return;
  }
  if (copyClusteredFileSector(cluster, kStatusCluster, 1,
                              reinterpret_cast<const uint8_t*>(auth_msc_status_text()),
                              auth_msc_status_len(), out)) {
    return;
  }
  if (auth_msc_latency_export_enabled()) {
    if (copyClusteredFileSector(cluster, kLatencyStatusCluster, 1,
                                reinterpret_cast<const uint8_t*>(auth_msc_latency_status_text()),
                                auth_msc_latency_status_len(), out)) {
      return;
    }
    if (copyClusteredFileSector(cluster, kLatencyCsvStartCluster, kLatencyCsvClusterCount,
                                auth_msc_latency_csv_data(), auth_msc_latency_csv_len(), out)) {
      return;
    }
  }

  const uint8_t* authDir = ps4AuthDirClusterPtrConst(cluster);
  if (authDir != nullptr) {
    memcpy(out, authDir, kSectorSize);
    return;
  }

  const uint8_t* upload = ps4UploadClusterPtrConst(cluster);
  if (upload != nullptr) {
    memcpy(out, upload, kSectorSize);
  }
}

void virtualDriveWriteSector(uint32_t lba, const uint8_t* in) {
  if (auth_msc_lba_in_range(lba, kFatStartSector, kFatSectors)) {
    memcpy(&g_fat[(lba - kFatStartSector) * kSectorSize], in, kSectorSize);
    return;
  }

  if (auth_msc_lba_in_range(lba, kRootStartSector, kRootDirSectors)) {
    memcpy(&g_root[(lba - kRootStartSector) * kSectorSize], in, kSectorSize);
    return;
  }

  if (lba < kDataStartSector) {
    return;
  }

  const uint16_t cluster = (uint16_t)(2u + (lba - kDataStartSector));
  uint8_t* authDir = ps4AuthDirClusterPtr(cluster);
  if (authDir != nullptr) {
    memcpy(authDir, in, kSectorSize);
    return;
  }

  uint8_t* upload = ps4UploadClusterPtr(cluster);
  if (upload != nullptr) {
    memcpy(upload, in, kSectorSize);
  }
}


}  // namespace

void auth_msc_virtual_drive_build() {
  virtualDriveBuild();
}

void auth_msc_virtual_drive_refresh_status() {
  virtualDriveRefreshStatus();
}

void auth_msc_virtual_drive_refresh_latency() {
  virtualDriveRefreshLatency();
}

void auth_msc_virtual_drive_process_import() {
  virtualDriveProcessImport();
}

void auth_msc_virtual_drive_read_sector(uint32_t lba, uint8_t* out) {
  virtualDriveReadSector(lba, out);
}

void auth_msc_virtual_drive_write_sector(uint32_t lba, const uint8_t* in) {
  virtualDriveWriteSector(lba, in);
}

uint32_t auth_msc_virtual_drive_sector_count() {
  return kSectorCount;
}

uint16_t auth_msc_virtual_drive_sector_size() {
  return kSectorSize;
}

#endif
