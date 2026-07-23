#include "../product_config.h"
#include "controller_runtime_internal.h"

#include <Arduino.h>

#include "controller_frame_state.h"
#include "controller_output_cache_state.h"
#include "controller_settings_state.h"
#include "controller_delivery_state.h"
#include "button_remap.h"
#include "dpad_mode.h"
#include "firmware_hotkeys.h"
#include "hotkey_combo.h"
#include "../input/shared/input_button_bits.h"
#include "../menu/menu_runtime_state.h"
#include "../menu/menu_ui_state.h"

namespace controller_runtime_internal {

namespace {

struct HeldVirtualHotkeyState {
  bool active = false;
};

uint32_t virtualHotkeyOutputButtons[MAX_USB_OUT] = {};
uint32_t virtualHotkeySuppressedButtons[MAX_USB_OUT] = {};

void __not_in_flash_func(clearHotkeyComboFromFrame)(controller_state_t& frame, uint32_t combo) {
  // PAD_* shares storage with digital_buttons, so these also clear INPUT_PAD_* bits.
  frame.digital_buttons &= ~combo;
}

uint32_t __not_in_flash_func(physicalHotkeyButtonsForController)(uint8_t index) {
  if (index >= max_devices || index >= MAX_USB_OUT) {
    return 0;
  }
  const controller_state_t& frame = controllerFrameConst(index);
  if (!frame.connected) {
    return 0;
  }

  return pre_transform_hotkey_buttons[index] & HOTKEY_ALLOWED_BUTTONS;
}

bool __not_in_flash_func(applyHeldVirtualButtonHotkey)(uint8_t index,
                                                       controller_state_t& frame,
                                                       HeldVirtualHotkeyState& state,
                                                       uint32_t combo,
                                                       uint32_t outputButton,
                                                       uint32_t physicalButtons,
                                                       uint32_t& outputButtons) {
  if (combo == 0 || outputButton == 0) {
    state = HeldVirtualHotkeyState{};
    return false;
  }

  const bool held = hotkeyComboMatches(physicalButtons, combo);
  if (!held) {
    state = HeldVirtualHotkeyState{};
    return false;
  }

  if (!state.active) {
    state.active = true;
  }

  // Suppress the physical combo before virtual output so games do not see Dn+Start.
  clearHotkeyComboFromFrame(frame, combo);
  virtualHotkeySuppressedButtons[index] = combo;
  outputButtons |= outputButton;

  return true;
}

}  // namespace

void __not_in_flash_func(handleHeldBootloaderHotkey)() {
  if (hotkey_bootsel == (uint32_t)-1) {
    return;
  }

  static uint32_t hotkey_bootsel_pressedAt = 0;
  static bool hotkey_bootsel_btnStateOld = false;
  bool hotkey_bootsel_btnStateNow =
      hotkeyComboMatches(physicalHotkeyButtonsForController(0), hotkey_bootsel);
  if (hotkey_bootsel_btnStateNow != hotkey_bootsel_btnStateOld) {
    hotkey_bootsel_btnStateOld = hotkey_bootsel_btnStateNow;
    hotkey_bootsel_pressedAt = hotkey_bootsel_btnStateNow ? millis() : 0;
  }
  if (hotkey_bootsel_btnStateNow && millis() - hotkey_bootsel_pressedAt > 5000) {
    rp2040.rebootToBootloader();
  }
}

void __not_in_flash_func(handleHeldDpadModeHotkey)() {
  if (hotkey_dpad_as_buttons == (uint32_t)-1) {
    return;
  }

  static uint32_t hotkey_dpad_as_buttons_pressedAt = 0;
  static bool hotkey_dpad_as_buttons_btnStateOld = false;
  bool hotkey_dpad_as_buttons_btnStateNow =
      hotkeyComboMatches(physicalHotkeyButtonsForController(0), hotkey_dpad_as_buttons);
  if (hotkey_dpad_as_buttons_btnStateNow != hotkey_dpad_as_buttons_btnStateOld) {
    hotkey_dpad_as_buttons_btnStateOld = hotkey_dpad_as_buttons_btnStateNow;
    hotkey_dpad_as_buttons_pressedAt = hotkey_dpad_as_buttons_btnStateNow ? millis() : 0;
  }
  if (hotkey_dpad_as_buttons_btnStateNow && millis() - hotkey_dpad_as_buttons_pressedAt > 5000) {
    dpad_mode = (dpad_mode + 1) % 4;
    hotkey_dpad_as_buttons_pressedAt = millis();
  }
}

void __not_in_flash_func(updateHeldControllerHotkeysBeforeTransforms)() {
  static HeldVirtualHotkeyState homeHotkeyState[MAX_USB_OUT];
  static HeldVirtualHotkeyState captureHotkeyState[MAX_USB_OUT];
  static uint32_t lastVirtualHotkeyOutputButtons[MAX_USB_OUT] = {};
  static uint32_t previousPhysicalButtons[MAX_USB_OUT] = {};
  const uint32_t homeCombo = menu_home_hotkey != 0 ? menu_home_hotkey_combo : 0;
  const uint32_t captureCombo = menu_capture_hotkey != 0 ? menu_capture_hotkey_combo : 0;

  if (menuOwnsControllerInput()) {
    for (uint8_t i = 0; i < MAX_USB_OUT; ++i) {
      homeHotkeyState[i] = HeldVirtualHotkeyState{};
      captureHotkeyState[i] = HeldVirtualHotkeyState{};
      virtualHotkeyOutputButtons[i] = 0;
      virtualHotkeySuppressedButtons[i] = 0;
      lastVirtualHotkeyOutputButtons[i] = 0;
      previousPhysicalButtons[i] = 0;
    }
    return;
  }

  for (uint8_t i = 0; i < max_devices && i < MAX_USB_OUT; ++i) {
    controller_state_t& frame = controllerFrame(i);
    if (!frame.connected) {
      if (homeHotkeyState[i].active || captureHotkeyState[i].active ||
          virtualHotkeyOutputButtons[i] != 0 || virtualHotkeySuppressedButtons[i] != 0) {
        virtualHotkeyOutputButtons[i] = 0;
        virtualHotkeySuppressedButtons[i] = 0;
      }
      homeHotkeyState[i] = HeldVirtualHotkeyState{};
      captureHotkeyState[i] = HeldVirtualHotkeyState{};
      previousPhysicalButtons[i] = 0;
      continue;
    }

    const uint32_t physicalButtons = physicalHotkeyButtonsForController(i);
    // Some serial controller protocols can expose the two edges of a chord in
    // adjacent polls. Bridge only that one-poll transition so Down->Start and
    // Start->Down are equivalent without turning sequential taps into a latch.
    const uint32_t transitionButtons = physicalButtons | previousPhysicalButtons[i];
    previousPhysicalButtons[i] = physicalButtons;
    const bool canTriggerHome = hotkeyComboMatches(transitionButtons, homeCombo);
    const bool canTriggerCapture = hotkeyComboMatches(transitionButtons, captureCombo);
    const bool hasActiveVirtualHotkey =
      homeHotkeyState[i].active ||
      captureHotkeyState[i].active ||
      virtualHotkeyOutputButtons[i] != 0 ||
      virtualHotkeySuppressedButtons[i] != 0;
    if (!hasActiveVirtualHotkey && !canTriggerHome && !canTriggerCapture) {
      continue;
    }
    virtualHotkeyOutputButtons[i] = 0;
    virtualHotkeySuppressedButtons[i] = 0;

    const uint32_t homeButtons = homeHotkeyState[i].active ? physicalButtons : transitionButtons;
    const uint32_t captureButtons = captureHotkeyState[i].active ? physicalButtons : transitionButtons;
    if ((homeHotkeyState[i].active || canTriggerHome) &&
        applyHeldVirtualButtonHotkey(i, frame, homeHotkeyState[i], homeCombo,
                                     INPUT_HOME, homeButtons, virtualHotkeyOutputButtons[i])) {
      continue;
    }

    if ((captureHotkeyState[i].active || canTriggerCapture) &&
        applyHeldVirtualButtonHotkey(i, frame, captureHotkeyState[i], captureCombo,
                                     INPUT_CAPTURE, captureButtons, virtualHotkeyOutputButtons[i])) {
      continue;
    }
  }

  for (uint8_t i = 0; i < max_devices && i < MAX_USB_OUT; ++i) {
    if (virtualHotkeyOutputButtons[i] != lastVirtualHotkeyOutputButtons[i]) {
      requestControllerFrameDelivery(i);
      lastVirtualHotkeyOutputButtons[i] = virtualHotkeyOutputButtons[i];
    }
  }
}

bool __not_in_flash_func(virtualControllerHotkeysRequireService)() {
  if ((menu_home_hotkey != 0 && menu_home_hotkey_combo != 0) ||
      (menu_capture_hotkey != 0 && menu_capture_hotkey_combo != 0)) {
    return true;
  }

  for (uint8_t i = 0; i < MAX_USB_OUT; ++i) {
    if (virtualHotkeyOutputButtons[i] != 0 ||
        virtualHotkeySuppressedButtons[i] != 0) {
      return true;
    }
  }
  return false;
}

void __not_in_flash_func(applyVirtualControllerHotkeyOutputs)() {
  for (uint8_t i = 0; i < max_devices && i < MAX_USB_OUT; ++i) {
    controller_state_t& frame = controllerFrame(i);
    if (frame.connected) {
      frame.digital_buttons &= ~virtualHotkeySuppressedButtons[i];
      frame.digital_buttons |= virtualHotkeyOutputButtons[i];
    }
  }
}

uint32_t __not_in_flash_func(suppressedControllerHotkeySourceButtons)(uint8_t index) {
  return (index < MAX_USB_OUT) ? virtualHotkeySuppressedButtons[index] : 0;
}

}  // namespace controller_runtime_internal
