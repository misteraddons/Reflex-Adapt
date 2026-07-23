#pragma once

#include <stdint.h>

#include "../../../core/device_mode.h"

// Stable WebHID input mode IDs (kept independent from compile-time DeviceEnum order)
enum webhid_input_mode_id_t : uint8_t {
  WEBHID_MODE_NONE        = 0,
  WEBHID_MODE_AUTODETECT  = 1,
  WEBHID_MODE_SMS         = 2,
  WEBHID_MODE_DRIVING     = 3,
  WEBHID_MODE_MEGADRIVE   = 4,
  WEBHID_MODE_SATURN      = 5,
  WEBHID_MODE_NES         = 6,
  WEBHID_MODE_SNES        = 7,
  WEBHID_MODE_N64         = 8,
  WEBHID_MODE_GAMECUBE    = 9,
  WEBHID_MODE_WII         = 10,
  WEBHID_MODE_VBOY        = 11,
  WEBHID_MODE_PCE         = 12,
  WEBHID_MODE_PSX         = 13,
  WEBHID_MODE_PSX_JOG     = 14,
  WEBHID_MODE_NEOGEO      = 15,
  WEBHID_MODE_3DO         = 16,
  WEBHID_MODE_JAGUAR      = 17,
  WEBHID_MODE_JPC         = 18,
  WEBHID_MODE_DREAMCAST   = 19,
  WEBHID_MODE_USB         = 20,
  WEBHID_MODE_JVS         = 21,
  WEBHID_MODE_DUMMY       = 22,
  WEBHID_MODE_CUSTOM      = 23,
  WEBHID_MODE_GBA         = 24,
  WEBHID_MODE_INTV        = 26,
  WEBHID_MODE_PADDLE      = 27,
  WEBHID_MODE_GAMEPORT    = 28,
  WEBHID_MODE_MEMCARD     = 29,
  WEBHID_MODE_PSX_DANCE   = 30,
};

uint8_t webhid_input_mode_from_device(DeviceEnum mode);
DeviceEnum webhid_input_mode_to_device(uint8_t mode_id, DeviceEnum fallback = RZORD_NONE);
const char* webhid_input_mode_name(uint8_t mode_id);
