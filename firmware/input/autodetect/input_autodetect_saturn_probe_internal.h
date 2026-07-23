#pragma once

#include "Input_AutoDetect.h"

#ifdef ENABLE_INPUT_AUTODETECT
struct SaturnProbeState {
  uint8_t saturn_hits = 0;
  uint8_t megadrive_hits = 0;
  uint8_t activity_hits = 0;
  uint8_t first_nibble_0 = 0x0F;
  uint8_t first_nibble_1 = 0x0F;
  uint8_t first_tl_0 = 1;
  uint8_t first_tr_0 = 1;
  uint8_t first_tl_1 = 1;
  uint8_t first_tr_1 = 1;
  uint8_t last_nibble_0 = 0x0F;
  uint8_t last_nibble_1 = 0x0F;
  uint8_t last_tl_0 = 1;
  uint8_t last_tr_0 = 1;
  uint8_t last_tl_1 = 1;
  uint8_t last_tr_1 = 1;
  bool stable_nibble_0 = true;
  bool stable_nibble_1 = true;
  bool stable_tl_0 = true;
  bool stable_tr_0 = true;
  bool stable_tl_1 = true;
  bool stable_tr_1 = true;
  bool satlib_confirmed_saturn = false;
  bool multitap_detected = false;
#ifdef AUTODETECT_DEBUG
  bool saturn_inv_fallback = false;
#endif
};

inline constexpr uint8_t kSaturnProbeSampleCount = 6;

void initializeSaturnProbeBus(const autodetect_pins_t& pins, bool is_hotswap);
void captureSaturnProbeSamples(const autodetect_pins_t& pins, uint8_t sat_settle_us,
                               SaturnProbeState& state);
void retrySaturnQuietWindow(const autodetect_pins_t& pins, uint8_t sat_settle_us,
                            SaturnProbeState& state);
void retrySaturnToggleWindow(const autodetect_pins_t& pins, uint8_t sat_settle_us,
                             SaturnProbeState& state);
void confirmSaturnViaLibraries(const autodetect_pins_t& pins, uint8_t port, bool is_hotswap,
                               SaturnProbeState& state);
bool passiveTlOnlyLow(const SaturnProbeState& state);
bool passivePin12DirectSignature(const SaturnProbeState& state);
void applySaturnInvertedFallback(const autodetect_pins_t& pins, SaturnProbeState& state);
void applySaturnSatlibStableFallback(SaturnProbeState& state);
inline bool saturnBusActive(const SaturnProbeState& state) {
  return state.activity_hits > 0;
}
#endif
