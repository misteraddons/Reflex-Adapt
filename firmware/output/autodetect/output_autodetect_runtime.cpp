#include "../../product_config.h"
#ifdef ADAPT_OUTPUT_USB_DEVICE

#include <Adafruit_TinyUSB.h>
#include <Arduino.h>

#include "../../core/firmware_support.h"
#if defined(ENABLE_INPUT_AUTODETECT)
#include "../../input/autodetect/input_autodetect_runtime.h"
#endif
#if defined(ENABLE_USB_AUTH_SIDECAR) || defined(ENABLE_INPUT_USB)
#include "../../input/usb_host/input_usb_host_service.h"
#endif
#include "../auth/auth_status.h"
#include "../auth/ps_auth_dongle_runtime.h"
#include "../output_runtime_state.h"
#include "output_autodetect_rp2040_usb_debug.h"
#include "output_autodetect_runtime.h"
#include "output_autodetect_support.h"
#include "usb_detect.h"
#include "../xinput/out_xinput.h"
#include "../xinput/out_xinput_multi.h"

// HOST DETECTION WARNING:
// This AUTO path is intentionally narrow and timing-sensitive. Windows,
// MiSTer/Linux, Switch, PS3, and Xbox-family hosts overlap at the USB
// descriptor/control-request level, so "harmless" extra probe stages or longer
// generic timeouts can break another host. The generic probe must get one fast
// RetroZord-style pass, then settle to the generic DInput fallback unless the
// host has produced an explicit console signal. PS3 assist is a secondary pass,
// not the default path. Reverting this toward the old 5s generic wait caused
// Windows to hang on the OLED "Detecting..." screen.
//
// Do not change these constants, stage order, or fallback conditions without
// benching at least Windows, MiSTer/Linux, Switch, PS3, and Xbox-family AUTO
// detection and documenting the result in the commit.
constexpr uint16_t kGenericNoMountResolveMs = 1000;
constexpr uint16_t kPs3AssistResolveMs = 900;

bool auto_detect_is_final_state() {
  if (autoDetectState == AUTO_STATE_XBOX_360) {
    return xinput_is_authenticated();
  }
  if (autoDetectState == AUTO_STATE_PLAYSTATION) {
    return false;
  }
  return autoDetectState != AUTO_STATE_IDLE;
}

static bool ps5_probe_has_native_host_traffic() {
#if defined(ENABLE_OUTPUT_PS5)
  const PsAuthDongleStatus status = ps_auth_dongle_status();
  // The basic 0x03/F0/F1/F2 report train is shared with PS4 auth, so it is not
  // enough to prove the host is PS5. Keep the PS5 probe alive only after a
  // PS5-style extended feature request or a completed signed-report submit.
  return is_ps5_extended_feature_probe(status.last_host_get_id) ||
         is_ps5_extended_feature_probe(status.last_host_set_id) ||
         status.ps5_host_submit_count != 0;
#else
  return false;
#endif
}

static bool playstation_auth_sidecar_has_usb_activity() {
#if defined(ENABLE_OUTPUT_PS5) && (defined(ENABLE_USB_AUTH_SIDECAR) || defined(ENABLE_INPUT_USB))
  const PsAuthDongleStatus auth = ps_auth_dongle_status();
  if (auth.connected || auth.last_vid != 0 || auth.last_pid != 0) {
    return true;
  }

  const UsbHostServiceStatus host = usb_host_service_status();
  return host.root_connected ||
         host.root_device_present ||
         host.root_device_connected ||
         host.root_device_enumerated ||
         host.addr1_vid_pid_valid ||
         host.root_device_vid != 0 ||
         host.root_device_pid != 0;
#else
  return false;
#endif
}

static bool ps5_probe_should_fallback_to_ps4(uint32_t elapsed_ms) {
#if defined(ENABLE_OUTPUT_PS5)
  if (playstation_auth_sidecar_has_usb_activity()) {
    return elapsed_ms >= 15000;
  }
  return elapsed_ms >= 5000;
#else
  (void)elapsed_ms;
  return true;
#endif
}

static uint32_t playstation_family_fallback_ms() {
  return playstation_auth_sidecar_has_usb_activity() ? 15000u : 5000u;
}

static uint8_t saturateProbeNibble(uint16_t value) {
  return (value > 0x0Fu) ? 0x0Fu : (uint8_t)value;
}

