#pragma once

#include <stdint.h>

#include "../input/shared/input_button_bits.h"

struct hotkey_t {
  uint32_t hotkey;
  uint32_t result;
  bool exclusive;
  bool clear;
};

extern const uint32_t hotkey_bootsel;
extern const uint32_t hotkey_dpad_as_buttons;
extern const hotkey_t hotkeys[];
extern const uint8_t hotkey_count;
