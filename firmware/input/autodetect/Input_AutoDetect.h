// Auto-detect input mode
// Probes multiple protocols to determine what controller is connected
//
// Hotswap first checks strict PSX replies before the slower miss paths.
// Normal detection order:
// 1. Joybus (N64/GC/GBA) - uses PIO on pin 13, most reliable
// 2. PCE - checks the shared bus before SNES clocks can disturb a multitap
// 3. SNES/NES - shift register protocol, reliable detection via ID bits
// 4. Saturn/Megadrive - TH multiplexing
// 5. PSX - SPI protocol on pin 13 (only if Joybus not detected)
// 6. Dreamcast - Maple Bus on pins 08-09

#ifndef INPUT_AUTODETECT_H
#define INPUT_AUTODETECT_H

#include <stddef.h>
#include <stdint.h>

// Jaguar AUTO is assisted-only on this build. Shared Saturn/Jaguar wiring makes
// passive idle reads too ambiguous for unattended Jaguar classification; hold
// Jaguar Pause to request the assisted Jaguar path.

// Known broken: Saturn auto-detect can still be unstable on some connectors
// (false detect loops or mode with no active pad).

// 3DO hot-swap probing is noisy on shared Saturn/3DO lines.
// Keep disabled by default to avoid Saturn sessions flipping to 3DO.
#ifndef AUTODETECT_ENABLE_3DO_HOTSWAP
  #define AUTODETECT_ENABLE_3DO_HOTSWAP 0
#endif

// Forward declarations for button and buzzer polling during detection delays
// This ensures button events aren't missed and buzzer melodies progress during probes
class ButtonHandler;
class BootselButtonHandler;
class Buzzer;
extern ButtonHandler modeButton;
extern BootselButtonHandler resetButton;
extern Buzzer buzzer;

// Inline delay helper that polls buttons and buzzer
// Uses poll() instead of update() to preserve button events for the main loop
inline void autoDetectDelay(uint32_t ms) {
  uint32_t start = millis();
  while (millis() - start < ms) {
    modeButton.poll();
    resetButton.poll();
    buzzer.update();  // Keep buzzer melody progressing
    delay(1);
  }
}

// Keep probe diagnostics compiled into every auto-detect translation unit.
// Serial `AUTODETECT` uses this to show which protocol probe won and the raw
// bytes each candidate saw, which is essential for hardware-specific AUTO bugs.
#ifndef AUTODETECT_DEBUG
  #define AUTODETECT_DEBUG
#endif
#ifdef AUTODETECT_DEBUG
  enum : uint8_t {
    ADDBG_PROBE_INDEX_JOYBUS = 0,
    ADDBG_PROBE_INDEX_SNES,
    ADDBG_PROBE_INDEX_PCE,
    ADDBG_PROBE_INDEX_ATARI_DRIVING,
    ADDBG_PROBE_INDEX_SATURN,
    ADDBG_PROBE_INDEX_3DO,
    ADDBG_PROBE_INDEX_PSX,
    ADDBG_PROBE_INDEX_WII,
    ADDBG_PROBE_INDEX_DREAMCAST,
    ADDBG_PROBE_INDEX_NEOGEO,
    ADDBG_PROBE_INDEX_ATARI_PADDLE,
    ADDBG_PROBE_COUNT,
  };

  enum : uint16_t {
    ADDBG_PROBE_JOYBUS        = (1u << ADDBG_PROBE_INDEX_JOYBUS),
    ADDBG_PROBE_SNES          = (1u << ADDBG_PROBE_INDEX_SNES),
    ADDBG_PROBE_PCE           = (1u << ADDBG_PROBE_INDEX_PCE),
    ADDBG_PROBE_ATARI_DRIVING = (1u << ADDBG_PROBE_INDEX_ATARI_DRIVING),
    ADDBG_PROBE_ATARI         = ADDBG_PROBE_ATARI_DRIVING,
    ADDBG_PROBE_SATURN        = (1u << ADDBG_PROBE_INDEX_SATURN),
    ADDBG_PROBE_3DO           = (1u << ADDBG_PROBE_INDEX_3DO),
    ADDBG_PROBE_PSX           = (1u << ADDBG_PROBE_INDEX_PSX),
    ADDBG_PROBE_WII           = (1u << ADDBG_PROBE_INDEX_WII),
    ADDBG_PROBE_DREAMCAST     = (1u << ADDBG_PROBE_INDEX_DREAMCAST),
    ADDBG_PROBE_JAGUAR        = (1u << 12),
    ADDBG_PROBE_NEOGEO        = (1u << ADDBG_PROBE_INDEX_NEOGEO),
    ADDBG_PROBE_ATARI_PADDLE  = (1u << ADDBG_PROBE_INDEX_ATARI_PADDLE),
  };
