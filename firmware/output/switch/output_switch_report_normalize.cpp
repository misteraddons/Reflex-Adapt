#include "output_switch_report_normalize.h"

#include <string.h>

#include "output_switch_rumble.h"

namespace {

bool is_switch_out_report_id(uint8_t rid) {
  return rid == 0x01 || rid == 0x10 || rid == 0x11;
}

bool is_known_switch_subcommand(uint8_t subcmd) {
  switch (subcmd) {
    case 0x01:
    case 0x02:
    case 0x03:
    case 0x04:
    case 0x08:
    case 0x10:
    case 0x21:
    case 0x22:
    case 0x30:
    case 0x40:
    case 0x41:
    case 0x48:
      return true;
    default:
      return false;
  }
}

int prepend_switch_report_id(uint8_t rid, uint8_t* out, int copy_size, int out_cap) {
  if (!out || copy_size <= 0 || out_cap <= 1) return copy_size;
  int payload_size = copy_size;
  if (payload_size > (out_cap - 1)) payload_size = out_cap - 1;
  if (payload_size > 0) memmove(&out[1], out, payload_size);
  out[0] = rid;
  return payload_size + 1;
}

bool looks_like_stripped_switch_subcmd_01(const uint8_t* buf, int size) {
  if (!buf || size < 10) return false;
  const uint8_t counter = buf[0];
  const uint8_t subcmd = buf[9];
  return (counter <= 0x0F) && is_known_switch_subcommand(subcmd);
}

bool looks_like_stripped_switch_rumble_only(const uint8_t* buf, int size) {
  if (!buf || size < 9) return false;
  if (buf[0] > 0x0F) return false;

  bool lValid = (buf[1] & 0x03) == 0x00 && (buf[4] & 0x40) == 0x40;
  bool rValid = (buf[5] & 0x03) == 0x00 && (buf[8] & 0x40) == 0x40;
  if (lValid || rValid) return true;

  SwitchRumbleData r0 = {};
  SwitchRumbleData r1 = {};
  decodeSwitchRumbleValues(&buf[1], &r0);
  decodeSwitchRumbleValues(&buf[5], &r1);
  return (r0.low_band_amp > 0.0f || r0.high_band_amp > 0.0f ||
          r1.low_band_amp > 0.0f || r1.high_band_amp > 0.0f);
}

}

int normalize_switch_out_report(
    uint16_t report_id,
    uint8_t* report,
    int report_size,
    uint8_t* out,
    int out_cap) {
  if (!report || !out || out_cap <= 0 || report_size <= 0) {
    return 0;
  }

  int copy_size = report_size;
  if (copy_size > out_cap) copy_size = out_cap;
  memcpy(out, report, copy_size);

  uint8_t rid = static_cast<uint8_t>(report_id);
  if (rid != 0) {
    bool payload_has_id = (copy_size > 0 && out[0] == rid);
    if (!payload_has_id) {
      copy_size = prepend_switch_report_id(rid, out, copy_size, out_cap);
    }
  } else if (copy_size > 0 && !is_switch_out_report_id(out[0])) {
    if (looks_like_stripped_switch_subcmd_01(out, copy_size)) {
      copy_size = prepend_switch_report_id(0x01, out, copy_size, out_cap);
    } else if (looks_like_stripped_switch_rumble_only(out, copy_size)) {
      copy_size = prepend_switch_report_id(0x10, out, copy_size, out_cap);
    }
  }

  return copy_size;
}

bool is_switch_output_report(uint16_t report_id, const uint8_t* report, int report_size) {
  if (!report || report_size <= 0) {
    return false;
  }

  uint8_t source[100] = {};
  int source_size = report_size;
  if (source_size > static_cast<int>(sizeof(source))) source_size = sizeof(source);
  memcpy(source, report, source_size);

  uint8_t normalized[100] = {};
  int normalized_size = normalize_switch_out_report(
      report_id, source, source_size, normalized, sizeof(normalized));
  if (normalized_size <= 0) {
    return false;
  }

  uint8_t effective_report_id = normalized[0];
  if (effective_report_id == 0x80) {
    if (normalized_size < 2) return false;
    uint8_t usb_cmd = normalized[1];
    return (usb_cmd >= 0x01 && usb_cmd <= 0x05);
  }

  if (!is_switch_out_report_id(effective_report_id)) return false;

  if (effective_report_id == 0x01) {
    if (normalized_size < 11) return false;
    if (normalized[1] > 0x0F) return false;
    return true;
  }

  if (normalized_size < 10) return false;
  return normalized[1] <= 0x0F;
}
