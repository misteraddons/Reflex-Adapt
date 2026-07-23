#pragma once

// Internal AUTO/WebHID HID callback bridge. This header is included from
// out_usb.h after descriptors and AUTO-detect runtime helpers are available.

static bool handle_output_autodetect_hid_get_report(
  uint8_t report_id,
  hid_report_type_t report_type,
  uint8_t* buffer,
  uint16_t reqlen,
  uint16_t* response_len
) {
  const bool auto_detect_ps3_feature =
    can_run_usb_detection() &&
    report_type == HID_REPORT_TYPE_FEATURE &&
    (report_id == 0xEF || report_id == 0xF2 || report_id == 0xF5 || report_id == 0xF7 || report_id == 0xF8);

  if (!auto_detect_ps3_feature &&
      report_type == HID_REPORT_TYPE_FEATURE &&
      report_id >= 0xE0 &&
      report_id <= 0xEF) {
    *response_len = webhid_get_report(report_id, buffer, reqlen);
    return true;
  }

  if (can_run_usb_detection() && outputMode != OUTPUT_PS4) {
    usb_detect_handle_hid_get(report_id, report_type, buffer, reqlen);
    if (report_type == HID_REPORT_TYPE_FEATURE) {
      switch (report_id) {
        case 0x00: {
          const uint16_t len = min(reqlen, (uint16_t) sizeof(ps3_magic_bytes));
          memcpy(buffer, ps3_magic_bytes, len);
          *response_len = len;
          return true;
        }
        case 0x01: {
          const uint16_t len = min(reqlen, (uint16_t) sizeof(report_01));
          memcpy(buffer, report_01, len);
          *response_len = len;
          return true;
        }
        case 0x03: {
          static const uint8_t auto_detect_ps4_f03[47] = {
            0x21, 0x27, 0x04, 0x91, 0x01, 0x2c, 0x56, 0xa0,
            0x0f, 0x3d, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00,
            0x20, 0x0d, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
          };
          const uint16_t len = min(reqlen, (uint16_t) sizeof(auto_detect_ps4_f03));
          memcpy(buffer, auto_detect_ps4_f03, len);
          *response_len = len;
          return true;
        }
        case 0xEF: {
          const uint16_t len = min(reqlen, (uint16_t) sizeof(report_ef));
          memcpy(buffer, report_ef, len);
          *response_len = len;
          return true;
        }
        case 0xF2: {
          const uint16_t len = min(reqlen, (uint16_t) sizeof(report_f2));
          memcpy(buffer, report_f2, len);
          *response_len = len;
          return true;
        }
        case 0xF5: {
          const uint16_t len = min(reqlen, (uint16_t) sizeof(report_f5));
          memcpy(buffer, report_f5, len);
          *response_len = len;
          return true;
        }
        case 0xF7: {
          const uint16_t len = min(reqlen, (uint16_t) sizeof(report_f7));
          memcpy(buffer, report_f7, len);
          *response_len = len;
          return true;
        }
        case 0xF8: {
          const uint16_t len = min(reqlen, (uint16_t) sizeof(report_f8));
          memcpy(buffer, report_f8, len);
          *response_len = len;
          return true;
        }
        default:
          break;
      }
    }

    // Match Sonik's exact AUTO probe behavior: returning -1 here wraps the
    // current TinyUSB xfer length back to 0 for nonzero report IDs and stalls
    // the request instead of sending a bogus 1-byte feature response.
    *response_len = (uint16_t)-1;
    return true;
  }

  return false;
}

static bool handle_output_autodetect_hid_set_report(
  uint8_t report_id,
  hid_report_type_t report_type,
  uint8_t const* buffer,
  uint16_t bufsize
) {
  const bool auto_detect_ps3_feature =
    can_run_usb_detection() &&
    report_type == HID_REPORT_TYPE_FEATURE &&
    (report_id == 0xEF || report_id == 0xF5);

  if (!auto_detect_ps3_feature &&
      report_type == HID_REPORT_TYPE_FEATURE &&
      report_id >= 0xE0 &&
      report_id <= 0xEF) {
    webhid_set_report(report_id, buffer, bufsize);
    return true;
  }

  if (can_run_usb_detection()) {
    usb_detect_handle_hid_set(report_id, report_type, buffer, bufsize);
    return true;
  }

  return false;
}
