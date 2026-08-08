#include "../../../product_config.h"

#include "webhid_input_modes.h"

uint8_t webhid_input_mode_from_device(DeviceEnum mode) {
  switch (mode) {
    case RZORD_NONE: return WEBHID_MODE_NONE;
#ifdef ENABLE_INPUT_AUTODETECT
    case RZORD_AUTODETECT: return WEBHID_MODE_AUTODETECT;
#endif
#ifdef ENABLE_INPUT_SMS
    case RZORD_SMS: return WEBHID_MODE_SMS;
#endif
#ifdef ENABLE_INPUT_DRIVING
    case RZORD_DRIVING: return WEBHID_MODE_DRIVING;
#endif
#ifdef ENABLE_INPUT_MEGADRIVE
    case RZORD_MEGADRIVE: return WEBHID_MODE_MEGADRIVE;
#endif
#ifdef ENABLE_INPUT_SATURN
    case RZORD_SATURN: return WEBHID_MODE_SATURN;
#endif
#ifdef ENABLE_INPUT_NES
    case RZORD_NES: return WEBHID_MODE_NES;
#endif
#ifdef ENABLE_INPUT_SNES
    case RZORD_SNES: return WEBHID_MODE_SNES;
#endif
#ifdef ENABLE_INPUT_N64
    case RZORD_N64: return WEBHID_MODE_N64;
#endif
#ifdef ENABLE_INPUT_GAMECUBE
    case RZORD_GAMECUBE: return WEBHID_MODE_GAMECUBE;
#endif
#ifdef ENABLE_INPUT_WII
    case RZORD_WII: return WEBHID_MODE_WII;
#endif
#ifdef ENABLE_INPUT_VBOY
    case RZORD_VBOY: return WEBHID_MODE_VBOY;
#endif
#ifdef ENABLE_INPUT_PCE
    case RZORD_PCE: return WEBHID_MODE_PCE;
#endif
#ifdef ENABLE_INPUT_PSX
    case RZORD_PSX: return WEBHID_MODE_PSX;
#endif
#ifdef ENABLE_INPUT_PSX_JOG
    case RZORD_PSX_JOG: return WEBHID_MODE_PSX_JOG;
#endif
#ifdef ENABLE_INPUT_NEOGEO
    case RZORD_NEOGEO: return WEBHID_MODE_NEOGEO;
#endif
#ifdef ENABLE_INPUT_3DO
    case RZORD_3DO: return WEBHID_MODE_3DO;
#endif
#ifdef ENABLE_INPUT_JAGUAR
    case RZORD_JAGUAR: return WEBHID_MODE_JAGUAR;
#endif
#ifdef ENABLE_INPUT_JPC
    case RZORD_JPC: return WEBHID_MODE_JPC;
#endif
#ifdef ENABLE_INPUT_DREAMCAST
    case RZORD_DREAMCAST: return WEBHID_MODE_DREAMCAST;
#endif
#ifdef ENABLE_INPUT_USB
    case RZORD_USB: return WEBHID_MODE_USB;
#endif
#ifdef ENABLE_INPUT_JVS
    case RZORD_JVS: return WEBHID_MODE_JVS;
#endif
#ifdef ENABLE_INPUT_DUMMY
    case RZORD_DUMMY: return WEBHID_MODE_DUMMY;
#endif
#ifdef ENABLE_INPUT_CUSTOM
    case RZORD_CUSTOM: return WEBHID_MODE_CUSTOM;
#endif
#ifdef ENABLE_INPUT_GBA
    case RZORD_GBA: return WEBHID_MODE_GBA;
#endif
#ifdef ENABLE_INPUT_INTV
    case RZORD_INTV: return WEBHID_MODE_INTV;
#endif
#ifdef ENABLE_INPUT_PADDLE
    case RZORD_PADDLE: return WEBHID_MODE_PADDLE;
#endif
#ifdef ENABLE_INPUT_GAMEPORT
    case RZORD_GAMEPORT: return WEBHID_MODE_GAMEPORT;
#endif
#ifdef ENABLE_INPUT_MEMCARD
    case RZORD_MEMCARD: return WEBHID_MODE_MEMCARD;
#endif
#ifdef ENABLE_INPUT_PSX_DANCE
    case RZORD_PSX_DANCE: return WEBHID_MODE_PSX_DANCE;
#endif
    default: return WEBHID_MODE_NONE;
  }
}

