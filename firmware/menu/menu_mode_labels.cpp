#include "../product_config.h"

#include "menu_mode_labels.h"

#include "menu_virtual_output_pad.h"

const char* getInputShortName(DeviceEnum mode) {
  switch (mode) {
    #ifdef ENABLE_INPUT_USB
      case RZORD_USB: return "USB";
    #endif
    #ifdef ENABLE_INPUT_ESP32_SPI
      case RZORD_ESP32_SPI: return "ESP32 BT";
    #endif
    #ifdef ENABLE_INPUT_JVS
      case RZORD_JVS: return "JVS";
    #endif
    #ifdef ENABLE_INPUT_DUMMY
      case RZORD_DUMMY: return "Dummy";
    #endif
    #ifdef ENABLE_INPUT_CUSTOM
      case RZORD_CUSTOM: return "Custom";
    #endif
    #ifdef ENABLE_INPUT_SATURN
      case RZORD_SATURN: return "Saturn";
    #endif
    #ifdef ENABLE_INPUT_MEGADRIVE
      case RZORD_MEGADRIVE: return "Genesis";
    #endif
    #ifdef ENABLE_INPUT_N64
      case RZORD_N64: return "N64";
    #endif
    #ifdef ENABLE_INPUT_GAMECUBE
      case RZORD_GAMECUBE: return "GameCube";
    #endif
    #ifdef ENABLE_INPUT_GBA
      case RZORD_GBA: return "GBA";
    #endif
    #ifdef ENABLE_INPUT_NES
      case RZORD_NES: return "NES";
    #endif
    #ifdef ENABLE_INPUT_SNES
      case RZORD_SNES: return "SNES";
    #endif
    #ifdef ENABLE_INPUT_VBOY
      case RZORD_VBOY: return "Virtual Boy";
    #endif
    #ifdef ENABLE_INPUT_PCE
      case RZORD_PCE: return "PCE/TG-16";
    #endif
    #ifdef ENABLE_INPUT_WII
      case RZORD_WII: return "Wii";
    #endif
    #ifdef ENABLE_INPUT_PSX
      case RZORD_PSX: return "PSX";
    #endif
    #ifdef ENABLE_INPUT_PSX_JOG
      case RZORD_PSX_JOG: return "JogCon";
    #endif
    #ifdef ENABLE_INPUT_PSX_DANCE
      case RZORD_PSX_DANCE: return "PSX Dance";
    #endif
    #ifdef ENABLE_INPUT_NEOGEO
      case RZORD_NEOGEO: return "NeoGeo";
    #endif
    #ifdef ENABLE_INPUT_3DO
      case RZORD_3DO: return "3DO";
    #endif
    #ifdef ENABLE_INPUT_JAGUAR
      case RZORD_JAGUAR: return "Jaguar";
    #endif
    #ifdef ENABLE_INPUT_DREAMCAST
      case RZORD_DREAMCAST: return "Dreamcast";
    #endif
    #ifdef ENABLE_INPUT_INTV
      case RZORD_INTV: return "INTV";
    #endif
    #ifdef ENABLE_INPUT_PADDLE
      case RZORD_PADDLE: return "Paddle";
    #endif
    #ifdef ENABLE_INPUT_DRIVING
      case RZORD_DRIVING: return "Atari Driver";
    #endif
    #ifdef ENABLE_INPUT_GAMEPORT
      case RZORD_GAMEPORT: return "Gameport";
    #endif
    #ifdef ENABLE_INPUT_MEMCARD
      case RZORD_MEMCARD: return "MemCard";
    #endif
    #ifdef ENABLE_INPUT_AUTODETECT
      case RZORD_AUTODETECT: return "Auto";
    #endif
    #ifdef ENABLE_INPUT_SMS
      case RZORD_SMS: return "Atari/C64/SMS";
    #endif
    #ifdef ENABLE_INPUT_JPC
      case RZORD_JPC: return "MSX/FM/X68K";
    #endif
    default: return "???";
  }
}

const char* getInputCompactName(DeviceEnum mode) {
  switch (mode) {
    #ifdef ENABLE_INPUT_VBOY
      case RZORD_VBOY: return "VB";
    #endif
    #ifdef ENABLE_INPUT_SMS
      case RZORD_SMS: return "Atari/SMS";
    #endif
    #ifdef ENABLE_INPUT_DRIVING
      case RZORD_DRIVING: return "AtariDrv";
    #endif
    #ifdef ENABLE_INPUT_JPC
      case RZORD_JPC: return "MSX/X68K";
    #endif
    #ifdef ENABLE_INPUT_ESP32_SPI
      case RZORD_ESP32_SPI: return "BT";
    #endif
    default: return getInputShortName(mode);
  }
}