static void update_auto_detect_probe_counters() {
  XInputDebugInfo info = {};
  xinput_get_debug_info(&info);
  output_autodetect_probe_counters =
    ((uint16_t)saturateProbeNibble(info.desc_device_calls) << 12) |
    ((uint16_t)saturateProbeNibble(info.desc_config_calls) << 8) |
    ((uint16_t)saturateProbeNibble(info.interfaces_opened) << 4) |
    (uint16_t)saturateProbeNibble(info.request_count);
}

static void sync_xinput_descriptor_hints() {
  if (xinput_seen_string_0xEE) {
    host_detection.ms_os_string_read = true;
    usb_detect_mark_result(DETECT_WINDOWS);
  }
  if (xinput_seen_string_4) {
    // Sonik's working detector treats the Xbox security-interface string as a
    // 360 hint even before the full XSM3 request train starts.
    host_detection.vendor_setup_seen = true;
  }
}

void auto_detect_hid_report_read_cb(uint8_t itf) {
  (void) itf;
}

extern "C" bool tud_hid_set_idle_cb(uint8_t instance, uint8_t idle_rate) {
  (void) instance;
  (void) idle_rate;
  return true;
}

extern "C" void tud_string_0xee_requested_cb(void) {
}

static void auto_detect_preserve_auto_input_before_output_reenum() {
#if defined(ENABLE_INPUT_AUTODETECT)
  if (savedDeviceMode != RZORD_AUTODETECT) {
    return;
  }

  DeviceEnum modeToPreserve = deviceMode;
  if (modeToPreserve == RZORD_AUTODETECT || modeToPreserve == RZORD_NONE) {
    // Output AUTO may reboot before the Home screen while input AUTO is still
    // unresolved. Do not run controller probes from the host-detect path.
    preserveDetectedInputModeForReboot(RZORD_NONE);
    return;
  }
  preserveDetectedInputModeForReboot(modeToPreserve);
#endif
}

void auto_detect_trigger_reenum(uint32_t next_output_mode) {
  autoDetectProbeStage = AUTO_PROBE_GENERIC;
  auto_detect_persist_debug_snapshot(output_autodetect_last_flags, output_autodetect_aux_flags);
  auto_detect_persist_runtime_mode((outputMode_t)next_output_mode, autoDetectState);
  auto_detect_preserve_auto_input_before_output_reenum();
  reboot();
}

bool auto_detect_preserve_current_runtime_mode_for_input_reboot() {
  if (auto_detect_preserve_resolved_runtime_mode_for_reboot()) {
    return true;
  }

  if (!is_auto_output_mode_selected() ||
      autoDetectState != AUTO_STATE_IDLE ||
      canonicalizeOutputMode(outputMode) != OUTPUT_MISTER) {
    return false;
  }

  const bool genericMounted =
      tud_mounted() &&
      usb_detect_has_mounted_generic_hid_activity() &&
      host_detection.any == 0 &&
      !host_detection.ms_os_string_read &&
      !host_detection.vendor_setup_seen &&
      !host_detection.ps3_feature_seen &&
      !host_detection.cleared_input &&
      !host_detection.cleared_output;
  if (!genericMounted) {
    return false;
  }

  autoDetectProbeStage = AUTO_PROBE_GENERIC;
  autoDetectState = AUTO_STATE_FALLBACK_HID;
  output_autodetect_last_flags = host_detection.any | DETECT_TIMEOUT;
  output_autodetect_aux_flags = usb_detect_get_aux_flags();
  auto_detect_persist_debug_snapshot(output_autodetect_last_flags, output_autodetect_aux_flags);
  auto_detect_persist_runtime_mode(OUTPUT_MISTER, autoDetectState);
  return true;
}

static void auto_detect_trigger_probe_stage(AutoDetectProbeStage next_stage) {
  autoDetectProbeStage = next_stage;
  auto_detect_persist_debug_snapshot(output_autodetect_last_flags, output_autodetect_aux_flags);
  auto_detect_persist_probe_stage(next_stage);
  reboot();
}

