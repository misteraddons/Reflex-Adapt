#include "turbo.h"

#include <Arduino.h>

#include "controller_frame_state.h"
#include "controller_settings_state.h"

namespace {

bool liveInputHasAnalogTriggers() {
  for (uint8_t i = 0; i < max_devices && i < MAX_USB_OUT; ++i) {
    const controller_state_t& frame = controllerFrameConst(i);
    if (frame.connected && frame.HAS_ANALOG_TRIGGERS) {
      return true;
    }
  }
  return false;
}

bool isAnalogTriggerTurboButton(TurboInputMode mode, TurboButton btn) {
  if (mode == TURBO_MODE_WII) {
    return btn == TURBO_BTN_4 || btn == TURBO_BTN_5;
  }
  return btn == TURBO_BTN_6 || btn == TURBO_BTN_7;
}

bool inputModeHasIndependentDigitalTriggerButtons(TurboInputMode mode) {
  return mode == TURBO_MODE_PSX || mode == TURBO_MODE_GAMECUBE;
}

bool turboButtonVisibleForMode(TurboInputMode mode, TurboButton btn) {
  const TurboButtonConfig& config = getTurboButtonConfig(mode);
  for (uint8_t i = 0; i < config.count; ++i) {
    if (config.indices[i] == btn) {
      return true;
    }
  }
  return false;
}

}  // namespace

void TurboController::refreshEnabledState() {
  any_enabled = false;
  for (uint8_t i = 0; i < TURBO_BTN_COUNT; i++) {
    if (button_rates[i] != TURBO_OFF) {
      any_enabled = true;
      return;
    }
  }
}

uint32_t TurboController::getTurboButtonMask(TurboButton btn) const {
  if (btn >= TURBO_BTN_COUNT) {
    return 0;
  }
  if (!turboButtonVisibleForMode(input_mode, btn)) {
    return 0;
  }
  if (isAnalogTriggerTurboButton(input_mode, btn) &&
      trigger_mode != TRIGGER_MODE_DIGITAL &&
      liveInputHasAnalogTriggers() &&
      !inputModeHasIndependentDigitalTriggerButtons(input_mode)) {
    return 0;
  }
  return turbo_button_masks[btn];
}

void TurboController::resetTracking() {
  for (uint8_t i = 0; i < TURBO_BTN_COUNT; i++) {
    last_toggle_time[i] = 0;
    turbo_state[i] = true;
  }
  for (uint8_t i = 0; i < MAX_USB_OUT; i++) {
    raw_held_buttons[i] = 0;
    latched_buttons[i] = 0;
    last_physical_buttons[i] = 0;
  }
}

TurboController::TurboController()
  : any_enabled(false),
    input_mode(TURBO_MODE_GENERIC) {
  for (uint8_t i = 0; i < TURBO_BTN_COUNT; i++) {
    button_rates[i] = TURBO_OFF;
  }
  resetTracking();
}

void TurboController::setInputMode(TurboInputMode mode) {
  if (input_mode == mode) {
    return;
  }
  input_mode = mode;
  resetTracking();
}

TurboInputMode TurboController::getInputMode() {
  return input_mode;
}

void TurboController::resetRuntimeState() {
  resetTracking();
}

const TurboButtonConfig& TurboController::getButtonConfig() {
  return getTurboButtonConfig(input_mode);
}

void TurboController::setButtonRate(TurboButton btn, TurboRate rate) {
  if (btn < TURBO_BTN_COUNT && button_rates[btn] != rate) {
    button_rates[btn] = rate;
    refreshEnabledState();
    resetTracking();
  }
}

TurboRate TurboController::getButtonRate(TurboButton btn) {
  if (btn < TURBO_BTN_COUNT) {
    return button_rates[btn];
  }
  return TURBO_OFF;
}

bool __not_in_flash_func(TurboController::hasAnyEnabled)() {
  return any_enabled;
}