const char* getOutputShortName(outputMode_t mode) {
  switch (mode) {
    case OUTPUT_HID: return "DInput";
    case OUTPUT_MISTER: return "DInput";
    case OUTPUT_MISTER_JOGCON: return "MiSTer-Jog";
    case OUTPUT_RESERVED_JOGCON: return "MiSTer-Jog";
    case OUTPUT_MISTER_NEGCON: return "MiSTer-Neg";
    case OUTPUT_MISTER_GUNCON: return "MiSTer-Gun";
    case OUTPUT_XINPUT: return "Xbox 360";
    case OUTPUT_XINPUTW: return "XInput";
    case OUTPUT_XINPUT2P: return "XInput";
    case OUTPUT_XBOXONE: return "Xbox One";
    case OUTPUT_XID: return "Xbox Classic";
    case OUTPUT_PS3: return "PS3";
    case OUTPUT_PS4: return "PS4";
    case OUTPUT_PS5: return "PS5";
    case OUTPUT_SWITCH: return "Switch Pokken";
    case OUTPUT_SWITCHPRO: return "Switch Pro";
    case OUTPUT_PANTHERLORD: return "PSX Mini";
    case OUTPUT_GCWIIU: return "Wii U GC Adapter";
    case OUTPUT_MDMINI: return "Genesis Mini";
    #ifdef ENABLE_OUTPUT_JVS
    case OUTPUT_JVS: return "JVS Arcade";
    #endif
    #ifdef ENABLE_OUTPUT_ESP32_BT
    case OUTPUT_ESP32_BT: return "Bluetooth";
    #endif
    #ifdef ENABLE_OUTPUT_CONSOLE
    case OUTPUT_CONSOLE_NES: return "NES";
    case OUTPUT_CONSOLE_SNES: return "SNES";
    case OUTPUT_CONSOLE_N64: return "N64";
    case OUTPUT_CONSOLE_GC: return "GameCube";
    case OUTPUT_CONSOLE_SATURN: return "Saturn";
    case OUTPUT_CONSOLE_GENESIS: return "Genesis";
    case OUTPUT_CONSOLE_WII: return "Wii Classic";
    case OUTPUT_CONSOLE_AUTO: return "Console Auto";
    #endif
    #ifdef ENABLE_OUTPUT_DB15
    case OUTPUT_DB15_SUPERGUN: return "DB15 Supergun";
    #endif
    case OUTPUT_AUTO: return "Auto";
    case OUTPUT_KEYBOARD: return "Keyboard";
    case OUTPUT_RESERVED_MOUSE: return "DInput";
    default: return "???";
  }
}

const char* getOutputCompactName(outputMode_t mode) {
  switch (mode) {
    case OUTPUT_HID: return "DIn";
    case OUTPUT_MISTER: return "DIn";
    case OUTPUT_RESERVED_JOGCON: return "MiSTer-Jog";
    case OUTPUT_XINPUT: return "XB360";
    case OUTPUT_XINPUTW: return "XIn";
    case OUTPUT_XINPUT2P: return "XIn";
    case OUTPUT_XBOXONE: return "XBOne";
    case OUTPUT_XID: return "Xbox OG";
    case OUTPUT_PANTHERLORD: return "PSX Mini";
    case OUTPUT_GCWIIU: return "WiiU GC";
    case OUTPUT_SWITCH: return "Pokken";
    case OUTPUT_MDMINI: return "MD Mini";
    #ifdef ENABLE_OUTPUT_ESP32_BT
    case OUTPUT_ESP32_BT: return "BT";
    #endif
    #ifdef ENABLE_OUTPUT_CONSOLE
    case OUTPUT_CONSOLE_NES: return "NES";
    case OUTPUT_CONSOLE_SNES: return "SNES";
    case OUTPUT_CONSOLE_N64: return "N64";
    case OUTPUT_CONSOLE_GC: return "GC";
    case OUTPUT_CONSOLE_SATURN: return "Saturn";
    case OUTPUT_CONSOLE_GENESIS: return "Genesis";
    case OUTPUT_CONSOLE_WII: return "Wii";
    case OUTPUT_CONSOLE_AUTO: return "Con Auto";
    #endif
    #ifdef ENABLE_OUTPUT_DB15
    case OUTPUT_DB15_SUPERGUN: return "DB15";
    #endif
    case OUTPUT_AUTO: return "Auto";
    case OUTPUT_KEYBOARD: return "KBD";
    default: return getOutputShortName(mode);
  }
}

const char* getVirtualOutputPadName(DeviceEnum inputMode, outputMode_t mode) {
  switch (mode) {
    case OUTPUT_HID:
    case OUTPUT_MISTER: return "DInput";
    case OUTPUT_MISTER_JOGCON:
    case OUTPUT_RESERVED_JOGCON: return "JogCon";
    case OUTPUT_MISTER_NEGCON: return "NeGcon";
    case OUTPUT_XINPUT:
    case OUTPUT_XINPUTW:
    case OUTPUT_XINPUT2P: return "Xbox";
    case OUTPUT_XBOXONE: return "Xbox One";
    case OUTPUT_XID: return "Xbox OG";
    case OUTPUT_PS3: return "DualShock3";
    case OUTPUT_PS4: return "DualShock4";
    case OUTPUT_PS5: return "DualSense";
    case OUTPUT_SWITCH: return "Pokken";
    case OUTPUT_SWITCHPRO: return "Switch Pro";
    case OUTPUT_PANTHERLORD: return "PSX Mini";
    case OUTPUT_GCWIIU: return "Wii U GC Adapter";
    #ifdef ENABLE_OUTPUT_JVS
    case OUTPUT_JVS: return "JVS";
    #endif
    case OUTPUT_MDMINI: return "MD Mini";
    default:
      return shouldShowVirtualOutputPad(inputMode, mode) ? getOutputCompactName(mode) : nullptr;
  }
}

outputMode_t getAutoDetectedOutputMode(AutoDetectState state) {
  switch (state) {
    case AUTO_STATE_PS3: return OUTPUT_PS3;
    case AUTO_STATE_WINDOWS: return get_configured_windows_auto_output_mode();
    case AUTO_STATE_PS4: return OUTPUT_PS4;
    case AUTO_STATE_PS5: return OUTPUT_PS5;
    case AUTO_STATE_PLAYSTATION: return OUTPUT_PS4;
    case AUTO_STATE_OG_XBOX: return OUTPUT_XID;
    case AUTO_STATE_XBOX_360: return OUTPUT_XINPUT;
    case AUTO_STATE_SWITCH: return OUTPUT_SWITCHPRO;
    case AUTO_STATE_FALLBACK_HID: return OUTPUT_MISTER;
    default: return OUTPUT_MISTER;
  }
}
