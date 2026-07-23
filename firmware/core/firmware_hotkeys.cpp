#include "firmware_hotkeys.h"

const uint32_t hotkey_bootsel = static_cast<uint32_t>(-1);
const uint32_t hotkey_dpad_as_buttons = static_cast<uint32_t>(-1);

const hotkey_t hotkeys[] = {
  {
    .hotkey = INPUT_PAD_D + INPUT_START,
    .result = INPUT_HOME,
    .exclusive = true,
    .clear = true,
  },
  {
    .hotkey = INPUT_PAD_U + INPUT_START,
    .result = INPUT_CAPTURE,
    .exclusive = true,
    .clear = true,
  },
};

const uint8_t hotkey_count = sizeof(hotkeys) / sizeof(hotkey_t);
