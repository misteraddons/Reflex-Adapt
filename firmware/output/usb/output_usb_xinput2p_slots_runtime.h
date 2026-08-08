#pragma once

#include <Arduino.h>

#include "../../core/controller_frame_state.h"
#include "../../core/firmware_support.h"
#include "../output_runtime_state.h"
#include "../xinput/out_xinput_multi.h"
#include "output_usb_player_count.h"

#ifdef ADAPT_OUTPUT_USB_DEVICE
#include <Adafruit_TinyUSB.h>
#endif

// Production XInput2P intentionally enumerates both wired XUSB children. Some
// SDL/rawinput tester apps will not route rumble to a neutral child until that
// controller has produced input, so users may need one button press per pad.
//
// ENABLE_XINPUT2P_DYNAMIC_PRESENT_SLOTS preserves the tested present-only
// alternative: enumerate only connected classic pads and soft-reboot USB when
// P2 appears/disappears. That fixes tester-app rumble before any button press
// for a single connected pad, but host-visible hotplug causes a Windows USB
// disconnect/reconnect, so it is kept opt-in rather than production default.
constexpr uint32_t kXinput2pSlotGrowRebootDebounceMs = 750;
constexpr uint32_t kXinput2pSlotShrinkRebootDebounceMs = 3000;

inline bool xinput2p_uses_dynamic_wired_slots() {
#if defined(ENABLE_XINPUT2P_DYNAMIC_PRESENT_SLOTS) && \
    defined(ADAPT_OUTPUT_USB_DEVICE) && \
    defined(ENABLE_EXPERIMENTAL_XINPUT2P_OUTPUT) && \
    !defined(FORCE_XINPUT2P_SINGLE_DRIVER) && \
    !defined(ENABLE_XINPUT2P_WIRELESS_TRANSPORT)
  return get_effective_output_mode() == OUTPUT_XINPUT2P &&
         configuredOutputMode != OUTPUT_XINPUT &&
         autoDetectState != AUTO_STATE_XBOX_360 &&
         autoDetectProbeStage != AUTO_PROBE_XINPUT;
#else
  return false;
#endif
}

inline uint8_t xinput2p_fixed_usb_controller_count() {
  uint8_t count = output_usb_player_count();
  if (count == 0) {
    count = 1;
  }
  if (count > XINPUT_MULTI_CONTROLLERS) {
    count = XINPUT_MULTI_CONTROLLERS;
  }
  return count;
}

inline uint8_t xinput2p_present_usb_controller_count() {
  const uint8_t outputPlayers = output_usb_player_count();
  if (outputPlayers <= 1) {
    return 1;
  }

  const uint8_t limit = min((uint8_t)XINPUT_MULTI_CONTROLLERS, outputPlayers);
  uint8_t highestConnectedSlot = 0;
  for (uint8_t i = 0; i < limit; ++i) {
    if (controllerFrameConst(i).connected) {
      highestConnectedSlot = i + 1;
    }
  }

  if (highestConnectedSlot == 0) {
    return 1;
  }
  return highestConnectedSlot;
}

inline uint8_t xinput2p_desired_usb_controller_count() {
#ifdef FORCE_XINPUT2P_ONE_CONTROLLER
  return 1;
#endif
#ifdef ENABLE_XINPUT2P_DYNAMIC_PRESENT_SLOTS
  if (xinput2p_uses_dynamic_wired_slots()) {
    return xinput2p_present_usb_controller_count();
  }
#endif
  return xinput2p_fixed_usb_controller_count();
}

inline void xinput2p_reboot_for_slot_count_change() {
#ifdef ADAPT_OUTPUT_USB_DEVICE
  tud_disconnect();
  delay(50);
#endif
  auto_detect_preserve_resolved_runtime_mode_for_reboot();
  reboot();
}

inline void service_xinput2p_dynamic_slot_reenumeration() {
  static uint8_t pendingDesiredCount = 0;
  static uint32_t pendingSinceMs = 0;

  if (!xinput2p_uses_dynamic_wired_slots()) {
    pendingDesiredCount = 0;
    pendingSinceMs = 0;
    return;
  }

  const uint8_t currentCount = xinput_multi_controller_count();
  if (currentCount == 0) {
    pendingDesiredCount = 0;
    pendingSinceMs = 0;
    return;
  }

  const uint8_t desiredCount = xinput2p_desired_usb_controller_count();
  if (desiredCount == currentCount) {
    pendingDesiredCount = 0;
    pendingSinceMs = 0;
    return;
  }

  const uint32_t now = millis();
  if (pendingDesiredCount != desiredCount) {
    pendingDesiredCount = desiredCount;
    pendingSinceMs = now;
    return;
  }

  const uint32_t debounceMs =
    (desiredCount > currentCount)
      ? kXinput2pSlotGrowRebootDebounceMs
      : kXinput2pSlotShrinkRebootDebounceMs;
  if (now - pendingSinceMs >= debounceMs) {
    xinput2p_reboot_for_slot_count_change();
  }
}
