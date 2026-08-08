#pragma once

// Internal HID get/set report handlers for the USB device output stack.
// This stays header-only so it can reuse the existing output report state,
// auth helpers, and autodetect hooks that still live in out_usb.h.

#include "../auth/ps_auth_dongle_runtime.h"

uint16_t hid_device_get_report_callback(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
  // Debug tracking
  webhid_get_report_count++;
  webhid_last_report_id = report_id;
  webhid_last_report_type = (uint8_t)report_type;

  uint16_t autodetect_response = 0;
  if (handle_output_autodetect_hid_get_report(report_id, report_type, buffer, reqlen, &autodetect_response)) {
    return autodetect_response;
  }

  const outputMode_t effectiveOutputMode = get_effective_output_mode();
  switch (effectiveOutputMode) {
    case OUTPUT_PS4:
      if (report_type == HID_REPORT_TYPE_FEATURE) {
        auto_promote_ps5_detection(report_id, reqlen);
        const bool useStoredPs4Keys =
          (effectiveOutputMode == OUTPUT_PS4) && ps4Auth.isReady();
        if (!useStoredPs4Keys) {
          const uint16_t dongleResponse =
            ps_auth_dongle_handle_get_report(report_id, buffer, reqlen);
          if (dongleResponse != 0) {
            return dongleResponse;
          }
        }
        switch (report_id) {
          case 0x03: {
            static const uint8_t ps4_f03[47] = {
              0x21, 0x27, 0x04, 0x91, 0x01, 0x2c, 0x56, 0xa0,
              0x0f, 0x3d, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00,
              0x20, 0x0d, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
            };
            memcpy(buffer, ps4_f03, min(reqlen, (uint16_t)sizeof(ps4_f03)));
            return reqlen;
          }
          case 0xF0:
            memset(buffer, 0, reqlen);
            return reqlen;
          case 0xF1:
            return useStoredPs4Keys ? ps4Auth.getAuthPage(buffer, reqlen) : 0;
          case 0xF2:
            return useStoredPs4Keys ? ps4Auth.getSigningState(buffer, reqlen) : 0;
          case 0xF3:
            return useStoredPs4Keys ? ps4Auth.getResetConfig(buffer, reqlen) : 0;
        }
      }
      break;

    #ifdef ENABLE_OUTPUT_PS5
    case OUTPUT_PS5:
      if (report_type == HID_REPORT_TYPE_FEATURE) {
        return ps_auth_dongle_handle_get_report(report_id, buffer, reqlen);
      }
      break;
    #endif

    case OUTPUT_SWITCHPRO:
      break;

    case OUTPUT_PANTHERLORD:
      break;

    case OUTPUT_GCWIIU:
      break;

    case OUTPUT_MDMINI:
      break;

    default:
      break;
  }
  return 0;
}

void hid_device_set_report_callback(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) {
  if (handle_output_autodetect_hid_set_report(report_id, report_type, buffer, bufsize)) {
    return;
  }

  switch (get_effective_output_mode()) {
    case OUTPUT_HID:
    case OUTPUT_MISTER:
      if (report_type == HID_REPORT_TYPE_OUTPUT) {
        const uint8_t sourcePort = output_usb_source_port_for_hid_interface(itf);
        if (report_id == 2 && bufsize >= 2)
          rumble_callback(sourcePort, buffer[0], buffer[1]);
        else if (report_id == 0 && bufsize >= 3 && buffer[0] == 2)
          rumble_callback(sourcePort, buffer[1], buffer[2]);
      }
      break;
    case OUTPUT_PS4:
      if (report_type == HID_REPORT_TYPE_FEATURE) {
        auto_promote_ps5_detection(report_id);
      }
      if (report_type == HID_REPORT_TYPE_FEATURE && report_id == 0xF0) {
        const outputMode_t effectiveOutputMode = get_effective_output_mode();
        const bool useStoredPs4Keys =
          (effectiveOutputMode == OUTPUT_PS4) && ps4Auth.isReady();
        if (useStoredPs4Keys) {
          ps4Auth.receiveNoncePage(buffer, bufsize);
        } else {
          ps_auth_dongle_handle_set_report(report_id, buffer, bufsize);
        }
      } else if (report_type == HID_REPORT_TYPE_OUTPUT && report_id == 0 &&
                 bufsize >= 6 && buffer[0] == 0x05) {
        rumble_callback(0, buffer[5], buffer[4]);
      }
      break;

    #ifdef ENABLE_OUTPUT_PS5
    case OUTPUT_PS5:
      if (report_type == HID_REPORT_TYPE_FEATURE) {
        ps_auth_dongle_handle_set_report(report_id, buffer, bufsize);
      }
      break;
    #endif

    case OUTPUT_SWITCHPRO:
      if (bufsize > 0) {
        requestControllerFrameDelivery(itf);
        hid_report_data_callback(itf, switchpro[itf]->switchCommon, report_id, (uint8_t *)buffer, bufsize);
      }
      break;

    case OUTPUT_PANTHERLORD:
      if (report_type == HID_REPORT_TYPE_OUTPUT && bufsize == 4) {
        uint8_t left = buffer[2];
        uint8_t right = buffer[3];
        rumble_callback(itf, left, right);
      }
      break;

    case OUTPUT_GCWIIU:
      if (report_type == HID_REPORT_TYPE_OUTPUT) {
        if ((report_id == 0 && bufsize >= 1 && buffer[0] == 0x13) || (report_id == 0x13)) {
          gcwiiu_initialized = true;
        }
        const uint8_t* rumbleData = nullptr;
        if (report_id == 0 && bufsize >= 5 && buffer[0] == 0x11) {
          rumbleData = &buffer[1];
        } else if (report_id == 0x11 && bufsize >= 4) {
          rumbleData = buffer;
        }
        if (rumbleData != nullptr) {
          rumble_callback(0, 0, rumbleData[0] ? 1 : 0);
          rumble_callback(1, 0, rumbleData[1] ? 1 : 0);
          rumble_callback(2, 0, rumbleData[2] ? 1 : 0);
          rumble_callback(3, 0, rumbleData[3] ? 1 : 0);
        }
      }
      break;

    default:
      break;
  }
}
