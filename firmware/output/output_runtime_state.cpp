#include "output_runtime_state.h"

#include "../menu/menu_runtime_state.h"

namespace {

constexpr uint32_t kAutoDetectScratchOutputMask = 0xFFu;
constexpr uint32_t kAutoDetectScratchStateShift = 8u;

bool isPersistedAutoDetectStateValid(uint8_t raw) {
  return raw > (uint8_t)AUTO_STATE_IDLE &&
         raw <= (uint8_t)AUTO_STATE_PLAYSTATION;
}

}  // namespace

outputMode_t configuredOutputMode = OUTPUT_AUTO;
outputMode_t outputMode = OUTPUT_MISTER;
socdMode_t socdMode = SOCD_OFF;
bool use_device_hid_class = false;
uint8_t hid_begin_success_count = 0;
uint8_t hid_first_fail_index = 255;

AutoDetectProbeStage autoDetectProbeStage = AUTO_PROBE_GENERIC;
AutoDetectState autoDetectState = AUTO_STATE_IDLE;

uint16_t output_autodetect_last_ms = 0;
uint8_t output_autodetect_last_flags = 0;
uint8_t output_autodetect_aux_flags = 0;
uint8_t output_autodetect_usb_hw_flags = 0;
uint8_t output_autodetect_usb_irq_observed = 0;
uint8_t output_autodetect_usb_irq_flags = 0;
uint16_t output_autodetect_probe_counters = 0;
uint8_t output_autodetect_transition_reason = AUTO_TRANSITION_NONE;

bool is_n64_switch_nso_quirk_active() {
#ifdef ENABLE_INPUT_N64
  return outputMode == OUTPUT_SWITCHPRO && deviceMode == RZORD_N64;
#else
  return false;
#endif
}

bool input_mode_supports_nso_special(DeviceEnum mode) {
  switch (mode) {
    #ifdef ENABLE_INPUT_NES
    case RZORD_NES:
    #endif
    #ifdef ENABLE_INPUT_SNES
    case RZORD_SNES:
    #endif
    #ifdef ENABLE_INPUT_N64
    case RZORD_N64:
    #endif
    #ifdef ENABLE_INPUT_GAMECUBE
    case RZORD_GAMECUBE:
    #endif
    #ifdef ENABLE_INPUT_VBOY
    case RZORD_VBOY:
    #endif
      return true;
    default:
      return false;
  }
}

bool is_nso_special_active() {
  return (outputMode == OUTPUT_SWITCHPRO &&
          nso_special &&
          input_mode_supports_nso_special(deviceMode)) ||
         is_n64_switch_nso_quirk_active();
}

outputMode_t canonicalizeOutputMode(outputMode_t mode) {
  if (mode == OUTPUT_RESERVED_JOGCON) {
    return OUTPUT_MISTER_JOGCON;
  }
  if (mode == OUTPUT_RESERVED_MOUSE) {
    return OUTPUT_MISTER;
  }
  if (mode == OUTPUT_XINPUTW) {
    return OUTPUT_XINPUT2P;
  }
  return mode;
}

bool is_xinput2p_output_enabled() {
#ifdef ENABLE_EXPERIMENTAL_XINPUT2P_OUTPUT
  return true;
#else
  return false;
#endif
}

outputMode_t sanitizeRuntimeOutputMode(outputMode_t mode) {
  mode = canonicalizeOutputMode(mode);
  #if defined(ADAPT_PRIMARY_EGRESS_CLASSIC_CONSOLE)
  if (mode == OUTPUT_CONSOLE_NES || mode == OUTPUT_CONSOLE_SNES) {
    return mode;
  }
  return OUTPUT_CONSOLE_SNES;
  #elif defined(ADAPT_PRIMARY_EGRESS_JVS_BOARD)
  (void)mode;
  return OUTPUT_JVS;
  #elif defined(ADAPT_PRIMARY_EGRESS_DB15)
  (void)mode;
  return OUTPUT_DB15_SUPERGUN;
  #else
  #if defined(PRODUCT_CLASSIC2USB) && defined(FORCE_CLASSIC_XINPUT2P_OUTPUT) && \
      defined(ENABLE_EXPERIMENTAL_XINPUT2P_OUTPUT)
  (void)mode;
  return OUTPUT_XINPUT2P;
  #endif
  #if defined(PRODUCT_CLASSIC2USB) && defined(FORCE_CLASSIC_XINPUT_OUTPUT) && \
      defined(ENABLE_EXPERIMENTAL_XINPUT2P_OUTPUT)
  (void)mode;
  return OUTPUT_XINPUT;
  #endif
  #if defined(PRODUCT_CLASSIC2USB) && defined(FORCE_CLASSIC_MANAGEMENT_OUTPUT)
  (void)mode;
  return OUTPUT_MISTER;
  #endif
  if (mode == OUTPUT_XINPUT) {
    return OUTPUT_XINPUT;
  }
  if (mode == OUTPUT_XINPUT2P && !is_xinput2p_output_enabled()) {
    return OUTPUT_MISTER;
  }
  #if defined(PRODUCT_CLASSIC2USB)
  if (mode == OUTPUT_PS5) {
    return OUTPUT_PS4;
  }
  #endif
  if (mode == OUTPUT_XBOXONE) {
    #if defined(PRODUCT_CLASSIC2USB)
    return OUTPUT_MISTER;
    #else
    #ifdef ENABLE_EXPERIMENTAL_XBOXONE_OUTPUT
    return OUTPUT_XBOXONE;
    #else
    return OUTPUT_MISTER;
    #endif
    #endif
  }
  if (mode == OUTPUT_AUTO) {
    return OUTPUT_MISTER;
  }
  return mode;
  #endif
}

