#pragma once

#include <stdint.h>

#include "../../core/controller_settings_state.h"
#include "../../core/dpad_mode.h"
#include "../output_mode.h"

enum bcd_input_platform_enum {
  BCD_PLAT_N64 = 0,
  BCD_PLAT_GC,
  BCD_PLAT_GBA,

  BCD_PLAT_MEGADRIVE,
  BCD_PLAT_SATURN,

  BCD_PLAT_PCE,

  BCD_PLAT_NES,
  BCD_PLAT_SNES,
  BCD_PLAT_VBOY,

  BCD_PLAT_NEOGEO,

  BCD_PLAT_WII,

  BCD_PLAT_3DO,

  BCD_PLAT_JAGUAR,

  BCD_PLAT_DREAMCAST,

  BCD_PLAT_INTV,

  BCD_PLAT_PADDLE,
  BCD_PLAT_DRIVING,

  BCD_PLAT_GAMEPORT,

  BCD_PLAT_MEMCARD,

  BCD_PLAT_SMS,
  BCD_PLAT_JPC,
  BCD_PLAT_PSX = 22,
  BCD_PLAT_ATARI,
  BCD_PLAT_JVS,
};

struct __attribute((packed, aligned(1))) bcd_device_version_t {
  union {
    struct {
      uint16_t platform_sub : 4;
      uint16_t platform     : 6;
      uint16_t revision     : 6;
    };
    uint16_t composite;
  };
};

extern bcd_device_version_t bcd_device_version;

#define WEBHID_RAW_DATA_SIZE 32
extern uint8_t webhid_raw_data[WEBHID_RAW_DATA_SIZE];
extern uint8_t webhid_raw_data_len;

void webhid_store_raw_data(const uint8_t* data, uint8_t len);
void webhid_update_device_mode(uint8_t mode);

enum n64_cstick_mode_enum : uint8_t {
  N64CSTICK_AUTO = 0,
  N64CSTICK_AS_BUTTONS,
  N64CSTICK_AS_RS,
};

#ifdef __cplusplus
extern "C" {
#endif

n64_cstick_mode_enum get_effective_n64_cstick_mode(void);
outputMode_t output_mode_for_effective_n64_cstick(void);

#ifdef __cplusplus
}
#endif