static void auto_detect_trigger_xinput_output() {
  // XSM3 means retail Xbox 360, not Windows. Resolve to the dedicated
  // single-controller OUTPUT_XINPUT path so the security interface and auth
  // state machine match the donor controller shape.
  usb_detect_mark_result(DETECT_XBOX360);
  autoDetectState = AUTO_STATE_XBOX_360;
  auto_detect_trigger_reenum((uint32_t)OUTPUT_XINPUT);
}

static bool auto_detect_promote_windows_xinput_to_xbox360() {
#if !defined(ENABLE_AUTO_XBOX360_LATE_PROMOTION)
  // Windows XInput2P can produce auth-like control traffic after AUTO has
  // already resolved Windows. Do not reinterpret that as Xbox 360; Xbox 360
  // should be selected only by the initial host detector unless this path is
  // explicitly enabled for console testing.
  return false;
#else
  if (!is_auto_output_mode_selected() ||
      autoDetectState != AUTO_STATE_WINDOWS ||
      canonicalizeOutputMode(outputMode) != OUTPUT_XINPUT2P ||
      !xinput_multi_console_auth_observed()) {
    return false;
  }

  // If the generic probe resolved early as Windows but the console later sends
  // XSM3 security traffic, relabel as Xbox and reboot into the donor-style
  // single-controller auth transport. Do not keep running on XInput2P here:
  // XInput2P is only the Windows two-player path.
  autoDetectState = AUTO_STATE_XBOX_360;
  output_autodetect_last_flags = host_detection.any | DETECT_XBOX360;
  output_autodetect_aux_flags = usb_detect_get_aux_flags();
  if (outputMode != OUTPUT_XINPUT) {
    auto_detect_trigger_reenum((uint32_t)OUTPUT_XINPUT);
  }
  return true;
#endif
}

static void auto_detect_trigger_fallback_hid() {
  const bool probeRuntimeActive = can_run_usb_detection();
  autoDetectProbeStage = AUTO_PROBE_GENERIC;
  autoDetectState = AUTO_STATE_FALLBACK_HID;
  output_autodetect_last_flags = host_detection.any | DETECT_TIMEOUT;
  output_autodetect_aux_flags = usb_detect_get_aux_flags();
  if (canonicalizeOutputMode(outputMode) != OUTPUT_MISTER || probeRuntimeActive) {
    auto_detect_trigger_reenum((uint32_t)OUTPUT_MISTER);
  }
}

static void auto_detect_trigger_ps3_hid_fallback() {
  const bool probeRuntimeActive = can_run_usb_detection();
  autoDetectProbeStage = AUTO_PROBE_GENERIC;
  autoDetectState = AUTO_STATE_PS3;
  output_autodetect_last_flags = host_detection.any | DETECT_PS3;
  output_autodetect_aux_flags = usb_detect_get_aux_flags();
  if (outputMode != OUTPUT_PS3 || probeRuntimeActive) {
    auto_detect_trigger_reenum((uint32_t)OUTPUT_PS3);
  }
}

static void auto_detect_trigger_ps4_output() {
  if (!authOutputModeCanRun(OUTPUT_PS4)) {
    auto_detect_trigger_fallback_hid();
    return;
  }

  autoDetectProbeStage = AUTO_PROBE_GENERIC;
  autoDetectState = AUTO_STATE_PS4;
  if (outputMode != OUTPUT_PS4) {
    auto_detect_trigger_reenum((uint32_t)OUTPUT_PS4);
  }
}

static void auto_detect_trigger_ps5_probe_output() {
#if defined(ENABLE_OUTPUT_PS5)
  if (!authOutputModeCanRun(OUTPUT_PS5)) {
    auto_detect_trigger_ps4_output();
    return;
  }

  autoDetectProbeStage = AUTO_PROBE_GENERIC;
  autoDetectState = AUTO_STATE_PS5;
  output_autodetect_transition_reason = AUTO_TRANSITION_PS4_EXTENDED_TO_PS5;
  if (outputMode != OUTPUT_PS5) {
    auto_detect_trigger_reenum((uint32_t)OUTPUT_PS5);
  }
#else
  auto_detect_trigger_ps4_output();
#endif
}

