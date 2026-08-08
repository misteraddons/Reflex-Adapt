#pragma once

// Internal USB begin/setup helpers. This remains header-only so the helpers can
// reuse the existing TinyUSB objects and runtime flags still owned by out_usb.h.

#include "../auth/auth_storage.h"
#include "../auth/ps_auth_dongle_runtime.h"
#include "output_usb_player_count.h"

static void prepare_manual_tinyusb_output_init_runtime() {
#if defined(ARDUINO_ARCH_MBED) && defined(ARDUINO_ARCH_RP2040)
  // Manual begin() is required on core without built-in support for TinyUSB such as mbed rp2040.
  TinyUSB_Device_Init(0);
#endif
}

static void begin_output_dreamcast_debug_cdc_runtime() {
#if defined(ENABLE_INPUT_DREAMCAST) && DREAMCAST_SERIAL_DEBUG
  bool dc_cdc_enable = false;
  if (deviceMode == RZORD_DREAMCAST) {
#if DREAMCAST_DEBUG_FORCE_CDC
    dc_cdc_enable = true;
#else
    dc_cdc_enable = use_device_hid_class && (outputMode == OUTPUT_HID || outputMode == OUTPUT_MISTER);
#endif
  }

  if (dc_cdc_enable) {
    dreamcastDebugCdc.begin(115200);
    dreamcastDebugCdc.setStringDescriptor("Reflex Dreamcast Debug");
    dreamcastDebugCdcEnabled = true;
  }
#endif
}

static void begin_output_hid_interfaces_runtime() {
  if (outputMode == OUTPUT_GCWIIU) {
    usb_device_hid[0]->setReportCallback(hid_device_get_report_callback_arr[0], hid_device_set_report_callback_arr[0]);
    usb_device_hid[0]->begin();
    hid_begin_success_count = 1;
  } else {
    hid_begin_success_count = 0;
    hid_first_fail_index = 255;
    const uint8_t outputPlayers = output_usb_player_count();
    for (uint8_t i = 0; i < outputPlayers; ++i) {
      if (!usb_device_hid[i]) {
        continue;
      }
      usb_device_hid[i]->setReportCallback(hid_device_get_report_callback_arr[i], hid_device_set_report_callback_arr[i]);
      if (usb_device_hid[i]->begin()) {
        hid_begin_success_count++;
      } else if (hid_first_fail_index == 255) {
        hid_first_fail_index = i;
      }
    }
  }

  if (can_run_usb_detection() && _usb_detect_xinput_auth) {
    _usb_detect_xinput_auth->begin();
  }

  if (outputMode == OUTPUT_PS4
      #ifdef ENABLE_OUTPUT_PS5
      || outputMode == OUTPUT_PS5
      #endif
      ) {
    ps_auth_dongle_set_output_mode(outputMode);
    if (outputMode == OUTPUT_PS4) {
      ps4Auth.begin(auth_storage_active_payload_address(AUTH_KEY_TYPE_PS4));
    }
  }
}

static void begin_output_non_hid_interfaces_runtime() {
  const outputMode_t effectiveOutputMode = get_effective_output_mode();
  if (effectiveOutputMode == OUTPUT_XINPUT) {
    _xinput->begin();
  } else if (effectiveOutputMode == OUTPUT_XINPUT2P) {
  #ifdef FORCE_XINPUT2P_SINGLE_DRIVER
    _xinput->begin();
  #elif defined(ENABLE_XINPUT2P_WIRELESS_TRANSPORT)
    _xinputw->begin();
  #else
    _xinput2p->begin();
  #endif
  } else if (effectiveOutputMode == OUTPUT_XINPUTW) {
    _xinputw->begin();
  } else if (effectiveOutputMode == OUTPUT_XID) {
    _xid->begin();
  #ifdef ENABLE_EXPERIMENTAL_XBOXONE_OUTPUT
  } else if (effectiveOutputMode == OUTPUT_XBOXONE) {
    _xboxone->begin();
  #endif
  }
}

static void begin_output_usb_interfaces_runtime() {
  if (use_device_hid_class) {
    begin_output_hid_interfaces_runtime();
  } else {
    begin_output_non_hid_interfaces_runtime();
  }
}
