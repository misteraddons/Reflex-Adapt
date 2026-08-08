#pragma once

#include <stddef.h>
#include <stdint.h>

inline uint32_t ps4AuthReportCrc32(uint8_t report_id,
                                  const uint8_t* payload,
                                  size_t payload_len) {
  uint32_t crc = 0xFFFFFFFFu;
  const auto update = [&crc](uint8_t byte) {
    crc ^= byte;
    for (uint8_t bit = 0; bit < 8; ++bit) {
      crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
  };

  update(report_id);
  for (size_t index = 0; index < payload_len; ++index) {
    update(payload[index]);
  }
  return ~crc;
}

inline void writePs4AuthReportCrc(uint8_t report_id,
                                  uint8_t* payload,
                                  size_t crc_offset) {
  const uint32_t crc =
      ps4AuthReportCrc32(report_id, payload, crc_offset);
  payload[crc_offset + 0] = static_cast<uint8_t>(crc >> 0);
  payload[crc_offset + 1] = static_cast<uint8_t>(crc >> 8);
  payload[crc_offset + 2] = static_cast<uint8_t>(crc >> 16);
  payload[crc_offset + 3] = static_cast<uint8_t>(crc >> 24);
}