void TurboController::updateRawButtons(uint8_t device, uint32_t current_buttons) {
  if (device >= MAX_USB_OUT) return;

  for (uint8_t i = 0; i < TURBO_BTN_COUNT; i++) {
    uint32_t mask = getTurboButtonMask((TurboButton)i);
    if (mask == 0) {
      continue;
    }
    if (button_rates[i] == TURBO_OFF) {
      raw_held_buttons[device] &= ~mask;
      latched_buttons[device] &= ~mask;
      continue;
    }

    bool button_in_report = (current_buttons & mask) != 0;
    bool button_was_in_report = (last_physical_buttons[device] & mask) != 0;
    bool button_tracked = (raw_held_buttons[device] & mask) != 0;

    if (isTurboRateLatched(button_rates[i])) {
      if (button_in_report && !button_was_in_report) {
        latched_buttons[device] ^= mask;
      }
      if (latched_buttons[device] & mask) {
        raw_held_buttons[device] |= mask;
      } else {
        raw_held_buttons[device] &= ~mask;
      }
    } else if (button_in_report) {
      raw_held_buttons[device] |= mask;
    } else if (button_tracked) {
      if (turbo_state[i]) {
        raw_held_buttons[device] &= ~mask;
      }
    }
  }

  last_physical_buttons[device] = current_buttons;
}

bool TurboController::isHoldingTurboButton(uint8_t device) {
  if (device >= MAX_USB_OUT) return false;
  for (uint8_t i = 0; i < TURBO_BTN_COUNT; i++) {
    if (button_rates[i] != TURBO_OFF) {
      if (raw_held_buttons[device] & getTurboButtonMask((TurboButton)i)) {
        return true;
      }
    }
  }
  return false;
}

uint32_t TurboController::getRawHeldButtons(uint8_t device) {
  if (device < MAX_USB_OUT) {
    return raw_held_buttons[device];
  }
  return 0;
}

uint16_t TurboController::getEnabledButtonsMask() {
  uint16_t mask = 0;
  for (uint8_t i = 0; i < TURBO_BTN_COUNT; i++) {
    if (button_rates[i] != TURBO_OFF && getTurboButtonMask((TurboButton)i) != 0) {
      mask |= (uint16_t)(1u << i);
    }
  }
  return mask;
}

void TurboController::setAllRates(const uint8_t* rates) {
  for (uint8_t i = 0; i < TURBO_BTN_COUNT; i++) {
    if (rates[i] < TURBO_RATE_LAST) {
      button_rates[i] = (TurboRate)rates[i];
    } else {
      button_rates[i] = TURBO_OFF;
    }
  }
  refreshEnabledState();
  resetTracking();
}

void TurboController::getAllRates(uint8_t* rates) {
  for (uint8_t i = 0; i < TURBO_BTN_COUNT; i++) {
    rates[i] = button_rates[i];
  }
}

void TurboController::update() {
  uint32_t now = millis();

  for (uint8_t i = 0; i < TURBO_BTN_COUNT; i++) {
    if (button_rates[i] == TURBO_OFF) {
      turbo_state[i] = true;
      continue;
    }

    uint16_t interval = getTurboIntervalMs(button_rates[i]);
    if (now - last_toggle_time[i] >= interval) {
      turbo_state[i] = !turbo_state[i];
      last_toggle_time[i] = now;
    }
  }
}

uint32_t TurboController::applyTurbo(uint8_t device, uint32_t current_buttons) {
  if (device >= MAX_USB_OUT) return current_buttons;

  uint32_t output_buttons = current_buttons;

  for (uint8_t i = 0; i < TURBO_BTN_COUNT; i++) {
    if (button_rates[i] == TURBO_OFF) continue;

    uint32_t mask = getTurboButtonMask((TurboButton)i);
    if (mask == 0) continue;

    if (raw_held_buttons[device] & mask) {
      if (turbo_state[i]) {
        output_buttons |= mask;
      } else {
        output_buttons &= ~mask;
      }
    } else if (isTurboRateLatched(button_rates[i])) {
      output_buttons &= ~mask;
    }
  }
  return output_buttons;
}

bool TurboController::getButtonTurboState(TurboButton btn) {
  if (btn < TURBO_BTN_COUNT) {
    return turbo_state[btn];
  }
  return false;
}

TurboController turbo;