outputMode_t get_configured_windows_auto_output_mode() {
  switch (menu_win_output) {
    case 0:
      return OUTPUT_HID;
    case 2:
      return OUTPUT_KEYBOARD;
    case 1:
    default:
      if (!is_xinput2p_output_enabled()) {
        return OUTPUT_HID;
      }
      return OUTPUT_XINPUT2P;
  }
}

bool output_supports_dpad_buttons(outputMode_t mode) {
  return mode == OUTPUT_HID || mode == OUTPUT_MISTER;
}

bool is_ps5_timing_quiet_mode_active() {
#ifdef ENABLE_OUTPUT_PS5
  return outputMode == OUTPUT_PS5;
#else
  return false;
#endif
}

void auto_detect_clear_scratch_state() {
  watchdog_hw->scratch[AUTO_DETECT_SCRATCH_MAGIC] = 0;
  watchdog_hw->scratch[AUTO_DETECT_SCRATCH_STATE] = 0;
  auto_detect_clear_input_scratch_state();
  watchdog_hw->scratch[AUTO_PROBE_SCRATCH_STAGE] = 0;
  watchdog_hw->scratch[AUTO_PROBE_SCRATCH_AUX] = 0;
  output_autodetect_probe_counters = 0;
  output_autodetect_transition_reason = AUTO_TRANSITION_NONE;
}

void auto_detect_clear_input_scratch_state() {
  watchdog_hw->scratch[AUTO_INPUT_MODE_SCRATCH_MAGIC] = 0;
  watchdog_hw->scratch[AUTO_INPUT_MODE_SCRATCH_MODE] = 0;
}

void auto_detect_persist_probe_stage(AutoDetectProbeStage stage) {
  watchdog_hw->scratch[AUTO_PROBE_SCRATCH_STAGE] = (uint32_t)stage;
  watchdog_hw->scratch[AUTO_DETECT_SCRATCH_STATE] =
    ((uint32_t)output_autodetect_transition_reason << 24);
}

void auto_detect_persist_debug_snapshot(uint8_t flags, uint8_t aux_flags) {
  watchdog_hw->scratch[AUTO_PROBE_SCRATCH_AUX] =
    (uint32_t)flags |
    ((uint32_t)aux_flags << 8) |
    ((uint32_t)output_autodetect_probe_counters << 16);
}

void auto_detect_restore_debug_snapshot() {
  uint32_t snapshot = watchdog_hw->scratch[AUTO_PROBE_SCRATCH_AUX];
  output_autodetect_last_flags = (uint8_t)(snapshot & 0xFFu);
  output_autodetect_aux_flags = (uint8_t)((snapshot >> 8) & 0xFFu);
  output_autodetect_probe_counters = (uint16_t)((snapshot >> 16) & 0xFFFFu);
}

AutoDetectProbeStage auto_detect_load_probe_stage() {
  const uint32_t raw = watchdog_hw->scratch[AUTO_PROBE_SCRATCH_STAGE];
  if (raw == (uint32_t)AUTO_PROBE_PS3) {
    return AUTO_PROBE_PS3;
  }
  // Ignore stale XID/XInput staged-probe scratch values from older
  // experimental builds so AUTO starts from the RetroZord-style generic pass.
  return AUTO_PROBE_GENERIC;
}

uint8_t auto_detect_load_transition_reason() {
  return (uint8_t)((watchdog_hw->scratch[AUTO_DETECT_SCRATCH_STATE] >> 24) & 0xFFu);
}

outputMode_t auto_detect_probe_output_mode(AutoDetectProbeStage stage) {
  switch (stage) {
    case AUTO_PROBE_PS3:
      return OUTPUT_PS3;
    default:
      return OUTPUT_MISTER;
  }
}

bool is_auto_output_mode_selected() {
  return configuredOutputMode == OUTPUT_AUTO;
}