#endif

#include "input_autodetect_debug_runtime_state.h"
#include "input_autodetect_hold_runtime_state.h"
#include "input_autodetect_passive_runtime_state.h"
#include "input_autodetect_runtime.h"
#include "input_autodetect_support.h"
#include "../wii/input_wii_shared.h"
#include "../base/RZInputModule.h"

// Include Joybus library for PIO-based detection
#ifdef ENABLE_INPUT_N64
  #include <JoybusLib/JoybusLib.h>
#elif defined(ENABLE_INPUT_GAMECUBE)
  #include <JoybusLib/JoybusLib.h>
#elif defined(ENABLE_INPUT_GBA)
  #include <JoybusLib/JoybusLib.h>
#endif

#ifdef ENABLE_INPUT_DREAMCAST
  #include <MapleLib/MapleLib.h>
#endif

#if defined(ENABLE_INPUT_SATURN) || defined(ENABLE_INPUT_MEGADRIVE)
  #include <SaturnLib/SaturnLib.h>
#endif

#ifdef ENABLE_INPUT_PCE
  #ifndef PCE_ENABLE_MULTITAP
    #define PCE_ENABLE_MULTITAP
  #endif
  #include <PceLib/PceLib.h>
#endif

// Pin definitions for detection probes
typedef struct {
  // SNES pins
  uint8_t snes_clk;
  uint8_t snes_lat;
  uint8_t snes_dat;
  // Saturn pins (D0-D3, TH, TR, TL)
  uint8_t sat_d0;
  uint8_t sat_d1;
  uint8_t sat_d2;
  uint8_t sat_d3;
  uint8_t sat_th;
  uint8_t sat_tr;
  uint8_t sat_tl;
  // Joybus/PSX pin (pin 13)
  uint8_t joybus_dat;
  // PIO resources for Joybus
  PIO joybus_pio;
  uint8_t joybus_sm;
  // PSX pins (SPI)
  uint8_t psx_att;   // pin 13 (same as joybus)
  uint8_t psx_cmd;   // pin 11
  uint8_t psx_dat;   // pin 12
  uint8_t psx_clk;   // pin 10
  uint8_t psx_ack;   // pin 08
  // Dreamcast Maple pins
  uint8_t dc_a;      // pin 08 (SDCKA)
  uint8_t dc_b;      // pin 09 (SDCKB)
  // 3DO pins
  uint8_t tdo_clk;   // pin 06
  uint8_t tdo_out;   // pin 05
  uint8_t tdo_in;    // pin 07
  // Wii I2C pins
  uint8_t wii_sda;   // pin 02
  uint8_t wii_scl;   // pin 01
  uint8_t wii_alt_sda;
  uint8_t wii_alt_scl;
  // Jaguar pins (row outputs)
  uint8_t jag_j0;    // pin 04
  uint8_t jag_j1;    // pin 03
  uint8_t jag_j2;    // pin 08
  uint8_t jag_j3;    // pin 07
  // Jaguar pins (column inputs)
  uint8_t jag_b0;    // pin 05
  uint8_t jag_b1;    // pin 06
  uint8_t jag_j8;    // pin 12
  uint8_t jag_j9;    // pin 09
  uint8_t jag_j10;   // pin 02
  uint8_t jag_j11;   // pin 01
  // Neo Geo pins (direct button matrix)
  uint8_t neo_up;    // pin 10
  uint8_t neo_down;  // pin 05
  uint8_t neo_left;  // pin 07
  uint8_t neo_right; // pin 04
  uint8_t neo_a;     // pin 11
  uint8_t neo_b;     // pin 03
  uint8_t neo_c;     // pin 06
  uint8_t neo_d;     // pin 02
  uint8_t neo_sel;   // pin 01
  uint8_t neo_start; // pin 12
  // Atari Driving pins (encoder + button)
  uint8_t drv_enc_a; // pin 01 (encoder A)
  uint8_t drv_enc_b; // pin 02 (encoder B)
  uint8_t drv_btn;   // pin 05 (fire button, active low)
  // Atari Paddle pins (pots + buttons) - Port 2 only due to ADC
  uint8_t pdl_pot_a; // pin 03 (pot A, ADC)
  uint8_t pdl_pot_b; // pin 04 (pot B, ADC)
  uint8_t pdl_btn_a; // pin 01 (button A)
  uint8_t pdl_btn_b; // pin 02 (button B)
  // SMS/JPC pins (DE9 connector, same as Saturn but different protocol)
  uint8_t sms_up;    // pin 01 (D0)
  uint8_t sms_down;  // pin 02 (D1)
  uint8_t sms_left;  // pin 03 (D2)
  uint8_t sms_right; // pin 04 (D3)
  uint8_t sms_tl;    // pin 06 (button 1)
  uint8_t sms_th;    // pin 07 (button 2 for JPC)
  uint8_t sms_tr;    // pin 09 (button 2 for SMS)
} autodetect_pins_t;

