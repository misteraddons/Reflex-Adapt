#pragma once

#include "../product_config.h"
#include "../firmware_platform_config.h"

enum socdMode_t {
  SOCD_OFF = 0,
  SOCD_NEUTRAL,
  SOCD_SECOND,
  SOCD_FIRST,
  SOCD_LAST_ENUM // not used
};

enum outputMode_t {
  OUTPUT_HID = 0,         // Windows (DInput) HID
  OUTPUT_MISTER,          // MiSTer/Linux (DInput) HID
  OUTPUT_MISTER_JOGCON,   // MiSTer Jogcon (HID)
  OUTPUT_MISTER_NEGCON,   // MiSTer Negcon (HID)
  OUTPUT_MISTER_GUNCON,   // MiSTer Guncon (HID)
  OUTPUT_RESERVED_JOGCON, // Legacy saved value, canonicalized to MiSTer Jogcon
  OUTPUT_RESERVED_MOUSE,  // Legacy saved value, canonicalized to MiSTer DInput
  //OUTPUT_HID_GUNCONMOUSE,  // HID Guncon mouse
  // Guard: Xbox 360 console mode is the donor-style single-controller XUSB
  // transport with XSM3 security auth. Do not share this enum/runtime with the
  // Windows two-player XInput2P path; retail Xbox 360 auth traffic is the
  // reason this mode stays separate.
  OUTPUT_XINPUT,
  OUTPUT_XID,             // ogXbox (analog buttons)
  OUTPUT_PS3,             // dualshock3 (analog buttons)
  OUTPUT_PS4,             // requires auth. DS4 (v2) is supported
  OUTPUT_PS5,             // requires auth. DS4 does not work, requires specialized controller
  OUTPUT_SWITCH,          // pokken
  OUTPUT_SWITCHPRO,       // switch pro
  OUTPUT_PANTHERLORD,     // generic ps1/ps2 to usb adapter
  OUTPUT_GCWIIU,          // WUP-028
  OUTPUT_MDMINI,          // Megadrive Mini
#ifdef ENABLE_OUTPUT_JVS
  OUTPUT_JVS,             // JVS arcade I/O board (RS-485)
#endif
  OUTPUT_AUTO,            // Auto-detect host and switch output mode after enumeration
  OUTPUT_XINPUTW,         // Deprecated Windows XInput wireless receiver style; canonicalized to OUTPUT_XINPUT2P
#ifdef ENABLE_OUTPUT_ESP32_BT
  OUTPUT_ESP32_BT,        // ESP32 Bluetooth output (USB2RF)
#endif
#ifdef ENABLE_OUTPUT_CONSOLE
  OUTPUT_CONSOLE_NES,     // NES/Famicom output (USB2Classic)
  OUTPUT_CONSOLE_SNES,    // SNES/SFC output (USB2Classic)
  OUTPUT_CONSOLE_N64,     // N64 output (USB2Classic)
  OUTPUT_CONSOLE_GC,      // GameCube output (USB2Classic)
  OUTPUT_CONSOLE_SATURN,  // Saturn output (USB2Classic)
  OUTPUT_CONSOLE_GENESIS, // Genesis/Mega Drive output (USB2Classic)
  OUTPUT_CONSOLE_WII,     // Wii Classic Controller output (USB2Classic)
  OUTPUT_CONSOLE_AUTO,    // Auto-detect console via cable ADC (USB2Classic)
#endif
#ifdef ENABLE_OUTPUT_DB15
  OUTPUT_DB15_SUPERGUN,   // DB15 parallel supergun output (USB2DB15)
#endif
  OUTPUT_KEYBOARD,        // USB HID keyboard with arcade/MAME defaults
  // Guard: Windows/Linux XInput uses the wired two-slot XInput2P runtime. It is
  // intentionally auth-free and must not replace OUTPUT_XINPUT for Xbox 360.
  OUTPUT_XINPUT2P,
  OUTPUT_XBOXONE,         // Xbox One / XGIP with physical controller auth sidecar
  OUTPUT_LAST             // NOT USED
};

#ifndef ENABLE_OUTPUT_JVS
#define OUTPUT_JVS ((outputMode_t)0x80)
#endif
#ifndef ENABLE_OUTPUT_ESP32_BT
#define OUTPUT_ESP32_BT ((outputMode_t)0x81)
#endif
#ifndef ENABLE_OUTPUT_CONSOLE
#define OUTPUT_CONSOLE_NES ((outputMode_t)0x90)
#define OUTPUT_CONSOLE_SNES ((outputMode_t)0x91)
#define OUTPUT_CONSOLE_N64 ((outputMode_t)0x92)
#define OUTPUT_CONSOLE_GC ((outputMode_t)0x93)
#define OUTPUT_CONSOLE_SATURN ((outputMode_t)0x94)
#define OUTPUT_CONSOLE_GENESIS ((outputMode_t)0x95)
#define OUTPUT_CONSOLE_WII ((outputMode_t)0x96)
#define OUTPUT_CONSOLE_AUTO ((outputMode_t)0x97)
#endif
#ifndef ENABLE_OUTPUT_DB15
#define OUTPUT_DB15_SUPERGUN ((outputMode_t)0x98)
#endif

inline constexpr outputMode_t LAST_MISTER_MODE = OUTPUT_MISTER_GUNCON;
