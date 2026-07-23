#include "../product_config.h"

#include "runtime/output_boot_bridge.h"
#include "output_runtime_state.h"

#ifndef XINPUT_WIRELESS_CONTROLLERS
  #define XINPUT_WIRELESS_CONTROLLERS 4
#endif
#ifndef XINPUT_MULTI_CONTROLLERS
  #define XINPUT_MULTI_CONTROLLERS 2
#endif

namespace {

uint8_t classic2usbHidOutputSlots() {
#if defined(PRODUCT_CLASSIC2USB)
  return (MAX_USB_OUT < 2) ? MAX_USB_OUT : 2;
#else
  return MAX_USB_OUT;
#endif
}

}  // namespace

bool must_set_serial_string(outputMode_t mode) {
  switch (mode) {
    case OUTPUT_HID:
    case OUTPUT_MISTER:
    case OUTPUT_MISTER_JOGCON:
    case OUTPUT_MISTER_NEGCON:
    case OUTPUT_MISTER_GUNCON:
      return true;
    default:
      return false;
  }
}

uint8_t get_max_output_for_mode(outputMode_t mode) {
  switch (mode) {
    case OUTPUT_HID:              return classic2usbHidOutputSlots();
    case OUTPUT_MISTER:           return classic2usbHidOutputSlots();
    case OUTPUT_MISTER_JOGCON:    return classic2usbHidOutputSlots();
    case OUTPUT_MISTER_NEGCON:    return classic2usbHidOutputSlots();
    case OUTPUT_MISTER_GUNCON:    return 1;
    case OUTPUT_RESERVED_JOGCON:  return classic2usbHidOutputSlots();
    case OUTPUT_RESERVED_MOUSE:   return 1;
    case OUTPUT_XINPUT:           return 1;
    case OUTPUT_XINPUTW:          return XINPUT_WIRELESS_CONTROLLERS;
    case OUTPUT_XINPUT2P:         return XINPUT_MULTI_CONTROLLERS;
    case OUTPUT_XID:              return 1;
    case OUTPUT_PS3:              return 1;
    case OUTPUT_PS4:              return 1;
    case OUTPUT_PS5:              return 1;
    case OUTPUT_SWITCH:           return classic2usbHidOutputSlots();
    case OUTPUT_SWITCHPRO:        return classic2usbHidOutputSlots();
    case OUTPUT_PANTHERLORD:      return 2;
    case OUTPUT_GCWIIU:           return 4;
    case OUTPUT_MDMINI:           return 1;
#ifdef ENABLE_OUTPUT_JVS
    case OUTPUT_JVS:              return 2;
#endif
#ifdef ENABLE_OUTPUT_CONSOLE
    case OUTPUT_CONSOLE_NES:      return 1;
    case OUTPUT_CONSOLE_SNES:     return 1;
    case OUTPUT_CONSOLE_N64:      return 1;
    case OUTPUT_CONSOLE_GC:       return 1;
    case OUTPUT_CONSOLE_SATURN:   return 1;
    case OUTPUT_CONSOLE_GENESIS:  return 1;
    case OUTPUT_CONSOLE_WII:      return 1;
    case OUTPUT_CONSOLE_AUTO:     return 1;
#endif
#ifdef ENABLE_OUTPUT_DB15
    case OUTPUT_DB15_SUPERGUN:    return 1;
#endif
    case OUTPUT_KEYBOARD:         return 2;
    case OUTPUT_XBOXONE:          return 1;
    default:                      return 1;
  }
}

const char* get_mode_name(outputMode_t mode) {
  switch (mode) {
    case OUTPUT_HID:              return "DInput";
    case OUTPUT_MISTER:           return "DInput";
    case OUTPUT_MISTER_JOGCON:    return "MiSTer JogCon";
    case OUTPUT_RESERVED_JOGCON:  return "MiSTer JogCon";
    case OUTPUT_MISTER_NEGCON:    return "MiSTer NegCon";
    case OUTPUT_MISTER_GUNCON:    return "MiSTer GunCon";
    case OUTPUT_RESERVED_MOUSE:   return "DInput";
    case OUTPUT_XINPUT:           return "Xbox 360";
    case OUTPUT_XINPUTW:          return "XInput";
    case OUTPUT_XINPUT2P:         return "XInput";
    case OUTPUT_XID:              return "Xbox Classic";
    case OUTPUT_PS3:              return "PS3";
    case OUTPUT_PS4:              return "PS4";
    case OUTPUT_PS5:              return "PS5";
    case OUTPUT_SWITCH:           return "Switch Pokken";
    case OUTPUT_SWITCHPRO:        return "Switch Pro";
    case OUTPUT_PANTHERLORD:      return "PSX Mini";
    case OUTPUT_GCWIIU:           return "Wii U GC Adapter";
    case OUTPUT_MDMINI:           return "Genesis Mini";
#ifdef ENABLE_OUTPUT_JVS
    case OUTPUT_JVS:              return "JVS Arcade";
#endif
    case OUTPUT_AUTO:             return "Auto";
    case OUTPUT_KEYBOARD:         return "Keyboard (MAME)";
    case OUTPUT_XBOXONE:          return "Xbox One";
#ifdef ENABLE_OUTPUT_ESP32_BT
    case OUTPUT_ESP32_BT:         return "Bluetooth";
#endif
#ifdef ENABLE_OUTPUT_CONSOLE
    case OUTPUT_CONSOLE_NES:      return "NES";
    case OUTPUT_CONSOLE_SNES:     return "SNES";
    case OUTPUT_CONSOLE_N64:      return "N64";
    case OUTPUT_CONSOLE_GC:       return "GameCube";
    case OUTPUT_CONSOLE_SATURN:   return "Saturn";
    case OUTPUT_CONSOLE_GENESIS:  return "Genesis";
    case OUTPUT_CONSOLE_WII:      return "Wii Classic";
    case OUTPUT_CONSOLE_AUTO:     return "Console Auto";
#endif
#ifdef ENABLE_OUTPUT_DB15
    case OUTPUT_DB15_SUPERGUN:    return "DB15 Supergun";
#endif
    default:                      return "MODE ERROR";
  }
}
