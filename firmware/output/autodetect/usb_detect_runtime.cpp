#include "../../product_config.h"
#ifdef ADAPT_OUTPUT_USB_DEVICE

#include <Arduino.h>

#include "usb_detect.h"
#include "usb_detect_descriptor_data.h"
#include "../output_runtime_state.h"
#include "../xinput/out_xinput.h"

// Host detection follows Santroller's public console_detection.md notes rather
// than copied Santroller source. Santroller is GPL-3.0; keep docs/NOTICE.md and the
// license copy in third_party/licenses/ in sync if this logic changes.
// https://github.com/Santroller/Santroller/blob/8057fac61242d6430e5666d47502c894ab88baa8/docs/reverse_engineering/console_detection.md

#ifdef __cplusplus
extern "C" {
#endif
bool can_run_usb_detection(void);
#ifdef __cplusplus
}
#endif

uint16_t const* usb_detect_handle_ms_os_10(void) {
  host_detection.ms_os_string_read = true;
  if (can_run_usb_detection())
    return (uint16_t const*)ms_os_string_desc;
  else
    return NULL;
}

void usb_detect_handle_manufacturer_read(void) {
  if (can_run_usb_detection())
    host_detection.string_manufacturer_read = true;
}

void usb_detect_handle_descriptor_read(void) {
  if (can_run_usb_detection())
    host_detection.hid_descriptor_report_read = true;
}

void usb_detect_handle_edpt_clear_stall(uint8_t ep_addr) {
  if (can_run_usb_detection()) {
    // Match RetroZord's detector truth: Switch clears the generic probe HID
    // endpoints. Treating any IN+OUT clear as Switch is too broad on Windows.
    host_detection.cleared_input  |= ep_addr == 0x81;
    host_detection.cleared_output |= ep_addr == 0x01;
  }
}

bool usb_detect_has_mounted_generic_hid_activity(void) {
  return host_detection.string_manufacturer_read &&
         host_detection.hid_descriptor_report_read;
}

static const uint8_t kPs3FeatureF2 = 0x01;
static const uint8_t kPs3FeatureF5 = 0x02;
static const uint8_t kPs3FeatureSetF5 = 0x04;

static bool usb_detect_ps3_feature_allowed() {
  // In the generic probe, a real PS3 can read descriptors before asking for
  // the DS3 feature train. In the Sony-VID PS3 assist probe, Linux/MiSTer binds
  // hid-sony, reads the manufacturer string, then asks the same feature reports.
  return autoDetectProbeStage != AUTO_PROBE_PS3 ||
         !host_detection.string_manufacturer_read;
}

static bool usb_detect_ps3_feature_train_complete() {
  const uint8_t ps3_get_train = kPs3FeatureF2 | kPs3FeatureF5;
  return ((host_detection.ps3_feature_mask & ps3_get_train) == ps3_get_train) ||
         ((host_detection.ps3_feature_mask & kPs3FeatureSetF5) != 0);
}

static void usb_detect_note_ps3_feature(uint8_t report_id, bool is_set_report) {
  host_detection.ps3_feature_seen = true;
  switch (report_id) {
    case 0xF2:
      host_detection.ps3_feature_mask |= kPs3FeatureF2;
      break;
    case 0xF5:
      host_detection.ps3_feature_mask |= kPs3FeatureF5;
      if (is_set_report)
        host_detection.ps3_feature_mask |= kPs3FeatureSetF5;
      break;
    default:
      break;
  }
  host_detection_last_signal_ms = millis();
  if (usb_detect_ps3_feature_allowed() && usb_detect_ps3_feature_train_complete()) {
    usb_detect_mark_result(DETECT_PS3);
  }
}

static void usb_detect_reject_linux_ps3_probe() {
  if (autoDetectProbeStage == AUTO_PROBE_PS3 &&
      (host_detection.any & DETECT_PS3) &&
      host_detection.string_manufacturer_read) {
    host_detection.any &= (uint8_t)~DETECT_PS3;
  }
}

