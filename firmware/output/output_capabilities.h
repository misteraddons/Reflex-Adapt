#pragma once

#include "runtime/input_runtime_output_bridge.h"
#include "switch/out_SwitchCommon.h"
#include "output_runtime_state.h"
#include "../core/controller_state.h"
#include "../core/n64_output_helpers.h"

inline bool output_allows_runtime_input_mode_change() {
  return (outputMode == OUTPUT_HID || outputMode == OUTPUT_MISTER) &&
         configuredOutputMode != OUTPUT_AUTO;
}

inline bool output_supports_analog_triggers(outputMode_t mode) {
  switch (mode) {
    case OUTPUT_HID:
    case OUTPUT_MISTER:
    case OUTPUT_MISTER_NEGCON:
    case OUTPUT_XID:
    case OUTPUT_XINPUT:
    case OUTPUT_XINPUTW:
    case OUTPUT_XINPUT2P:
    case OUTPUT_XBOXONE:
    case OUTPUT_PS3:
    case OUTPUT_PS4:
    case OUTPUT_PS5:
    case OUTPUT_GCWIIU:
      return true;
    default:
      return false;
  }
}

inline bool output_has_secondary_player_slot(outputMode_t mode) {
  return get_max_output_for_mode(mode) > 1;
}

inline bool output_runtime_supports_analog_triggers() {
  return output_supports_analog_triggers(outputMode);
}

inline bool output_runtime_has_secondary_player_slot() {
  return output_has_secondary_player_slot(outputMode);
}

inline bool output_is_generic_mister_hid_mode(outputMode_t mode) {
  const outputMode_t canonicalMode = canonicalizeOutputMode(mode);
  return canonicalMode == OUTPUT_HID || canonicalMode == OUTPUT_MISTER;
}

inline bool output_is_console_clean_usb_mode(outputMode_t mode) {
  switch (canonicalizeOutputMode(mode)) {
    case OUTPUT_XINPUT:
    case OUTPUT_XINPUT2P:
    case OUTPUT_XBOXONE:
    case OUTPUT_XID:
    case OUTPUT_PS3:
    case OUTPUT_PS4:
    case OUTPUT_PS5:
    case OUTPUT_SWITCH:
    case OUTPUT_SWITCHPRO:
      return true;
    default:
      return false;
  }
}

inline bool output_is_specialized_mister_psx_mode(outputMode_t mode) {
  switch (canonicalizeOutputMode(mode)) {
    case OUTPUT_MISTER_JOGCON:
    case OUTPUT_MISTER_NEGCON:
    case OUTPUT_MISTER_GUNCON:
      return true;
    default:
      return false;
  }
}

inline bool output_allows_management_usb_endpoints(outputMode_t effectiveMode) {
#if defined(DISABLE_MANAGEMENT_USB_ENDPOINTS)
  (void)effectiveMode;
  return false;
#else
  const bool genericDInputMode =
    output_is_generic_mister_hid_mode(effectiveMode);
  const bool autoProbeActive =
    configuredOutputMode == OUTPUT_AUTO &&
    autoDetectState == AUTO_STATE_IDLE;
  const bool managementDInputMode =
    genericDInputMode && !autoProbeActive;

  return managementDInputMode &&
         !output_is_console_clean_usb_mode(effectiveMode) &&
         output_is_generic_mister_hid_mode(effectiveMode);
#endif
}

inline bool output_allows_management_cdc(outputMode_t effectiveMode) {
#if defined(DISABLE_MANAGEMENT_USB_ENDPOINTS)
  (void)effectiveMode;
  return false;
#else
  return output_allows_management_usb_endpoints(effectiveMode) ||
         output_is_specialized_mister_psx_mode(effectiveMode);
#endif
}

inline bool output_allows_management_msc(outputMode_t effectiveMode) {
#if defined(DISABLE_MANAGEMENT_USB_ENDPOINTS)
  (void)effectiveMode;
  return false;
#else
  // Specialized MiSTer PSX modes keep their dedicated HID reports while
  // exposing the same utility drive and manager bootstrap as DInput.
  return output_allows_management_usb_endpoints(effectiveMode) ||
         output_is_specialized_mister_psx_mode(effectiveMode);
#endif
}