static void auto_detect_trigger_playstation_family_output() {
#if defined(PRODUCT_CLASSIC2USB)
  // Classic2USB has no PS auth-provider sidecar, so PlayStation-family AUTO
  // should settle directly into the PS4-compatible path.
  auto_detect_trigger_ps4_output();
  return;
#endif

  // RetroZord truth reports only DETECT_PS4 for PlayStation-family hosts. Start
  // with the PS4-compatible runtime so PS4 never briefly appears as PS5, then
  // promote only if PS5-specific feature traffic is observed.
  if (!authOutputModeCanRun(OUTPUT_PS4)) {
    auto_detect_trigger_fallback_hid();
    return;
  }

  autoDetectProbeStage = AUTO_PROBE_GENERIC;
  autoDetectState = AUTO_STATE_PLAYSTATION;
  if (outputMode != OUTPUT_PS4) {
    auto_detect_trigger_reenum((uint32_t)OUTPUT_PS4);
  }
}

static void auto_detect_trigger_windows_output(outputMode_t automode_windows_output) {
  const bool probeRuntimeActive = can_run_usb_detection();
  autoDetectProbeStage = AUTO_PROBE_GENERIC;
  autoDetectState = AUTO_STATE_WINDOWS;
  if (canonicalizeOutputMode(outputMode) != canonicalizeOutputMode(automode_windows_output) ||
      probeRuntimeActive) {
    auto_detect_trigger_reenum((uint32_t)automode_windows_output);
  }
}

