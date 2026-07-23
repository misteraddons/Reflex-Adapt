#pragma once

#include <stdint.h>

#ifdef AUTODETECT_DEBUG
constexpr uint8_t kAutoDetectDebugPortCount = 2;

struct AutoDetectDebug {
  uint16_t joybus_type;      // Joybus device type (0 = none/timeout)
  uint8_t psx_recv1;         // PSX first response byte
  uint8_t psx_type;          // PSX controller type byte
  uint8_t psx_sync;          // PSX poll sync byte, normally 0x5A
  uint8_t psx_multitap;      // 0 = none, 1 = probed, 2 = bare multitap reply
  uint16_t probes_run;       // Bitmask of probes that ran
  uint8_t final_result;      // Final AutoDetectResult value for this port
  uint16_t probe_ms[ADDBG_PROBE_COUNT];

  uint16_t snes_data;
  uint16_t snes_extended;
  uint8_t snes_id;
  uint8_t snes_transitions;
  uint8_t snes_result;
  uint8_t snes_hits;
  uint8_t nes_hits;
  uint8_t vboy_hits;

  uint8_t pce_nibble0;
  uint8_t pce_nibble_dpad;
  uint8_t pce_nibble_btn;
  bool pce_raw_candidate;
  bool pce_lib_confirmed;
  uint8_t pce_tap_ports;
  uint8_t pce_last_count;
  uint8_t pce_last_type;

  uint8_t saturn_hits;
  uint8_t megadrive_hits;
  uint8_t saturn_activity_hits;
  bool saturn_bus_active;
  uint8_t sat_nibble0_first;
  uint8_t sat_nibble1_first;
  uint8_t sat_nibble0_last;
  uint8_t sat_nibble1_last;
  uint8_t sat_tl0_first;
  uint8_t sat_tr0_first;
  uint8_t sat_tl1_first;
  uint8_t sat_tr1_first;
  uint8_t sat_tl0_last;
  uint8_t sat_tr0_last;
  uint8_t sat_tl1_last;
  uint8_t sat_tr1_last;
  bool saturn_inv_fallback;
  uint8_t satlib_passes;
  uint8_t satlib_hits;
  uint8_t satlib_tap_ports;
  uint8_t satlib_last_count;
  uint8_t satlib_last_type;

  uint8_t jag_a[4];
  uint8_t jag_b[4];
  bool jag_c2c3_a;
  bool jag_c2c3_b;
  bool jag_non_idle_a;
  bool jag_non_idle_b;
  bool jag_similar;
  bool jag_detected;
  bool jag_skipped_due_saturn_activity;
  uint8_t jag_diff_bits;

  bool wii_ack52_a;
  bool wii_ack52_b;
  bool wii_ack51;
  bool wii_ack53;
  bool wii_sig_ok;
  bool wii_detected;
  uint8_t wii_type;
  uint8_t wii_id2;
  uint8_t wii_id3;
};

extern AutoDetectDebug lastDebug[kAutoDetectDebugPortCount];
#endif
