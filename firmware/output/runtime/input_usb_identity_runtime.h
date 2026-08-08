#pragma once

#include "../../core/device_mode.h"
#include "input_runtime_output_bridge.h"

inline bool inputModeBcdPlatform(DeviceEnum mode, uint8_t& platform, uint8_t& platformSub) {
  platformSub = 0;
  switch (mode) {
    #ifdef ENABLE_INPUT_N64
    case RZORD_N64:       platform = BCD_PLAT_N64; return true;
    #endif
    #ifdef ENABLE_INPUT_GAMECUBE
    case RZORD_GAMECUBE:  platform = BCD_PLAT_GC; return true;
    #endif
    #ifdef ENABLE_INPUT_GBA
    case RZORD_GBA:       platform = BCD_PLAT_GBA; return true;
    #endif
    #ifdef ENABLE_INPUT_MEGADRIVE
    case RZORD_MEGADRIVE: platform = BCD_PLAT_MEGADRIVE; return true;
    #endif
    #ifdef ENABLE_INPUT_SATURN
    case RZORD_SATURN:    platform = BCD_PLAT_SATURN; return true;
    #endif
    #ifdef ENABLE_INPUT_PCE
    case RZORD_PCE:       platform = BCD_PLAT_PCE; return true;
    #endif
    #ifdef ENABLE_INPUT_NES
    case RZORD_NES:       platform = BCD_PLAT_NES; return true;
    #endif
    #ifdef ENABLE_INPUT_SNES
    case RZORD_SNES:      platform = BCD_PLAT_SNES; return true;
    #endif
    #ifdef ENABLE_INPUT_VBOY
    case RZORD_VBOY:      platform = BCD_PLAT_VBOY; return true;
    #endif
    #ifdef ENABLE_INPUT_NEOGEO
    case RZORD_NEOGEO:    platform = BCD_PLAT_NEOGEO; return true;
    #endif
    #ifdef ENABLE_INPUT_WII
    case RZORD_WII:       platform = BCD_PLAT_WII; return true;
    #endif
    #ifdef ENABLE_INPUT_3DO
    case RZORD_3DO:       platform = BCD_PLAT_3DO; return true;
    #endif
    #ifdef ENABLE_INPUT_JAGUAR
    case RZORD_JAGUAR:    platform = BCD_PLAT_JAGUAR; return true;
    #endif
    #ifdef ENABLE_INPUT_DREAMCAST
    case RZORD_DREAMCAST: platform = BCD_PLAT_DREAMCAST; return true;
    #endif
    #ifdef ENABLE_INPUT_INTV
    case RZORD_INTV:      platform = BCD_PLAT_INTV; return true;
    #endif
    #ifdef ENABLE_INPUT_PADDLE
    case RZORD_PADDLE:    platform = BCD_PLAT_PADDLE; return true;
    #endif
    #ifdef ENABLE_INPUT_DRIVING
    case RZORD_DRIVING:   platform = BCD_PLAT_DRIVING; return true;
    #endif
    #ifdef ENABLE_INPUT_GAMEPORT
    case RZORD_GAMEPORT:  platform = BCD_PLAT_GAMEPORT; return true;
    #endif
    #ifdef ENABLE_INPUT_MEMCARD
    case RZORD_MEMCARD:   platform = BCD_PLAT_MEMCARD; return true;
    #endif
    #ifdef ENABLE_INPUT_SMS
    case RZORD_SMS:       platform = BCD_PLAT_SMS; return true;
    #endif
    #ifdef ENABLE_INPUT_JPC
    case RZORD_JPC:       platform = BCD_PLAT_JPC; return true;
    #endif
    #ifdef ENABLE_INPUT_PSX
    case RZORD_PSX:       platform = BCD_PLAT_PSX; return true;
    #endif
    #ifdef ENABLE_INPUT_PSX_JOG
    case RZORD_PSX_JOG:   platform = BCD_PLAT_PSX; return true;
    #endif
    #ifdef ENABLE_INPUT_PSX_DANCE
    case RZORD_PSX_DANCE: platform = BCD_PLAT_PSX; return true;
    #endif
    #ifdef ENABLE_INPUT_JVS
    case RZORD_JVS:       platform = BCD_PLAT_JVS; return true;
    #endif
    default:
      platform = BCD_PLAT_N64;
      return false;
  }
}

inline bool applyInputModeBcdDeviceVersion(DeviceEnum mode) {
  uint8_t platform = BCD_PLAT_N64;
  uint8_t platformSub = 0;
  if (!inputModeBcdPlatform(mode, platform, platformSub)) {
    return false;
  }
  bcd_device_version.platform = platform;
  bcd_device_version.platform_sub = platformSub;
  return true;
}

inline bool inputModeNeedsBcdDeviceReenumeration(DeviceEnum mode) {
  uint8_t platform = BCD_PLAT_N64;
  uint8_t platformSub = 0;
  if (!inputModeBcdPlatform(mode, platform, platformSub)) {
    return false;
  }
  return bcd_device_version.platform != platform ||
         bcd_device_version.platform_sub != platformSub;
}