extern "C" void auto_detect_process() {
  static uint32_t ps5_probe_started_at_ms = 0;
  static uint32_t playstation_family_started_at_ms = 0;

#if defined(ENABLE_OUTPUT_PS5)
  if (is_auto_output_mode_selected() &&
      autoDetectState == AUTO_STATE_PS5 &&
      canonicalizeOutputMode(outputMode) == OUTPUT_PS5) {
    const uint32_t ms_now = millis();
    if (ps5_probe_started_at_ms == 0) {
      ps5_probe_started_at_ms = ms_now;
    }
    const uint32_t elapsed_ms = ms_now - ps5_probe_started_at_ms;
    output_autodetect_last_ms = (elapsed_ms > 0xFFFFu)
      ? 0xFFFFu
      : (uint16_t)elapsed_ms;

    if (authOutputModeHasProvider(OUTPUT_PS5) ||
        ps5_probe_has_native_host_traffic()) {
      return;
    }

    if (ps5_probe_should_fallback_to_ps4(elapsed_ms)) {
      autoDetectState = AUTO_STATE_PS4;
      output_autodetect_transition_reason = AUTO_TRANSITION_PS5_PROBE_TO_PS4;
      auto_detect_trigger_reenum((uint32_t)OUTPUT_PS4);
    }
    return;
  }
#endif

  ps5_probe_started_at_ms = 0;

  if (is_auto_output_mode_selected() &&
      autoDetectState == AUTO_STATE_PLAYSTATION &&
      canonicalizeOutputMode(outputMode) == OUTPUT_PS4) {
    const uint32_t ms_now = millis();
    if (playstation_family_started_at_ms == 0) {
      playstation_family_started_at_ms = ms_now;
    }
    const uint32_t elapsed_ms = ms_now - playstation_family_started_at_ms;
    output_autodetect_last_ms = (elapsed_ms > 0xFFFFu)
      ? 0xFFFFu
      : (uint16_t)elapsed_ms;

#if defined(ENABLE_OUTPUT_PS5)
    if (authOutputModeHasProvider(OUTPUT_PS5) ||
        ps5_probe_has_native_host_traffic()) {
      auto_detect_trigger_ps5_probe_output();
      return;
    }
#endif

    if (elapsed_ms >= playstation_family_fallback_ms()) {
      autoDetectState = AUTO_STATE_PS4;
      auto_detect_persist_runtime_mode(outputMode, autoDetectState);
    }
    return;
  }

  playstation_family_started_at_ms = 0;

  if (auto_detect_promote_windows_xinput_to_xbox360()) {
    return;
  }

  if (!can_run_usb_detection())
    return;

  const outputMode_t automode_windows_output = get_configured_windows_auto_output_mode();
  // Use RetroZord's host-detect probe, then enumerate as Switch Pro for the
  // final controller personality.
  const outputMode_t automode_switch1_output = OUTPUT_SWITCHPRO;
  static uint32_t auto_detect_started_at_ms = 0;

  if (auto_detect_started_at_ms == 0) {
    auto_detect_started_at_ms = millis();
  }

  uint32_t ms_now = millis();
  uint32_t auto_detect_elapsed_ms = ms_now - auto_detect_started_at_ms;
  output_autodetect_last_ms = (auto_detect_elapsed_ms > 0xFFFFu)
    ? 0xFFFFu
    : (uint16_t)auto_detect_elapsed_ms;
  sync_xinput_descriptor_hints();
  output_autodetect_last_flags = host_detection.any |
    ((autoDetectState == AUTO_STATE_FALLBACK_HID) ? DETECT_TIMEOUT : 0);
  output_autodetect_aux_flags = usb_detect_get_aux_flags();
  update_auto_detect_probe_counters();

#if defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
  update_output_autodetect_rp2040_usb_debug(ms_now, auto_detect_elapsed_ms);
#endif

  if (autoDetectProbeStage != AUTO_PROBE_GENERIC &&
      autoDetectProbeStage != AUTO_PROBE_PS3) {
    // Older builds could leave XID/XInput staged probes in watchdog scratch.
    // Keep only the generic detector plus the delayed PS3 assist stage.
    autoDetectProbeStage = AUTO_PROBE_GENERIC;
  }

  const uint8_t detected_host = usb_host_detection_task();

  if (detected_host == DETECT_NONE &&
      autoDetectProbeStage == AUTO_PROBE_PS3 &&
      auto_detect_elapsed_ms >= kPs3AssistResolveMs) {
    output_autodetect_transition_reason = AUTO_TRANSITION_GENERIC_NO_MOUNT_TO_FALLBACK;
    auto_detect_trigger_fallback_hid();
    return;
  }

  if (detected_host == DETECT_NONE &&
      autoDetectProbeStage == AUTO_PROBE_GENERIC &&
      !tud_mounted() &&
      auto_detect_elapsed_ms >= kGenericNoMountResolveMs) {
    if (tud_connected()) {
      // PS3 often needs the Sony-shaped discriminator descriptor before it
      // asks for the feature reports that identify it. Only connected hosts
      // that did not mount the generic probe get this assist stage.
      output_autodetect_transition_reason = AUTO_TRANSITION_GENERIC_PS3_ASSIST;
      auto_detect_trigger_probe_stage(AUTO_PROBE_PS3);
      return;
    } else {
      output_autodetect_transition_reason = AUTO_TRANSITION_GENERIC_NO_CONNECT_TO_FALLBACK;
    }
    auto_detect_trigger_fallback_hid();
    return;
  }

  switch (detected_host) {
    case DETECT_NONE:
      break;

    case DETECT_PS3:
      auto_detect_trigger_ps3_hid_fallback();
      break;

    case DETECT_WINDOWS:
      auto_detect_trigger_windows_output(automode_windows_output);
      break;

    case DETECT_PS4:
      auto_detect_trigger_playstation_family_output();
      break;

    case DETECT_OGXBOX:
      autoDetectProbeStage = AUTO_PROBE_GENERIC;
      autoDetectState = AUTO_STATE_OG_XBOX;
      if (outputMode != OUTPUT_XID) {
        auto_detect_trigger_reenum((uint32_t)OUTPUT_XID);
      }
      break;

    case DETECT_XBOX360:
      auto_detect_trigger_xinput_output();
      break;

    case DETECT_SWITCH1:
    case DETECT_SWITCH2:
      autoDetectProbeStage = AUTO_PROBE_GENERIC;
      autoDetectState = AUTO_STATE_SWITCH;
      if (outputMode != automode_switch1_output) {
        auto_detect_trigger_reenum((uint32_t)automode_switch1_output);
      }
      break;

    case DETECT_TIMEOUT:
      if (autoDetectProbeStage == AUTO_PROBE_GENERIC) {
        output_autodetect_transition_reason = AUTO_TRANSITION_GENERIC_PS3_ASSIST;
        auto_detect_trigger_probe_stage(AUTO_PROBE_PS3);
        return;
      }
      auto_detect_trigger_fallback_hid();
      break;

    default:
      auto_detect_trigger_fallback_hid();
      break;
  }
}

#else

#include "../output_runtime_state.h"

bool auto_detect_preserve_current_runtime_mode_for_input_reboot() {
  return auto_detect_preserve_resolved_runtime_mode_for_reboot();
}

#endif  // ADAPT_OUTPUT_USB_DEVICE
