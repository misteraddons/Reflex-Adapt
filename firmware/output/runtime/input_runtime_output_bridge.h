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

struct bcd_device_version_t {
  uint8_t platform_sub = 0;
  uint8_t platform = 0;
  uint8_t revision = 0;
};

extern bcd_device_version_t bcd_device_version;

constexpr uint8_t pack_bcd_byte(uint8_t decimal) {
  return (uint8_t)(((decimal / 10u) << 4) | (decimal % 10u));
}

constexpr uint16_t encode_bcd_device_version(uint8_t revision,
                                             uint8_t platform,
                                             uint8_t platform_sub = 0) {
  const uint8_t identity = (uint8_t)(platform + (25u * platform_sub));
  return (uint16_t)((uint16_t)pack_bcd_byte(revision) << 8) |
         pack_bcd_byte(identity);
}

inline uint16_t current_bcd_device_version() {
  return encode_bcd_device_version(bcd_device_version.revision,
                                   bcd_device_version.platform,
                                   bcd_device_version.platform_sub);
}

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