void usb_detect_handle_hid_get(uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
  (void)buffer;
  (void)reqlen;
  if (!can_run_usb_detection() || report_type != HID_REPORT_TYPE_FEATURE)
    return;

  if (report_id == 0x03) {
    usb_detect_mark_result(DETECT_PS4);
    return;
  }

  switch (report_id) {
    case 0xEF:
    case 0xF2:
    case 0xF5:
    case 0xF7:
    case 0xF8:
      usb_detect_note_ps3_feature(report_id, false);
      break;
    default:
      break;
  }
}

void usb_detect_handle_hid_set(uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
  (void)buffer;
  (void)bufsize;
  if (!can_run_usb_detection() || report_type != HID_REPORT_TYPE_FEATURE)
    return;

  switch (report_id) {
    case 0xEF:
    case 0xF5:
      usb_detect_note_ps3_feature(report_id, true);
      break;
    default:
      break;
  }
}

bool usb_detect_handle_vendor_control_xfer(uint8_t rhport, uint8_t stage, const tusb_control_request_t *request) {
  if (!can_run_usb_detection())
    return false;

  if (request->bmRequestType_bit.type == TUSB_REQ_TYPE_VENDOR &&
      request->bRequest >= 0x81 &&
      request->bRequest <= 0x87) {
    usb_detect_mark_result(DETECT_XBOX360);
    return xinput_auth_handle_control(rhport, stage, request);
  }

  if (stage != CONTROL_STAGE_SETUP)
    return true;

  host_detection.vendor_setup_seen = true;

  if (request->bmRequestType == 0xC0 && request->bRequest == 0x20) {
    usb_detect_mark_result(DETECT_WINDOWS);
    return tud_control_xfer(rhport, request, (void *)MS_CompatIDDescriptor, sizeof(MS_CompatIDDescriptor));
  }

  if (request->bmRequestType == 0xC1 && request->bRequest == 0x06 && request->wValue == 0x4200) {
    usb_detect_mark_result(DETECT_OGXBOX);
    return false;
  }

  if (request->bmRequestType_bit.type == TUSB_REQ_TYPE_VENDOR) {
    usb_detect_mark_result(DETECT_XBOX360);
  }

  return false;
}

uint8_t usb_detect_get_aux_flags(void) {
  uint8_t flags = 0;
  if (host_detection.string_manufacturer_read)
    flags |= 0x01;
  if (host_detection.hid_descriptor_report_read)
    flags |= 0x02;
  if (host_detection.cleared_input)
    flags |= 0x04;
  if (host_detection.cleared_output)
    flags |= 0x08;
  if (host_detection.ms_os_string_read)
    flags |= 0x10;
  if (host_detection.vendor_setup_seen)
    flags |= 0x20;
  if (host_detection.not_wiiu)
    flags |= 0x40;
  if (host_detection.ps3_feature_seen)
    flags |= 0x80;
  return flags;
}

static uint8_t usb_detect_resolve_host_flags(uint8_t flags) {
  // HOST DETECTION WARNING: PS3, PS4, Xbox, and Xbox 360 have been
  // hardware-validated with this priority and timing. DO NOT TOUCH HOST
  // DETECTION CODE WITHOUT ASKING. These host signals overlap; PS4 can read
  // the same descriptors Windows does before asking for feature report 0x03,
  // so bit position must not decide host priority.
  if (flags & DETECT_XBOX360)
    return DETECT_XBOX360;
  if (flags & DETECT_OGXBOX)
    return DETECT_OGXBOX;
  if (flags & DETECT_PS3)
    return DETECT_PS3;
  if (flags & DETECT_PS4)
    return DETECT_PS4;
  if (flags & DETECT_SWITCH1)
    return DETECT_SWITCH1;
  if (flags & DETECT_SWITCH2)
    return DETECT_SWITCH2;
  if (flags & DETECT_WINDOWS)
    return DETECT_WINDOWS;
  return DETECT_NONE;
}

