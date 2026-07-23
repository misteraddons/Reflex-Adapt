#pragma once

#include <stdint.h>

#include "../product_config.h"

// Shared input-mode enum used across the firmware. It used to live directly in
// the monolithic entry file, which forced every consumer to depend on include
// order inside firmware.cpp.
enum DeviceEnum : uint8_t {
  RZORD_NONE = 0,

#ifdef ENABLE_INPUT_AUTODETECT
  RZORD_AUTODETECT,
#endif
#ifdef ENABLE_INPUT_DUMMY
  RZORD_DUMMY,
#endif
#ifdef ENABLE_INPUT_USB
  RZORD_USB,
#endif
#ifdef ENABLE_INPUT_ESP32_SPI
  RZORD_ESP32_SPI,
#endif
#ifdef ENABLE_INPUT_JVS
  RZORD_JVS,
#endif
#ifdef ENABLE_INPUT_CUSTOM
  RZORD_CUSTOM,
#endif
#ifdef ENABLE_INPUT_SMS
  RZORD_SMS,
#endif
#ifdef ENABLE_INPUT_DRIVING
  RZORD_DRIVING,
#endif
#ifdef ENABLE_INPUT_MEGADRIVE
  RZORD_MEGADRIVE,
#endif
#ifdef ENABLE_INPUT_SATURN
  RZORD_SATURN,
#endif
#ifdef ENABLE_INPUT_DREAMCAST
  RZORD_DREAMCAST,
#endif
#ifdef ENABLE_INPUT_NES
  RZORD_NES,
#endif
#ifdef ENABLE_INPUT_SNES
  RZORD_SNES,
#endif
#ifdef ENABLE_INPUT_N64
  RZORD_N64,
#endif
#ifdef ENABLE_INPUT_GAMECUBE
  RZORD_GAMECUBE,
#endif
#ifdef ENABLE_INPUT_WII
  RZORD_WII,
#endif
#ifdef ENABLE_INPUT_VBOY
  RZORD_VBOY,
#endif
#ifdef ENABLE_INPUT_GBA
  RZORD_GBA,
#endif
#ifdef ENABLE_INPUT_PCE
  RZORD_PCE,
#endif
#ifdef ENABLE_INPUT_PSX
  RZORD_PSX,
#endif
#ifdef ENABLE_INPUT_PSX_JOG
  RZORD_PSX_JOG,
#endif
#ifdef ENABLE_INPUT_PSX_DANCE
  RZORD_PSX_DANCE,
#endif
#ifdef ENABLE_INPUT_NEOGEO
  RZORD_NEOGEO,
#endif
#ifdef ENABLE_INPUT_3DO
  RZORD_3DO,
#endif
#ifdef ENABLE_INPUT_JAGUAR
  RZORD_JAGUAR,
#endif
#ifdef ENABLE_INPUT_INTV
  RZORD_INTV,
#endif
#ifdef ENABLE_INPUT_PADDLE
  RZORD_PADDLE,
#endif
#ifdef ENABLE_INPUT_GAMEPORT
  RZORD_GAMEPORT,
#endif
#ifdef ENABLE_INPUT_JPC
  RZORD_JPC,
#endif
#ifdef ENABLE_INPUT_MEMCARD
  RZORD_MEMCARD,
#endif

  RZORD_LAST
};

// Shared UI/helpers still refer to the canonical mode names even in slim
// product builds. Provide out-of-range sentinels for disabled modes so those
// switches compile, while runtime iteration still only sees the enabled enum
// entries above.
#ifndef ENABLE_INPUT_AUTODETECT
#define RZORD_AUTODETECT ((DeviceEnum)0x80)
#endif
#ifndef ENABLE_INPUT_DUMMY
#define RZORD_DUMMY ((DeviceEnum)0x81)
#endif
#ifndef ENABLE_INPUT_USB
#define RZORD_USB ((DeviceEnum)0x82)
#endif
#ifndef ENABLE_INPUT_ESP32_SPI
#define RZORD_ESP32_SPI ((DeviceEnum)0x9E)
#endif
#ifndef ENABLE_INPUT_JVS
#define RZORD_JVS ((DeviceEnum)0x83)
#endif
#ifndef ENABLE_INPUT_CUSTOM
#define RZORD_CUSTOM ((DeviceEnum)0x84)
#endif
#ifndef ENABLE_INPUT_SMS
#define RZORD_SMS ((DeviceEnum)0x85)
#endif
#ifndef ENABLE_INPUT_DRIVING
#define RZORD_DRIVING ((DeviceEnum)0x86)
#endif
#ifndef ENABLE_INPUT_MEGADRIVE
#define RZORD_MEGADRIVE ((DeviceEnum)0x87)
#endif
#ifndef ENABLE_INPUT_SATURN
#define RZORD_SATURN ((DeviceEnum)0x88)
#endif
#ifndef ENABLE_INPUT_DREAMCAST
#define RZORD_DREAMCAST ((DeviceEnum)0x89)
#endif
#ifndef ENABLE_INPUT_NES
#define RZORD_NES ((DeviceEnum)0x8A)
#endif
#ifndef ENABLE_INPUT_SNES
#define RZORD_SNES ((DeviceEnum)0x8B)
#endif
#ifndef ENABLE_INPUT_N64
#define RZORD_N64 ((DeviceEnum)0x8C)
#endif
#ifndef ENABLE_INPUT_GAMECUBE
#define RZORD_GAMECUBE ((DeviceEnum)0x8D)
#endif
#ifndef ENABLE_INPUT_WII
#define RZORD_WII ((DeviceEnum)0x8E)
#endif
#ifndef ENABLE_INPUT_VBOY
#define RZORD_VBOY ((DeviceEnum)0x8F)
#endif
#ifndef ENABLE_INPUT_GBA
#define RZORD_GBA ((DeviceEnum)0x90)
#endif
#ifndef ENABLE_INPUT_PCE
#define RZORD_PCE ((DeviceEnum)0x91)
#endif
#ifndef ENABLE_INPUT_PSX
#define RZORD_PSX ((DeviceEnum)0x93)
#endif
#ifndef ENABLE_INPUT_PSX_JOG
#define RZORD_PSX_JOG ((DeviceEnum)0x94)
#endif
#ifndef ENABLE_INPUT_PSX_DANCE
#define RZORD_PSX_DANCE ((DeviceEnum)0x95)
#endif
#ifndef ENABLE_INPUT_NEOGEO
#define RZORD_NEOGEO ((DeviceEnum)0x96)
#endif
#ifndef ENABLE_INPUT_3DO
#define RZORD_3DO ((DeviceEnum)0x97)
#endif
#ifndef ENABLE_INPUT_JAGUAR
#define RZORD_JAGUAR ((DeviceEnum)0x98)
#endif
#ifndef ENABLE_INPUT_INTV
#define RZORD_INTV ((DeviceEnum)0x99)
#endif
#ifndef ENABLE_INPUT_PADDLE
#define RZORD_PADDLE ((DeviceEnum)0x9A)
#endif
#ifndef ENABLE_INPUT_GAMEPORT
#define RZORD_GAMEPORT ((DeviceEnum)0x9B)
#endif
#ifndef ENABLE_INPUT_JPC
#define RZORD_JPC ((DeviceEnum)0x9C)
#endif
#ifndef ENABLE_INPUT_MEMCARD
#define RZORD_MEMCARD ((DeviceEnum)0x9D)
#endif