inline outputMode_t output_effective_mode() {
  return get_effective_output_mode();
}

inline bool output_effective_mode_is(outputMode_t mode) {
  return output_effective_mode() == canonicalizeOutputMode(mode);
}

inline bool output_effective_mode_is_any(outputMode_t firstMode, outputMode_t secondMode) {
  const outputMode_t effectiveMode = output_effective_mode();
  return effectiveMode == canonicalizeOutputMode(firstMode) ||
         effectiveMode == canonicalizeOutputMode(secondMode);
}

inline bool output_effective_mode_is_psx_guncon() {
  return output_effective_mode_is(OUTPUT_MISTER_GUNCON);
}

inline bool output_effective_mode_is_psx_negcon() {
  return output_effective_mode_is(OUTPUT_MISTER_NEGCON);
}

inline bool output_effective_mode_is_psx_jogcon() {
  return output_effective_mode_is(OUTPUT_MISTER_JOGCON);
}

inline bool output_effective_mode_uses_psx_negcon_extended_axes() {
  return output_effective_mode_is_any(OUTPUT_MISTER, OUTPUT_MISTER_NEGCON);
}

inline const char* output_effective_psx_usb_id(bool isIrRemote) {
  if (output_effective_mode_is_psx_guncon()) return "ReflexPSGun";
  if (output_effective_mode_is_psx_negcon()) return "ReflexPSWheel";
  if (output_effective_mode_is_psx_jogcon()) return "MiSTer-A1 RZJog";
  if (isIrRemote) return "RZMPsIR";
  return "RZRPs";
}

inline bool output_is_switchpro_runtime_mode(outputMode_t mode) {
  return canonicalizeOutputMode(mode) == OUTPUT_SWITCHPRO;
}

inline bool output_runtime_is_switchpro_mode() {
  return output_is_switchpro_runtime_mode(outputMode);
}

inline bool output_uses_switch_profile(switchpro_mode_enum profile) {
  return output_is_switchpro_runtime_mode(outputMode) && SwitchCommon::switchpro_mode == profile;
}

inline bool output_uses_switch_n64_profile() {
  return output_uses_switch_profile(SWITCHPRO_N64);
}

inline bool output_maps_n64_c_buttons_to_face_buttons() {
  return output_uses_switch_n64_profile() ||
         get_effective_n64_cstick_mode() == N64CSTICK_AS_BUTTONS;
}

inline bool output_should_spatialize_n64_c_buttons(const controller_state_t& frame) {
  if (!controllerFrameTypeNameIsN64(frame) ||
      get_effective_n64_cstick_mode() != N64CSTICK_AS_BUTTONS) {
    return false;
  }

  // Generic HID/MiSTer has enough button bits for N64 C buttons, so keep the
  // neutral unique backing buttons: C-Up=X, C-Down=L3, C-Left=Y, C-Right=R3.
  const outputMode_t mode = canonicalizeOutputMode(get_effective_output_mode());
  if (mode == OUTPUT_MISTER || mode == OUTPUT_HID) {
    return false;
  }

  return true;
}

inline uint32_t output_apply_n64_c_buttons_to_face_buttons(uint32_t buttons,
                                                           const controller_state_t& frame) {
  if (!output_should_spatialize_n64_c_buttons(frame)) {
    return buttons;
  }

  // Some console-style USB outputs do not have enough distinct N64-style
  // buttons, so C buttons land on the four face-button positions there.
  constexpr uint32_t kButtonA = 1UL << 0;
  constexpr uint32_t kButtonB = 1UL << 1;
  constexpr uint32_t kButtonX = 1UL << 2;
  constexpr uint32_t kButtonY = 1UL << 3;
  constexpr uint32_t kButtonL3 = 1UL << 8;
  constexpr uint32_t kButtonR3 = 1UL << 9;

  buttons &= ~(kButtonL3 | kButtonR3);
  if (frame.X)      buttons |= kButtonY;
  if (frame.L3)     buttons |= kButtonA;
  if (frame.Y)      buttons |= kButtonX;
  if (frame.R3)     buttons |= kButtonB;
  return buttons;
}

