#include "button_handler_internal.h"

namespace button_handler_internal {

void pollState(bool raw, uint32_t now,
               bool& last_raw, bool& current_state,
               uint32_t& last_change_time, uint32_t& press_start_time,
               uint8_t& tap_count, uint32_t& first_tap_time,
               bool& long_press_fired, bool& waiting_for_double_tap,
               ButtonEvent& pendingEvent) {
  ButtonEvent event = BTN_EVENT_NONE;

  if (raw != last_raw) {
    last_change_time = now;
    last_raw = raw;
  }

  if ((now - last_change_time) >= BTN_DEBOUNCE_MS) {
    bool prev_state = current_state;
    current_state = raw;

    if (current_state && !prev_state) {
      press_start_time = now;
      long_press_fired = false;
      tap_count++;
      if (tap_count == 1) {
        first_tap_time = now;
        waiting_for_double_tap = true;
      }
    }

    if (!current_state && prev_state) {
      if (long_press_fired) {
        event = BTN_EVENT_LONG_RELEASE;
      } else if (tap_count >= 2) {
        event = BTN_EVENT_DOUBLE;
      } else if (tap_count == 1) {
        event = BTN_EVENT_SINGLE;
      }
      tap_count = 0;
      waiting_for_double_tap = false;
    }
  }

  if (current_state && !long_press_fired && raw) {
    if ((now - press_start_time) >= BTN_LONG_PRESS_MS) {
      event = BTN_EVENT_LONG;
      long_press_fired = true;
      tap_count = 0;
      waiting_for_double_tap = false;
    }
  }

  if (event != BTN_EVENT_NONE) {
    pendingEvent = event;
  }
}

bool updateSuppressedState(bool raw, uint32_t now,
                           bool& last_raw, bool& current_state,
                           uint32_t& last_change_time,
                           uint32_t& press_start_time,
                           uint8_t& tap_count, uint32_t& first_tap_time,
                           bool& long_press_fired, bool& waiting_for_double_tap,
                           bool& suppress_until_release,
                           ButtonEvent& pendingEvent) {
  if (!suppress_until_release) {
    return false;
  }

  if (raw != last_raw) {
    last_change_time = now;
    last_raw = raw;
  }

  if ((now - last_change_time) >= BTN_DEBOUNCE_MS) {
    current_state = raw;
    if (!current_state) {
      press_start_time = 0;
      tap_count = 0;
      first_tap_time = 0;
      long_press_fired = false;
      waiting_for_double_tap = false;
      pendingEvent = BTN_EVENT_NONE;
      suppress_until_release = false;
    }
  }

  return true;
}

ButtonEvent updateState(bool raw, uint32_t now,
                        bool& last_raw, bool& current_state,
                        uint32_t& last_change_time, uint32_t& press_start_time,
                        uint8_t& tap_count, uint32_t& first_tap_time,
                        bool& long_press_fired, bool& waiting_for_double_tap,
                        ButtonEvent& pendingEvent, bool fire_single_on_release) {
  ButtonEvent event = BTN_EVENT_NONE;

  if (raw != last_raw) {
    last_change_time = now;
    last_raw = raw;
  }

  if ((now - last_change_time) >= BTN_DEBOUNCE_MS) {
    bool prev_state = current_state;
    current_state = raw;

    if (current_state && !prev_state) {
      press_start_time = now;
      long_press_fired = false;
      tap_count++;
      if (tap_count == 1) {
        first_tap_time = now;
        waiting_for_double_tap = true;
      }
    }

    if (!current_state && prev_state) {
      if (long_press_fired) {
        event = BTN_EVENT_LONG_RELEASE;
        tap_count = 0;
        waiting_for_double_tap = false;
      } else if (tap_count >= 2) {
        event = BTN_EVENT_DOUBLE;
        tap_count = 0;
        waiting_for_double_tap = false;
      } else if (tap_count == 1 && fire_single_on_release) {
        event = BTN_EVENT_SINGLE;
        tap_count = 0;
        waiting_for_double_tap = false;
      }
    }
  }

  if (current_state && !long_press_fired && raw) {
    if ((now - press_start_time) >= BTN_LONG_PRESS_MS) {
      event = BTN_EVENT_LONG;
      long_press_fired = true;
      tap_count = 0;
      waiting_for_double_tap = false;
    }
  }

  if (waiting_for_double_tap && !current_state && tap_count == 1) {
    if ((now - first_tap_time) >= BTN_DOUBLE_TAP_MS) {
      event = BTN_EVENT_SINGLE;
      tap_count = 0;
      waiting_for_double_tap = false;
    }
  }

  if (waiting_for_double_tap && tap_count >= 1) {
    if ((now - first_tap_time) >= BTN_DOUBLE_TAP_MS && !current_state) {
      if (tap_count == 1 && event == BTN_EVENT_NONE) {
        event = BTN_EVENT_SINGLE;
      }
      tap_count = 0;
      waiting_for_double_tap = false;
    }
  }

  if (event != BTN_EVENT_NONE) {
    pendingEvent = event;
  }

  if (pendingEvent != BTN_EVENT_NONE) {
    ButtonEvent result = pendingEvent;
    pendingEvent = BTN_EVENT_NONE;
    return result;
  }

  return BTN_EVENT_NONE;
}

}  // namespace button_handler_internal
