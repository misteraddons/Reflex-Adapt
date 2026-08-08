#include "RZInputModule.h"

ButtonDebouncer::ButtonDebouncer(uint32_t _buttonsMask, uint8_t _debounceTimeMs)
    : buttonsMask{_buttonsMask},
      debounceTimeMs{_debounceTimeMs},
      debouncedState{0},
      lastDebouncedState{0} {
  gpio_init_mask(buttonsMask);
  gpio_set_dir_in_masked(buttonsMask);
  for (uint8_t i = 0; i < 32; ++i) {
    if (buttonsMask & (1UL << i)) {
      gpio_pull_up(i);
    }
    debouncedAtMs[i] = 0;
  }
}

bool ButtonDebouncer::update() {
  bool stateChanged = false;
  lastDebouncedState = debouncedState;
  uint32_t currentState = ~gpio_get_all() & buttonsMask;
  if (debounceTimeMs == 0) {
    debouncedState = currentState;
    return debouncedState != lastDebouncedState;
  }
  uint32_t millisNow = millis();
  for (uint8_t pin = 0; pin < 32; ++pin) {
    uint32_t pin_mask = buttonToMask(pin);
    if ((buttonsMask & pin_mask) &&
        ((debouncedState & pin_mask) != (currentState & pin_mask)) &&
        ((millisNow - debouncedAtMs[pin]) > debounceTimeMs)) {
      debouncedState ^= pin_mask;
      debouncedAtMs[pin] = millisNow;
      stateChanged = true;
    }
  }
  return stateChanged;
}

uint32_t ButtonDebouncer::getDebouced() const {
  return debouncedState;
}

uint32_t ButtonDebouncer::getLast() const {
  return lastDebouncedState;
}

bool ButtonDebouncer::digitalPressed(const int8_t button) const {
  uint32_t pin_mask = buttonToMask(button);
  return (debouncedState & pin_mask) != 0;
}

bool ButtonDebouncer::digitalChanged(const uint8_t button) const {
  uint32_t pin_mask = buttonToMask(button);
  return (lastDebouncedState & pin_mask) != (debouncedState & pin_mask);
}

bool ButtonDebouncer::digitalJustPressed(const uint8_t button) const {
  return digitalChanged(button) & digitalPressed(button);
}

bool ButtonDebouncer::digitalJustReleased(const uint8_t button) const {
  return digitalChanged(button) & !digitalPressed(button);
}

void RZInputModule::resetState(const uint8_t index) {
  if (index >= inputPortCount()) {
    return;
  }
  clearInputFrameButtons(index);
}

uint16_t RZInputModule::convertAnalog(const uint8_t value) {
  return (value << 8) + value;
}

bool RZInputModule::canUseAnalogTrigger() {
  return output_runtime_supports_analog_triggers();
}

void RZInputModule::beginPollCycle() {
  beginInputPollCycle();
}

bool RZInputModule::endPollCycle() {
  return inputPollUpdated();
}

void RZInputModule::markPollUpdated() {
  markInputPollUpdated();
}

void RZInputModule::setUpdated(const uint8_t index) {
  markInputFrameUpdated(index);
}

void RZInputModule::setPortLed(const uint8_t index, const uint8_t state) {
  if (index != 1) {
    return;
  }

  if (output_runtime_has_secondary_player_slot()) {
    setLed(state);
  }
}

void RZInputModule::setPhysicalPortMask(uint8_t mask) {
  uint8_t validMask = 0xFF;
  if (MAX_USB_OUT < 8) {
    validMask = (uint8_t)((1u << MAX_USB_OUT) - 1u);
  }
  const uint8_t sanitized = mask & validMask;
  physical_port_mask = sanitized == 0 ? validMask : sanitized;
}

bool RZInputModule::physical_port_enabled(uint8_t port) const {
  return port < 8 && (physical_port_mask & (uint8_t)(1u << port));
}

bool RZInputModule::should_poll_port(uint8_t input_ports, uint8_t port) {
  if (!physical_port_enabled(port)) {
    return false;
  }

  if (empty_port_behaviour == EMPTY_PORT_ALWAYS_SCAN) {
    return true;
  }

  if (!is_port_connected(port)) {
    if (empty_port_behaviour == EMPTY_PORT_USE_INTERVAL &&
        (millis() - input_empty_port_last_tested_at < polling_empty_interval_ms)) {
      return false;
    }

    uint8_t oldest_port = port;
    for (uint8_t i = 0; i < input_ports; ++i) {
      if (!physical_port_enabled(i) || is_port_connected(i) || i == port) {
        continue;
      }
      if (input_empty_port_tested_port_timestamp[i] <
          input_empty_port_tested_port_timestamp[oldest_port]) {
        oldest_port = i;
      }
    }

    if (oldest_port != port) {
      return false;
    }

    input_empty_port_last_tested_at = millis();
    input_empty_port_tested_port_timestamp[port] = input_empty_port_last_tested_at;
  }

  return true;
}
