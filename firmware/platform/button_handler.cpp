#include "button_handler.h"
#include "button_handler_internal.h"

bool ButtonHandler::readButton() {
  bool raw = digitalRead(pin);
  return active_low ? !raw : raw;
}

ButtonHandler::ButtonHandler(uint8_t pin_num, bool is_active_low)
  : pin(pin_num),
    active_low(is_active_low),
    last_raw(false),
    current_state(false),
    last_change_time(0),
    press_start_time(0),
    tap_count(0),
    first_tap_time(0),
    long_press_fired(false),
    waiting_for_double_tap(false),
    suppress_until_release(false),
    pendingEvent(BTN_EVENT_NONE) {}

void ButtonHandler::begin() {
  pinMode(pin, INPUT_PULLUP);
  last_raw = readButton();
  current_state = last_raw;
  suppress_until_release = false;
}

void ButtonHandler::suppressUntilRelease() {
  uint32_t now = millis();
  last_raw = readButton();
  current_state = last_raw;
  last_change_time = now;
  press_start_time = 0;
  tap_count = 0;
  first_tap_time = 0;
  long_press_fired = false;
  waiting_for_double_tap = false;
  suppress_until_release = true;
  pendingEvent = BTN_EVENT_NONE;
}

bool ButtonHandler::isPressed() {
  return current_state;
}

void ButtonHandler::poll() {
  if (button_handler_internal::updateSuppressedState(
        readButton(), millis(),
        last_raw, current_state,
        last_change_time, press_start_time,
        tap_count, first_tap_time,
        long_press_fired, waiting_for_double_tap,
        suppress_until_release, pendingEvent)) {
    return;
  }

  button_handler_internal::pollState(
    readButton(), millis(),
    last_raw, current_state,
    last_change_time, press_start_time,
    tap_count, first_tap_time,
    long_press_fired, waiting_for_double_tap,
    pendingEvent
  );
}

ButtonEvent ButtonHandler::update() {
  if (button_handler_internal::updateSuppressedState(
        readButton(), millis(),
        last_raw, current_state,
        last_change_time, press_start_time,
        tap_count, first_tap_time,
        long_press_fired, waiting_for_double_tap,
        suppress_until_release, pendingEvent)) {
    return BTN_EVENT_NONE;
  }

  return button_handler_internal::updateState(
    readButton(), millis(),
    last_raw, current_state,
    last_change_time, press_start_time,
    tap_count, first_tap_time,
    long_press_fired, waiting_for_double_tap,
    pendingEvent, true
  );
}

bool BootselButtonHandler::readButton() {
  return BOOTSEL;  // BOOTSEL macro reads the boot button state
}

BootselButtonHandler::BootselButtonHandler()
  : last_raw(false),
    current_state(false),
    last_change_time(0),
    press_start_time(0),
    tap_count(0),
    first_tap_time(0),
    long_press_fired(false),
    waiting_for_double_tap(false),
    suppress_until_release(false),
    pendingEvent(BTN_EVENT_NONE) {}

void BootselButtonHandler::begin() {
  // No pin setup needed for BOOTSEL
  last_raw = readButton();
  current_state = last_raw;
  suppress_until_release = false;
}

void BootselButtonHandler::suppressUntilRelease() {
  uint32_t now = millis();
  last_raw = readButton();
  current_state = last_raw;
  last_change_time = now;
  press_start_time = 0;
  tap_count = 0;
  first_tap_time = 0;
  long_press_fired = false;
  waiting_for_double_tap = false;
  suppress_until_release = true;
  pendingEvent = BTN_EVENT_NONE;
}

bool BootselButtonHandler::isPressed() {
  return current_state;
}

void BootselButtonHandler::poll() {
  if (button_handler_internal::updateSuppressedState(
        readButton(), millis(),
        last_raw, current_state,
        last_change_time, press_start_time,
        tap_count, first_tap_time,
        long_press_fired, waiting_for_double_tap,
        suppress_until_release, pendingEvent)) {
    return;
  }

  button_handler_internal::pollState(
    readButton(), millis(),
    last_raw, current_state,
    last_change_time, press_start_time,
    tap_count, first_tap_time,
    long_press_fired, waiting_for_double_tap,
    pendingEvent
  );
}

ButtonEvent BootselButtonHandler::update() {
  if (button_handler_internal::updateSuppressedState(
        readButton(), millis(),
        last_raw, current_state,
        last_change_time, press_start_time,
        tap_count, first_tap_time,
        long_press_fired, waiting_for_double_tap,
        suppress_until_release, pendingEvent)) {
    return BTN_EVENT_NONE;
  }

  return button_handler_internal::updateState(
    readButton(), millis(),
    last_raw, current_state,
    last_change_time, press_start_time,
    tap_count, first_tap_time,
    long_press_fired, waiting_for_double_tap,
    pendingEvent, false
  );
}
