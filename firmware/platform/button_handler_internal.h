#pragma once

#include "button_handler.h"

namespace button_handler_internal {

void pollState(bool raw, uint32_t now,
               bool& last_raw, bool& current_state,
               uint32_t& last_change_time, uint32_t& press_start_time,
               uint8_t& tap_count, uint32_t& first_tap_time,
               bool& long_press_fired, bool& waiting_for_double_tap,
               ButtonEvent& pendingEvent);

bool updateSuppressedState(bool raw, uint32_t now,
                          bool& last_raw, bool& current_state,
                          uint32_t& last_change_time,
                          uint32_t& press_start_time,
                          uint8_t& tap_count, uint32_t& first_tap_time,
                          bool& long_press_fired, bool& waiting_for_double_tap,
                          bool& suppress_until_release,
                          ButtonEvent& pendingEvent);

ButtonEvent updateState(bool raw, uint32_t now,
                        bool& last_raw, bool& current_state,
                        uint32_t& last_change_time, uint32_t& press_start_time,
                        uint8_t& tap_count, uint32_t& first_tap_time,
                        bool& long_press_fired, bool& waiting_for_double_tap,
                        ButtonEvent& pendingEvent, bool fire_single_on_release);

}  // namespace button_handler_internal