bool auto_detect_has_resolved_runtime_mode() {
  return configuredOutputMode == OUTPUT_AUTO &&
         autoDetectState != AUTO_STATE_IDLE;
}

void auto_detect_persist_runtime_mode(outputMode_t mode, AutoDetectState state) {
  watchdog_hw->scratch[AUTO_DETECT_SCRATCH_MAGIC] = AUTO_DETECT_MAGIC;
  watchdog_hw->scratch[AUTO_DETECT_SCRATCH_STATE] =
    (uint32_t)sanitizeRuntimeOutputMode(mode) |
    ((uint32_t)state << kAutoDetectScratchStateShift) |
    ((uint32_t)output_autodetect_transition_reason << 24);
}

outputMode_t auto_detect_load_persisted_runtime_mode() {
  if (watchdog_hw->scratch[AUTO_DETECT_SCRATCH_MAGIC] != AUTO_DETECT_MAGIC) {
    return OUTPUT_AUTO;
  }
  return sanitizeRuntimeOutputMode(
    (outputMode_t)(watchdog_hw->scratch[AUTO_DETECT_SCRATCH_STATE] &
                   kAutoDetectScratchOutputMask));
}

AutoDetectState auto_detect_state_for_output_mode(outputMode_t mode) {
  switch (sanitizeRuntimeOutputMode(mode)) {
    case OUTPUT_PS3:
      return AUTO_STATE_PS3;
    case OUTPUT_XINPUT:
      return AUTO_STATE_XBOX_360;
    case OUTPUT_KEYBOARD:
    case OUTPUT_XINPUTW:
    case OUTPUT_XINPUT2P:
      return AUTO_STATE_WINDOWS;
    case OUTPUT_PS4:
      return AUTO_STATE_PS4;
    case OUTPUT_PS5:
      return AUTO_STATE_PS5;
    case OUTPUT_XBOXONE:
      return AUTO_STATE_FALLBACK_HID;
    case OUTPUT_XID:
      return AUTO_STATE_OG_XBOX;
    case OUTPUT_SWITCH:
    case OUTPUT_SWITCHPRO:
      return AUTO_STATE_SWITCH;
    default:
      return AUTO_STATE_FALLBACK_HID;
  }
}

AutoDetectState auto_detect_load_persisted_state(outputMode_t mode) {
  if (watchdog_hw->scratch[AUTO_DETECT_SCRATCH_MAGIC] != AUTO_DETECT_MAGIC) {
    return AUTO_STATE_IDLE;
  }

  const uint8_t raw_state =
    (uint8_t)((watchdog_hw->scratch[AUTO_DETECT_SCRATCH_STATE] >>
               kAutoDetectScratchStateShift) & 0xFFu);
  if (isPersistedAutoDetectStateValid(raw_state)) {
    return (AutoDetectState)raw_state;
  }

  return auto_detect_state_for_output_mode(mode);
}

bool auto_detect_preserve_resolved_runtime_mode_for_reboot() {
  if (!auto_detect_has_resolved_runtime_mode()) {
    return false;
  }
  if (autoDetectState == AUTO_STATE_WINDOWS &&
      canonicalizeOutputMode(outputMode) !=
        canonicalizeOutputMode(get_configured_windows_auto_output_mode())) {
    return false;
  }

  autoDetectProbeStage = AUTO_PROBE_GENERIC;
  auto_detect_persist_debug_snapshot(output_autodetect_last_flags, output_autodetect_aux_flags);
  auto_detect_persist_runtime_mode(outputMode, autoDetectState);
  return true;
}

bool auto_detect_preserve_known_runtime_mode_for_input_reboot() {
  if (auto_detect_preserve_resolved_runtime_mode_for_reboot()) {
    return true;
  }
  if (!is_auto_output_mode_selected() ||
      autoDetectState != AUTO_STATE_IDLE ||
      canonicalizeOutputMode(outputMode) != OUTPUT_MISTER) {
    return false;
  }

  autoDetectProbeStage = AUTO_PROBE_GENERIC;
  auto_detect_persist_debug_snapshot(output_autodetect_last_flags, output_autodetect_aux_flags);
  auto_detect_persist_runtime_mode(OUTPUT_MISTER, AUTO_STATE_FALLBACK_HID);
  return true;
}

__attribute__((weak)) bool auto_detect_preserve_current_runtime_mode_for_input_reboot() {
  return auto_detect_preserve_resolved_runtime_mode_for_reboot();
}

outputMode_t get_effective_output_mode() {
  if (configuredOutputMode == OUTPUT_AUTO && autoDetectState == AUTO_STATE_IDLE) {
    return canonicalizeOutputMode(auto_detect_probe_output_mode(autoDetectProbeStage));
  }
  return canonicalizeOutputMode(outputMode);
}
