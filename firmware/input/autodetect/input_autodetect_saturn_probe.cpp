#include "../../product_config.h"
#include "../../firmware_platform_config.h"

#include <Arduino.h>

#include "../../platform/button_handler.h"
#include "../../platform/buzzer.h"
#include "Input_AutoDetect.h"
#include "input_autodetect_saturn_probe_internal.h"

#ifdef ENABLE_INPUT_AUTODETECT
AutoDetectResult AutoDetector::probeSaturn(const autodetect_pins_t& pins, uint8_t port, bool* sawBusActivity, bool is_hotswap) {
  const uint8_t sat_settle_us = 10;
  SaturnProbeState state;

  initializeSaturnProbeBus(pins, is_hotswap);
  captureSaturnProbeSamples(pins, sat_settle_us, state);
  retrySaturnQuietWindow(pins, sat_settle_us, state);
  retrySaturnToggleWindow(pins, sat_settle_us, state);

  if (passiveTlOnlyLow(state)) {
    if (sawBusActivity != nullptr) {
      *sawBusActivity = false;
    }
    return AUTODETECT_NONE;
  }

  if (passivePin12DirectSignature(state)) {
    if (sawBusActivity != nullptr) {
      *sawBusActivity = false;
    }
    return AUTODETECT_NONE;
  }

  confirmSaturnViaLibraries(pins, port, is_hotswap, state);
  applySaturnInvertedFallback(pins, state);
  applySaturnSatlibStableFallback(state);

  bool bus_active = saturnBusActive(state);
  if (sawBusActivity != nullptr) {
    *sawBusActivity = bus_active;
  }

  #ifdef AUTODETECT_DEBUG
  if (port < kAutoDetectDebugPortCount) {
    lastDebug[port].saturn_hits = state.saturn_hits;
    lastDebug[port].megadrive_hits = state.megadrive_hits;
    lastDebug[port].saturn_activity_hits = state.activity_hits;
    lastDebug[port].saturn_bus_active = bus_active;
    lastDebug[port].sat_nibble0_first = state.first_nibble_0;
    lastDebug[port].sat_nibble1_first = state.first_nibble_1;
    lastDebug[port].sat_nibble0_last = state.last_nibble_0;
    lastDebug[port].sat_nibble1_last = state.last_nibble_1;
    lastDebug[port].sat_tl0_first = state.first_tl_0;
    lastDebug[port].sat_tr0_first = state.first_tr_0;
    lastDebug[port].sat_tl1_first = state.first_tl_1;
    lastDebug[port].sat_tr1_first = state.first_tr_1;
    lastDebug[port].sat_tl0_last = state.last_tl_0;
    lastDebug[port].sat_tr0_last = state.last_tr_0;
    lastDebug[port].sat_tl1_last = state.last_tl_1;
    lastDebug[port].sat_tr1_last = state.last_tr_1;
    lastDebug[port].saturn_inv_fallback = state.saturn_inv_fallback;
  }
  #endif

  if (state.multitap_detected) {
    return AUTODETECT_NONE;
  }

  if (state.saturn_hits > 0) {
    return AUTODETECT_SATURN;
  }

  if (state.megadrive_hits >= 3) {
    return AUTODETECT_MEGADRIVE;
  }

  if (state.activity_hits > 0 && state.megadrive_hits > 0) {
    return AUTODETECT_SATURN;
  }

  if (state.megadrive_hits > 0) {
    return AUTODETECT_MEGADRIVE;
  }

  return AUTODETECT_NONE;
}

#endif