constexpr uint8_t kAutoDetectPortCount = 2;
extern const autodetect_pins_t autodetect_pins[kAutoDetectPortCount];

class AutoDetector {
public:
  // Reset shared pins to inputs with pull-ups between probes
  static void resetSharedPins(const autodetect_pins_t& pins, bool is_hotswap = false);

  static AutoDetectResult detectPort(uint8_t port, bool is_hotswap = false);
  static AutoDetectResult detectPortPsxOnly(uint8_t port, bool is_hotswap = false);
  static AutoDetectResult detectPortDreamcastOnly(uint8_t port, bool is_hotswap = false);
  static AutoDetectResult detectPortShiftRegisterOnly(uint8_t port, bool is_hotswap = false);

private:
  #if defined(ENABLE_INPUT_N64) || defined(ENABLE_INPUT_GAMECUBE) || defined(ENABLE_INPUT_GBA)
  // Probe Joybus protocol (N64/GameCube/GBA) using PIO
  // Controller type IDs (from JoybusLib.h):
  //   0x0500 = N64 controller
  //   0x0001 = N64 mic
  //   0x0002 = N64 keyboard
  //   0x0200 = N64 mouse
  //   0x0900 = GameCube controller
  //   0x0800 = GC wheel
  //   0x0820 = GC keyboard
  //   0x8900+ = GC wireless variants
  //   0x0004 = GBA
  static AutoDetectResult probeJoybus(const autodetect_pins_t& pins, uint8_t port, bool is_hotswap);
  #endif

  // Probe SNES/NES/VirtualBoy protocol
  // Uses shift register: pulse LATCH, clock out 16 bits, check ID in bits 12-15
  static AutoDetectResult probeSNES(const autodetect_pins_t& pins);
  static AutoDetectResult probeSNES(const autodetect_pins_t& pins, uint8_t debug_port);

  // Probe PC-Engine protocol
  // PCE controller returns 0x0 on D0-D3 when SEL=HIGH and CLR=HIGH
  // Pin mapping: SEL=sat_tl (pin 05), CLR=sat_th (pin 06), D0-D3=sat_d0-d3
  static AutoDetectResult probePCE(const autodetect_pins_t& pins);
  static AutoDetectResult probePCE(const autodetect_pins_t& pins, uint8_t debug_port);

