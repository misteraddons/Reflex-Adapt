#include "../runtime/input_runtime_output_bridge.h"
#include "../auth/auth_status.h"
#include "../output_runtime_state.h"
#include "output_autodetect_support.h"

extern "C" bool can_run_usb_detection(void) {
  return is_auto_output_mode_selected() && autoDetectState == AUTO_STATE_IDLE;
}

uint16_t usb_detect_probe_device_version(void) {
  // Windows caches MS OS 1.0 support by VID/PID/bcdDevice. Keep Sonik's
  // probe layout, but bump the revision field for AUTO Stage 0 so Windows
  // will re-query index 0xEE instead of reusing a stale "no WCID" cache entry.
  uint16_t probe_revision = bcd_device_version.revision;
  probe_revision = (probe_revision < 0x3Fu) ? (probe_revision + 1u) : 0x3Eu;
  return (uint16_t)((probe_revision << 10) | (bcd_device_version.composite & 0x03FFu));
}

bool is_ps5_extended_feature_probe(uint8_t report_id) {
  switch (report_id) {
    // Extended Sony/DS4-style feature IDs beyond the minimal 0x03 + F0-F3 auth path.
    // If a PlayStation-family host asks for any of these after AUTO has already
    // resolved to the PS4-compatible descriptor, treat that as a best-effort
    // PS5 refinement instead of adding another reboot stage.
    case 0x02:
    case 0x05:
    case 0x08:
    case 0x09:
    case 0x0A:
    case 0x12:
    case 0x20:
    case 0x21:
    case 0x81:
    case 0x82:
    case 0x83:
    case 0x84:
    case 0x85:
    case 0xA3:
    case 0xA4:
    case 0xA5:
      return true;
    default:
      return false;
  }
}

void auto_promote_ps5_detection(uint8_t report_id, uint16_t reqlen) {
  (void)reqlen;
#if defined(ENABLE_OUTPUT_PS5)
  if (!is_auto_output_mode_selected() ||
      (autoDetectState != AUTO_STATE_PS4 &&
       autoDetectState != AUTO_STATE_PLAYSTATION) ||
      canonicalizeOutputMode(outputMode) != OUTPUT_PS4 ||
      !authOutputModeCanRun(OUTPUT_PS5) ||
      !is_ps5_extended_feature_probe(report_id)) {
    return;
  }

  // The generic probe can only identify a PlayStation-family host. Once the
  // PS4-compatible runtime sees PS5/extended feature traffic, promote into the
  // native P5General path and let the console re-enumerate cleanly.
  autoDetectState = AUTO_STATE_PS5;
  output_autodetect_transition_reason = AUTO_TRANSITION_PS4_EXTENDED_TO_PS5;
  auto_detect_trigger_reenum((uint32_t)OUTPUT_PS5);
#else
  (void)report_id;
#endif
}