uint8_t usb_host_detection_task(void) {
  static bool fully_initialized = false;
  static uint32_t started_at_ms = 0;

  const uint16_t non_wiiu_timeout_ms  = 250;
  const uint16_t ps4_feature_grace_ms = 900;
  const uint16_t post_detect_resolve_delay_ms = 20;
  const uint8_t detect_explicit = DETECT_PS3 | DETECT_PS4 | DETECT_OGXBOX | DETECT_XBOX360 | DETECT_SWITCH1 | DETECT_SWITCH2 | DETECT_WINDOWS;
  const uint8_t detect_premount = DETECT_PS3 | DETECT_OGXBOX | DETECT_XBOX360;
#ifdef USB_DETECT_ENABLE_WIIU
  const uint16_t wiiu_timeout_ms      = 2100;
#endif

  if (fully_initialized || !can_run_usb_detection())
    return DETECT_NONE;

  const uint32_t ms_now = millis();

  if ((host_detection.any & detect_premount) &&
      (uint32_t)(ms_now - host_detection_last_signal_ms) >= post_detect_resolve_delay_ms) {
    fully_initialized = true;
    return usb_detect_resolve_host_flags(host_detection.any);
  }

  if (!tud_mounted())
    return DETECT_NONE;

  if (started_at_ms == 0)
    started_at_ms = ms_now;

  const bool mounted_hid_generic_activity =
    usb_detect_has_mounted_generic_hid_activity();
  const bool mounted_hid_ps4_feature_probe_active =
    mounted_hid_generic_activity &&
    host_detection.any == 0 &&
    !host_detection.ms_os_string_read &&
    !host_detection.vendor_setup_seen &&
    !host_detection.ps3_feature_seen &&
    !host_detection.cleared_input &&
    !host_detection.cleared_output;

  usb_detect_reject_linux_ps3_probe();

  if (host_detection.cleared_input && host_detection.cleared_output &&
      !mounted_hid_generic_activity &&
      !host_detection.ms_os_string_read &&
      !host_detection.vendor_setup_seen &&
      !(host_detection.any & DETECT_SWITCH1)) {
    host_detection.any |= DETECT_SWITCH1;
    host_detection_last_signal_ms = ms_now;
  }

#ifdef USB_DETECT_ENABLE_WIIU
  if (host_detection.string_manufacturer_read || host_detection.hid_descriptor_report_read)
    host_detection.not_wiiu = true;
  else if (!host_detection.any && !host_detection.string_manufacturer_read && !host_detection.hid_descriptor_report_read && (ms_now - started_at_ms) > wiiu_timeout_ms)
    host_detection.any |= DETECT_WIIU;
#endif

#ifdef USB_DETECT_ENABLE_WIIU
  const bool wiiu_timeout_ready = (ms_now - started_at_ms) > wiiu_timeout_ms;
#endif
  const uint32_t ms_since_last_signal = ms_now - host_detection_last_signal_ms;
  const bool explicit_detect_ready =
    (host_detection.any & detect_explicit) &&
    ms_since_last_signal >= post_detect_resolve_delay_ms;
  const bool ps4_feature_grace_ready =
    !mounted_hid_ps4_feature_probe_active ||
    (ms_now - started_at_ms) > ps4_feature_grace_ms;
  const bool no_signal_timeout_ready =
    host_detection.any == 0 &&
    ps4_feature_grace_ready &&
    (ms_now - started_at_ms) > non_wiiu_timeout_ms;

#ifdef USB_DETECT_ENABLE_WIIU
  if ((host_detection.not_wiiu && (explicit_detect_ready || no_signal_timeout_ready)) || wiiu_timeout_ready)
#else
  if (explicit_detect_ready || no_signal_timeout_ready)
#endif
  {
    fully_initialized = true;

    if (host_detection.any == 0) {
      if (host_detection.hid_descriptor_report_read && !host_detection.string_manufacturer_read)
        return DETECT_SWITCH2;
      else
        return DETECT_TIMEOUT;
    }

    return usb_detect_resolve_host_flags(host_detection.any);
  }

  return DETECT_NONE;
}

#endif  // ADAPT_OUTPUT_USB_DEVICE
