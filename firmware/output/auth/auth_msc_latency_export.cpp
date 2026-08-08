#include "../../product_config.h"

#if defined(ADAPT_OUTPUT_USB_DEVICE) && defined(ADAPT_HAS_USB_AUTH_SIDECAR)

#include "auth_msc_latency_export.h"

#include <Arduino.h>
#include <string.h>

#if defined(ADAPT_ENABLE_LATENCY_TEST)
#include "../../platform/latency_test.h"
#endif

namespace {

constexpr uint16_t kSectorSize = 512;

#if defined(ADAPT_ENABLE_LATENCY_TEST)
#ifndef AUTH_MSC_LATENCY_CSV_CLUSTER_COUNT
  #if defined(PRODUCT_CLASSIC2USB)
    #define AUTH_MSC_LATENCY_CSV_CLUSTER_COUNT 48
  #else
    #define AUTH_MSC_LATENCY_CSV_CLUSTER_COUNT 206
  #endif
#endif

constexpr uint16_t kLatencyStatusClusterCount = 1;
constexpr uint16_t kLatencyCsvClusterCount = AUTH_MSC_LATENCY_CSV_CLUSTER_COUNT;
constexpr size_t kLatencyStatusCapacity = (size_t)kLatencyStatusClusterCount * kSectorSize;
constexpr size_t kLatencyCsvCapacity = (size_t)kLatencyCsvClusterCount * kSectorSize;

char g_latency_status_file[kLatencyStatusCapacity];
size_t g_latency_status_file_length = 0;
uint8_t g_latency_csv_buffer[kLatencyCsvCapacity];
size_t g_latency_csv_length = 0;
char g_latency_csv_short_name[11] = {'L','A','T','0','0','0','0','0','C','S','V'};
#else
constexpr uint16_t kLatencyCsvClusterCount = 0;
constexpr char kLatencyDisabledStatus[] =
  "Reflex Latency Export\r\n"
  "====================\r\n"
  "\r\n"
  "Latency export is unavailable in retail firmware.\r\n";
char g_latency_csv_short_name[11] = {'L','A','T',' ',' ',' ',' ',' ','C','S','V'};
#endif

#if defined(ADAPT_ENABLE_LATENCY_TEST)
void makeLatencyCsvShortName(char out[11], uint32_t stamp) {
  memset(out, ' ', 11);
  out[0] = 'L';
  out[1] = 'A';
  out[2] = 'T';
  const uint32_t clamped = stamp % 100000u;
  out[3] = (char)('0' + ((clamped / 10000u) % 10u));
  out[4] = (char)('0' + ((clamped / 1000u) % 10u));
  out[5] = (char)('0' + ((clamped / 100u) % 10u));
  out[6] = (char)('0' + ((clamped / 10u) % 10u));
  out[7] = (char)('0' + (clamped % 10u));
  out[8] = 'C';
  out[9] = 'S';
  out[10] = 'V';
}
#endif

}  // namespace

uint16_t auth_msc_latency_csv_cluster_count() {
  return kLatencyCsvClusterCount;
}

bool auth_msc_latency_export_enabled() {
#if defined(ADAPT_ENABLE_LATENCY_TEST)
  return true;
#else
  return false;
#endif
}

void auth_msc_latency_reset() {
#if defined(ADAPT_ENABLE_LATENCY_TEST)
  memset(g_latency_status_file, 0, sizeof(g_latency_status_file));
  memset(g_latency_csv_buffer, 0, sizeof(g_latency_csv_buffer));
  g_latency_status_file_length = 0;
  g_latency_csv_length = 0;
  makeLatencyCsvShortName(g_latency_csv_short_name, 0);
#endif
}

void auth_msc_latency_refresh() {
#if defined(ADAPT_ENABLE_LATENCY_TEST)
  g_latency_status_file_length = latencyTest.writeExportStatus(g_latency_status_file, sizeof(g_latency_status_file));

  memset(g_latency_csv_buffer, 0, sizeof(g_latency_csv_buffer));
  g_latency_csv_length = latencyTest.writeExportCsv(reinterpret_cast<char*>(g_latency_csv_buffer), kLatencyCsvCapacity);

  makeLatencyCsvShortName(g_latency_csv_short_name, latencyTest.exportFileStamp());
#endif
}

const char* auth_msc_latency_status_text() {
#if defined(ADAPT_ENABLE_LATENCY_TEST)
  return g_latency_status_file;
#else
  return kLatencyDisabledStatus;
#endif
}

size_t auth_msc_latency_status_len() {
#if defined(ADAPT_ENABLE_LATENCY_TEST)
  return g_latency_status_file_length;
#else
  return sizeof(kLatencyDisabledStatus) - 1u;
#endif
}

const uint8_t* auth_msc_latency_csv_data() {
#if defined(ADAPT_ENABLE_LATENCY_TEST)
  return g_latency_csv_buffer;
#else
  return nullptr;
#endif
}

size_t auth_msc_latency_csv_len() {
#if defined(ADAPT_ENABLE_LATENCY_TEST)
  return g_latency_csv_length;
#else
  return 0;
#endif
}

const char* auth_msc_latency_csv_short_name() {
  return g_latency_csv_short_name;
}

#endif