inline uint8_t output_n64_c_backing_r2(const controller_state_t& frame) {
  return output_should_spatialize_n64_c_buttons(frame) ? 0 : frame.L3;
}

inline uint8_t output_n64_c_backing_select(const controller_state_t& frame) {
  return output_should_spatialize_n64_c_buttons(frame) ? 0 : frame.R3;
}

inline bool output_maps_vboy_right_dpad_to_face_buttons() {
  return get_effective_n64_cstick_mode() == N64CSTICK_AS_BUTTONS;
}

inline void output_set_switch_profile(switchpro_mode_enum profile) {
  if (!output_is_switchpro_runtime_mode(outputMode)) {
    return;
  }

  SwitchCommon::switchpro_mode = profile;
}

inline void output_set_switch_profile_for_nso_special(bool enabled, switchpro_mode_enum nsoProfile) {
  output_set_switch_profile(enabled ? nsoProfile : SWITCHPRO_PRO);
}

inline void output_set_switch_profile_for_n64_nso_special(bool enabled) {
  output_set_switch_profile_for_nso_special(enabled, SWITCHPRO_N64);
}

inline void output_set_switch_profile_for_nes_nso_special(bool enabled) {
  output_set_switch_profile_for_nso_special(enabled, SWITCHPRO_NES);
}

inline void output_reset_switch_profile_to_default() {
  output_set_switch_profile(SWITCHPRO_PRO);
}

inline void output_apply_switch_profile_for_joybus_input_mode(uint8_t joybusInputMode, bool nsoSpecial) {
  if (!output_runtime_is_switchpro_mode()) {
    return;
  }

  switch (joybusInputMode) {
    case 1: // N64
      output_set_switch_profile_for_n64_nso_special(nsoSpecial);
      break;
    case 2: // GBA can optionally reuse the NES profile
      output_set_switch_profile_for_nes_nso_special(nsoSpecial);
      break;
    default:
      output_reset_switch_profile_to_default();
      break;
  }
}

inline void output_apply_switch_profile_for_snes_input_mode(uint8_t snesInputMode, bool nsoSpecial) {
  if (!output_runtime_is_switchpro_mode()) {
    return;
  }

  switch (snesInputMode) {
    case 1: // NES
      output_set_switch_profile_for_nes_nso_special(nsoSpecial);
      break;
    case 2: // SNES stays on Pro profile so rumble still works
      output_reset_switch_profile_to_default();
      break;
    default:
      break;
  }
}

inline bool output_try_override_runtime_mode(outputMode_t desiredMode) {
  if (!output_allows_runtime_input_mode_change()) {
    return false;
  }

  outputMode = desiredMode;
  return true;
}

inline bool output_try_enable_psx_negcon_mode() {
  return output_try_override_runtime_mode(OUTPUT_MISTER_NEGCON);
}

inline bool output_try_enable_psx_guncon_mode() {
  return output_try_override_runtime_mode(OUTPUT_MISTER_GUNCON);
}

inline bool output_try_enable_psx_jogcon_mode() {
  return output_try_override_runtime_mode(OUTPUT_MISTER_JOGCON);
}

inline uint8_t output_runtime_mode_value() {
  return (uint8_t)outputMode;
}

inline void output_promote_psx_peripheral_mode(bool isJogcon, bool isNegcon, bool isGuncon) {
  if (!output_is_generic_mister_hid_mode(outputMode)) {
    return;
  }

  if (isJogcon) {
    outputMode = OUTPUT_MISTER_JOGCON;
  } else if (isNegcon) {
    outputMode = OUTPUT_MISTER_NEGCON;
  } else if (isGuncon) {
    outputMode = OUTPUT_MISTER_GUNCON;
  }
}