  #ifdef ENABLE_INPUT_PCE
  static bool confirmPCEViaPceLib(const autodetect_pins_t& pins, bool is_hotswap,
                                  uint8_t debug_port = 0xFF, bool allow_pad_confirm = true,
                                  bool* saw_multitap = nullptr);
  static bool confirmPCEViaRawProbe(const autodetect_pins_t& pins, bool is_hotswap);
  #endif

  #if defined(ENABLE_INPUT_SATURN) || defined(ENABLE_INPUT_MEGADRIVE)
  static bool confirmSaturnViaSaturnLib(const autodetect_pins_t& pins,
                                        uint8_t port,
                                        bool is_hotswap);
  #endif

  // Probe Saturn/Megadrive protocol
  // Uses TH line to multiplex data on D0-D3
  // Detection based on SaturnLib protocol analysis
  static AutoDetectResult probeSaturn(const autodetect_pins_t& pins, uint8_t port, bool* sawBusActivity = nullptr, bool is_hotswap = false);

  // Probe 3DO protocol
  // 3DO uses serial shift register: clock out 16 bits, check for ID pattern
  // Digital pad: (data & 0xC007) == 0x1
  #ifdef ENABLE_INPUT_3DO
  static AutoDetectResult probe3DO(const autodetect_pins_t& pins, bool is_hotswap);
  #endif

  // Probe PSX protocol using bit-bang SPI
  // PSX responds to the 0x01, 0x42, 0x00, 0xFF, 0xFF poll with type and sync.
  #ifdef ENABLE_INPUT_PSX
  static bool isKnownPsxReplyType(uint8_t controllerType);
  static bool isLegacyPsxReplyFamily(uint8_t controllerType);
  static AutoDetectResult probePSX(const autodetect_pins_t& pins, uint8_t port);
  static bool psxWaitForAck(const autodetect_pins_t& pins, uint16_t timeoutUs);
  static uint8_t psxTransferByte(const autodetect_pins_t& pins, uint8_t txByte);
  #endif

  // Probe Wii extension controller using I2C
  // Wii extensions respond at I2C address 0x52
  #ifdef ENABLE_INPUT_WII
  static AutoDetectResult probeWii(const autodetect_pins_t& pins, uint8_t port);
  #endif

  #ifdef ENABLE_INPUT_DREAMCAST
  // Probe Dreamcast Maple Bus on pins 08-09.
  // Uses a short MaplePort session and releases all resources before returning.
  static AutoDetectResult probeDreamcast(const autodetect_pins_t& pins, bool is_hotswap);
  #endif

  // Probe Neo Geo controller (direct button matrix)
  // Neo Geo uses pins 1-7, 10-12 for direct button reads
  // Detection: only trust Neo-Geo-unique pins (10/11/12) when they are
  // actively pulled LOW. Avoid broad "multiple LOW pins" heuristics since
  // they cause false positives on JPC/FM Towns wiring.
  #ifdef ENABLE_INPUT_NEOGEO
  static AutoDetectResult probeNeoGeo(const autodetect_pins_t& pins);
  #endif

  // Probe Atari Paddle (analog + fake-ADC timing detection)
  // Paddle uses ADC-capable pins, but no-cap builds can still be detected via
  // RC timing to digital threshold.
  // Port 2 only has ADC capability (GPIO 28, 29)
  #ifdef ENABLE_INPUT_PADDLE
  static AutoDetectResult probeAtariPaddle(const autodetect_pins_t& pins, uint8_t port);
  #endif

  // Probe Atari Driving Controller (encoder detection)
  // Driving controller uses rotary encoder on pins 1-2
  // Detection: Check for encoder state changes over short period
  #ifdef ENABLE_INPUT_DRIVING
  static AutoDetectResult probeAtariDriving(const autodetect_pins_t& pins, bool is_hotswap = false);
  #endif
};

#ifdef AUTODETECT_DEBUG
// Debug formatting helpers live in input_autodetect_support.cpp now.
#endif

#endif // INPUT_AUTODETECT_H
