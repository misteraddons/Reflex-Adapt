#pragma once

#include <Arduino.h>
#include "../firmware_platform_config.h"
#include "../core/controller_settings_state.h"
#include "../core/device_runtime_state.h"
#include "output_mode.h"

#if defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
  #include "hardware/watchdog.h"
#endif

extern outputMode_t configuredOutputMode;
extern outputMode_t outputMode;
extern socdMode_t socdMode;
extern bool use_device_hid_class;
extern uint8_t hid_begin_success_count;
extern uint8_t hid_first_fail_index;

// N64 input on Switch Pro output always uses the N64 NSO mapping/profile.
bool is_n64_switch_nso_quirk_active();

// Manual NSO mode is only meaningful for classic Nintendo controller inputs
// that can intentionally present as Switch Online-style profiles.
bool input_mode_supports_nso_special(DeviceEnum mode);

// Effective NSO special mode (manual toggle plus automatic N64-on-Switch behavior).
bool is_nso_special_active();

outputMode_t canonicalizeOutputMode(outputMode_t mode);
bool is_xinput2p_output_enabled();

outputMode_t sanitizeRuntimeOutputMode(outputMode_t mode);
outputMode_t get_configured_windows_auto_output_mode();

// Z-button output policy:
// - N64 mode 0 combines Z into L1; mode 1 keeps Z on L2.
// - GameCube mode 0 maps Z to R1/RB; mode 1 keeps Z on Select/Back.
// Check if output mode supports D-Pad as buttons (only HID and MiSTer have flexible button counts)
bool output_supports_dpad_buttons(outputMode_t mode);

// Native PS5/P5General is timing-sensitive. Keep nonessential firmware
// services quiet while this output is active so USB auth/report timing stays
// as close to the donor path as possible.
bool is_ps5_timing_quiet_mode_active();

// Clear all OUTPUT_AUTO scratch state so manual output modes cannot be
// overridden by stale auto-detect state.
void auto_detect_clear_scratch_state();

// Clear only chained AUTO input scratch while leaving any preserved AUTO output
// result intact for an input-mode reboot.
void auto_detect_clear_input_scratch_state();

enum AutoDetectProbeStage : uint8_t {
  AUTO_PROBE_GENERIC = 0,
  AUTO_PROBE_PS3,
  AUTO_PROBE_PS3_AFTER_PS4,
  AUTO_PROBE_PS3_AFTER_WINDOWS,
  AUTO_PROBE_XID,
  AUTO_PROBE_XINPUT,
};

extern AutoDetectProbeStage autoDetectProbeStage;
extern uint8_t output_autodetect_last_flags;
extern uint8_t output_autodetect_aux_flags;
extern uint8_t output_autodetect_usb_hw_flags;
extern uint8_t output_autodetect_usb_irq_observed;
extern uint8_t output_autodetect_usb_irq_flags;
extern uint16_t output_autodetect_probe_counters;
extern uint16_t output_autodetect_last_ms;
extern uint8_t output_autodetect_transition_reason;

enum AutoDetectTransitionReason : uint8_t {
  AUTO_TRANSITION_NONE = 0,
  AUTO_TRANSITION_GENERIC_NO_MOUNT_TO_FALLBACK = 1,
  AUTO_TRANSITION_GENERIC_NO_CONNECT_TO_FALLBACK = 2,
  // Deprecated aliases kept so old diagnostics/scripts remain readable while
  // AUTO returns to RetroZord's single-pass generic HID fallback model.
  AUTO_TRANSITION_GENERIC_NO_MOUNT_TO_PS3 = AUTO_TRANSITION_GENERIC_NO_MOUNT_TO_FALLBACK,
  AUTO_TRANSITION_GENERIC_NO_CONNECT_TO_XID = AUTO_TRANSITION_GENERIC_NO_CONNECT_TO_FALLBACK,
  AUTO_TRANSITION_DIAG_PULLUP_PULSE = 3,
  AUTO_TRANSITION_PS4_EXTENDED_TO_PS5 = 4,
  AUTO_TRANSITION_PS5_PROBE_TO_PS4 = 5,
  AUTO_TRANSITION_GENERIC_PS3_ASSIST = 6,
};

void auto_detect_persist_probe_stage(AutoDetectProbeStage stage);

void auto_detect_persist_debug_snapshot(uint8_t flags, uint8_t aux_flags);

void auto_detect_restore_debug_snapshot();

AutoDetectProbeStage auto_detect_load_probe_stage();

uint8_t auto_detect_load_transition_reason();

outputMode_t auto_detect_probe_output_mode(AutoDetectProbeStage stage);

enum AutoDetectState : uint8_t {
  AUTO_STATE_IDLE = 0,
  AUTO_STATE_PS3,
  AUTO_STATE_WINDOWS,
  AUTO_STATE_PS4,
  AUTO_STATE_OG_XBOX,
  AUTO_STATE_XBOX_360,
  AUTO_STATE_SWITCH,
  AUTO_STATE_FALLBACK_HID,
  AUTO_STATE_PS5,
  AUTO_STATE_PLAYSTATION,
};

extern AutoDetectState autoDetectState;

#define OUTPUT_AUTODETECT_TIMING_VALID 0x01

bool is_auto_output_mode_selected();

bool auto_detect_has_resolved_runtime_mode();

void auto_detect_persist_runtime_mode(outputMode_t mode, AutoDetectState state);

outputMode_t auto_detect_load_persisted_runtime_mode();

AutoDetectState auto_detect_load_persisted_state(outputMode_t mode);

AutoDetectState auto_detect_state_for_output_mode(outputMode_t mode);

bool auto_detect_preserve_resolved_runtime_mode_for_reboot();
bool auto_detect_preserve_current_runtime_mode_for_input_reboot();
bool auto_detect_preserve_known_runtime_mode_for_input_reboot();

outputMode_t get_effective_output_mode();


#ifdef __cplusplus
extern "C" {
#endif
uint8_t get_max_output_for_mode(outputMode_t mode);
const char* get_mode_name(outputMode_t mode);
#ifdef __cplusplus
}
#endif