DeviceEnum webhid_input_mode_to_device(uint8_t mode_id, DeviceEnum fallback) {
  switch (mode_id) {
    case WEBHID_MODE_NONE: return RZORD_NONE;
#ifdef ENABLE_INPUT_AUTODETECT
    case WEBHID_MODE_AUTODETECT: return RZORD_AUTODETECT;
#endif
#ifdef ENABLE_INPUT_SMS
    case WEBHID_MODE_SMS: return RZORD_SMS;
#endif
#ifdef ENABLE_INPUT_DRIVING
    case WEBHID_MODE_DRIVING: return RZORD_DRIVING;
#endif
#ifdef ENABLE_INPUT_MEGADRIVE
    case WEBHID_MODE_MEGADRIVE: return RZORD_MEGADRIVE;
#endif
#ifdef ENABLE_INPUT_SATURN
    case WEBHID_MODE_SATURN: return RZORD_SATURN;
#endif
#ifdef ENABLE_INPUT_NES
    case WEBHID_MODE_NES: return RZORD_NES;
#endif
#ifdef ENABLE_INPUT_SNES
    case WEBHID_MODE_SNES: return RZORD_SNES;
#endif
#ifdef ENABLE_INPUT_N64
    case WEBHID_MODE_N64: return RZORD_N64;
#endif
#ifdef ENABLE_INPUT_GAMECUBE
    case WEBHID_MODE_GAMECUBE: return RZORD_GAMECUBE;
#endif
#ifdef ENABLE_INPUT_WII
    case WEBHID_MODE_WII: return RZORD_WII;
#endif
#ifdef ENABLE_INPUT_VBOY
    case WEBHID_MODE_VBOY: return RZORD_VBOY;
#endif
#ifdef ENABLE_INPUT_PCE
    case WEBHID_MODE_PCE: return RZORD_PCE;
#endif
#ifdef ENABLE_INPUT_PSX
    case WEBHID_MODE_PSX: return RZORD_PSX;
#endif
#ifdef ENABLE_INPUT_PSX_JOG
    case WEBHID_MODE_PSX_JOG: return RZORD_PSX_JOG;
#endif
#ifdef ENABLE_INPUT_NEOGEO
    case WEBHID_MODE_NEOGEO: return RZORD_NEOGEO;
#endif
#ifdef ENABLE_INPUT_3DO
    case WEBHID_MODE_3DO: return RZORD_3DO;
#endif
#ifdef ENABLE_INPUT_JAGUAR
    case WEBHID_MODE_JAGUAR: return RZORD_JAGUAR;
#endif
#ifdef ENABLE_INPUT_JPC
    case WEBHID_MODE_JPC: return RZORD_JPC;
#endif
#ifdef ENABLE_INPUT_DREAMCAST
    case WEBHID_MODE_DREAMCAST: return RZORD_DREAMCAST;
#endif
#ifdef ENABLE_INPUT_USB
    case WEBHID_MODE_USB: return RZORD_USB;
#endif
#ifdef ENABLE_INPUT_JVS
    case WEBHID_MODE_JVS: return RZORD_JVS;
#endif
#ifdef ENABLE_INPUT_DUMMY
    case WEBHID_MODE_DUMMY: return RZORD_DUMMY;
#endif
#ifdef ENABLE_INPUT_CUSTOM
    case WEBHID_MODE_CUSTOM: return RZORD_CUSTOM;
#endif
#ifdef ENABLE_INPUT_GBA
    case WEBHID_MODE_GBA: return RZORD_GBA;
#endif
#ifdef ENABLE_INPUT_INTV
    case WEBHID_MODE_INTV: return RZORD_INTV;
#endif
#ifdef ENABLE_INPUT_PADDLE
    case WEBHID_MODE_PADDLE: return RZORD_PADDLE;
#endif
#ifdef ENABLE_INPUT_GAMEPORT
    case WEBHID_MODE_GAMEPORT: return RZORD_GAMEPORT;
#endif
#ifdef ENABLE_INPUT_MEMCARD
    case WEBHID_MODE_MEMCARD: return RZORD_MEMCARD;
#endif
#ifdef ENABLE_INPUT_PSX_DANCE
    case WEBHID_MODE_PSX_DANCE: return RZORD_PSX_DANCE;
#endif
    default: return fallback;
  }
}

const char* webhid_input_mode_name(uint8_t mode_id) {
  switch (mode_id) {
    case WEBHID_MODE_AUTODETECT: return "Auto";
    case WEBHID_MODE_SMS: return "Atari/C64/SMS";
    case WEBHID_MODE_DRIVING: return "Atari Driver";
    case WEBHID_MODE_MEGADRIVE: return "Genesis";
    case WEBHID_MODE_SATURN: return "Saturn";
    case WEBHID_MODE_NES: return "NES";
    case WEBHID_MODE_SNES: return "SNES";
    case WEBHID_MODE_N64: return "N64";
    case WEBHID_MODE_GAMECUBE: return "GameCube";
    case WEBHID_MODE_WII: return "Wii";
    case WEBHID_MODE_VBOY: return "Virtual Boy";
    case WEBHID_MODE_PCE: return "PCE";
    case WEBHID_MODE_PSX: return "PSX";
    case WEBHID_MODE_PSX_JOG: return "PSX JogCon";
    case WEBHID_MODE_NEOGEO: return "NeoGeo";
    case WEBHID_MODE_3DO: return "3DO";
    case WEBHID_MODE_JAGUAR: return "Jaguar";
    case WEBHID_MODE_JPC: return "MSX/FM/X68K";
    case WEBHID_MODE_DREAMCAST: return "Dreamcast";
    case WEBHID_MODE_USB: return "USB";
    case WEBHID_MODE_JVS: return "JVS";
    case WEBHID_MODE_DUMMY: return "Dummy";
    case WEBHID_MODE_CUSTOM: return "Custom";
    case WEBHID_MODE_GBA: return "GBA";
    case WEBHID_MODE_INTV: return "INTV";
    case WEBHID_MODE_PADDLE: return "Paddle";
    case WEBHID_MODE_GAMEPORT: return "Gameport";
    case WEBHID_MODE_MEMCARD: return "MemCard";
    case WEBHID_MODE_PSX_DANCE: return "PSX Dance";
    default: return "None";
  }
}
