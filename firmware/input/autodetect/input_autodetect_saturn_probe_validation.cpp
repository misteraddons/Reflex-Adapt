#include "../../product_config.h"
#include "../../firmware_platform_config.h"

#include <Arduino.h>

#include "../../platform/button_handler.h"
#include "../../platform/buzzer.h"
#include "Input_AutoDetect.h"
#include "input_autodetect_saturn_probe_internal.h"

#ifdef ENABLE_INPUT_AUTODETECT
void confirmSaturnViaLibraries(const autodetect_pins_t& pins, uint8_t port, bool is_hotswap,
                               SaturnProbeState& state) {
  #if defined(ENABLE_INPUT_SATURN) || defined(ENABLE_INPUT_MEGADRIVE)
  auto detect_saturn_multitap_only = [&]() -> bool {
    SaturnPort sat_probe(
      pins.sat_d0, pins.sat_d1, pins.sat_d2, pins.sat_d3,
      pins.sat_th, pins.sat_tr, pins.sat_tl,
      SatLibConfig_saturn_only
    );
    sat_probe.begin();
    sat_probe.detectMultitap();
    const uint8_t tap_ports = sat_probe.getMultitapPorts();

    #ifdef AUTODETECT_DEBUG
    if (port < kAutoDetectDebugPortCount) {
      lastDebug[port].satlib_tap_ports = tap_ports;
    }
    #endif

    if (tap_ports == TAP_SAT_PORTS) {
      state.multitap_detected = true;
      return true;
    }
    return false;
  };

  if (state.saturn_hits != 0 || state.megadrive_hits != 0) {
    detect_saturn_multitap_only();
    return;
  }
  if (is_hotswap && state.saturn_hits == 0 && state.megadrive_hits == 0 && state.activity_hits == 0) {
    return;
  }

  uint8_t satlib_total_passes = 0;
  auto run_satlib_phase = [&](SatLibConfig_t cfg, bool probe_multitap) -> bool {
    SaturnPort sat_probe(
      pins.sat_d0, pins.sat_d1, pins.sat_d2, pins.sat_d3,
      pins.sat_th, pins.sat_tr, pins.sat_tl,
      cfg
    );
    sat_probe.begin();
    if (probe_multitap) {
      sat_probe.detectMultitap();
    }
    const uint8_t tap_ports = sat_probe.getMultitapPorts();

    #ifdef AUTODETECT_DEBUG
    if (port < kAutoDetectDebugPortCount) {
      lastDebug[port].satlib_tap_ports = tap_ports;
    }
    #endif

    if (tap_ports == TAP_SAT_PORTS) {
      state.multitap_detected = true;
      return true;
    }

    autoDetectDelay(is_hotswap ? 70 : 110);

    const uint8_t passes = is_hotswap ? 18 : 24;
    for (uint8_t pass = 0; pass < passes; ++pass) {
      sat_probe.update();
      satlib_total_passes++;

      uint8_t sat_count = sat_probe.getControllerCount();
      uint8_t sat_type = 0;
      bool found_sat = false;
      bool found_mega = false;

      for (uint8_t i = 0; i < sat_count; ++i) {
        SatDeviceType_Enum dt = sat_probe.getSaturnController(i).deviceType();
        sat_type = (uint8_t)dt;
        if (dt == SAT_DEVICE_MEGA3 || dt == SAT_DEVICE_MEGA6) {
          found_mega = true;
        } else if (dt != SAT_DEVICE_NONE && dt != SAT_DEVICE_NOTSUPPORTED) {
          found_sat = true;
          break;
        }
      }

      #ifdef AUTODETECT_DEBUG
      if (port < kAutoDetectDebugPortCount) {
        lastDebug[port].satlib_passes = satlib_total_passes;
        lastDebug[port].satlib_last_count = sat_count;
        lastDebug[port].satlib_last_type = sat_type;
      }
      #endif

      if (found_sat) {
        state.saturn_hits = 1;
        state.activity_hits = 1;
        state.satlib_confirmed_saturn = true;
        #ifdef AUTODETECT_DEBUG
        if (port < kAutoDetectDebugPortCount) {
          lastDebug[port].satlib_hits++;
        }
        #endif
        return true;
      }

      if (found_mega) {
        state.megadrive_hits = 1;
        state.activity_hits = 1;
        return true;
      }

      autoDetectDelay(is_hotswap ? 6 : 4);
    }
    return false;
  };

  bool matched = run_satlib_phase(SatLibConfig_saturn_only, true);
  if (!matched && state.saturn_hits == 0 && state.megadrive_hits == 0) {
    run_satlib_phase(SatLibConfig_default, true);
  }
  #else
  (void)pins;
  (void)port;
  (void)is_hotswap;
  (void)state;
  #endif
}

bool passiveTlOnlyLow(const SaturnProbeState& state) {
  return state.stable_nibble_0 && state.stable_nibble_1 &&
         state.stable_tl_0 && state.stable_tl_1 &&
         state.stable_tr_0 && state.stable_tr_1 &&
         state.first_nibble_0 == 0x0F && state.first_nibble_1 == 0x0F &&
         state.last_nibble_0 == 0x0F && state.last_nibble_1 == 0x0F &&
         state.first_tl_0 == 0 && state.first_tl_1 == 0 &&
         state.last_tl_0 == 0 && state.last_tl_1 == 0 &&
         state.first_tr_0 == 1 && state.first_tr_1 == 1 &&
         state.last_tr_0 == 1 && state.last_tr_1 == 1;
}

bool passivePin12DirectSignature(const SaturnProbeState& state) {
  return state.stable_tl_0 && state.stable_tl_1 &&
         state.stable_tr_0 && state.stable_tr_1 &&
         state.first_tr_0 == 1 && state.first_tr_1 == 1 &&
         state.last_tr_0 == 1 && state.last_tr_1 == 1 &&
         state.first_tl_0 == state.first_tl_1 &&
         state.last_tl_0 == state.last_tl_1 &&
         state.first_nibble_0 == state.first_nibble_1 &&
         state.last_nibble_0 == state.last_nibble_1 &&
         (state.first_nibble_0 & 0x0C) == 0x0C &&
         (state.last_nibble_0 & 0x0C) == 0x0C &&
         (state.first_nibble_0 != 0x0F || state.first_tl_0 == 0 ||
          state.last_nibble_0 != 0x0F || state.last_tl_0 == 0);
}

void applySaturnInvertedFallback(const autodetect_pins_t& pins, SaturnProbeState& state) {
  if (state.saturn_hits == 0 && state.megadrive_hits == 0 && state.activity_hits > 0 &&
      state.satlib_confirmed_saturn &&
      state.stable_nibble_0 && state.stable_nibble_1 &&
      state.stable_tl_0 && state.stable_tr_0 && state.stable_tl_1 && state.stable_tr_1 &&
      state.first_tl_0 == 1 && state.first_tr_0 == 1 && state.first_tl_1 == 1 && state.first_tr_1 == 1) {
    uint8_t inv0 = (~state.first_nibble_0) & 0x0F;
    uint8_t inv1 = (~state.first_nibble_1) & 0x0F;
    bool inv_threewire_ok = false;

    if (inv0 == 0x01 && inv1 == 0x01) {
      gpio_set_function(pins.sat_th, GPIO_FUNC_SIO);
      gpio_set_function(pins.sat_tr, GPIO_FUNC_SIO);
      gpio_set_function(pins.sat_tl, GPIO_FUNC_SIO);
      gpio_init(pins.sat_th);
      gpio_init(pins.sat_tr);
      gpio_init(pins.sat_tl);
      gpio_set_dir(pins.sat_th, GPIO_OUT);
      gpio_set_dir(pins.sat_tr, GPIO_OUT);
      gpio_set_dir(pins.sat_tl, GPIO_IN);
      gpio_pull_up(pins.sat_tl);
      gpio_put(pins.sat_th, 1);
      delayMicroseconds(6);

      auto sample_tl = [&](uint8_t tr_val) -> uint8_t {
        gpio_put(pins.sat_tr, tr_val);
        delayMicroseconds(10);
        return gpio_get(pins.sat_tl);
      };

      uint8_t tl0a = sample_tl(0);
      uint8_t tl1a = sample_tl(1);
      uint8_t tl0b = sample_tl(0);
      uint8_t tl1b = sample_tl(1);

      bool follows = (tl0a == 0 && tl1a == 1 && tl0b == 0 && tl1b == 1);
      bool inverts = (tl0a == 1 && tl1a == 0 && tl0b == 1 && tl1b == 0);
      inv_threewire_ok = follows || inverts;

      gpio_put(pins.sat_tr, 1);
      gpio_set_dir(pins.sat_tr, GPIO_IN);
      gpio_pull_up(pins.sat_tr);
      gpio_set_dir(pins.sat_th, GPIO_IN);
      gpio_pull_up(pins.sat_th);
    }

    if (((inv0 & 0x07) == 0x04) || (inv0 == 0x01 && inv1 == 0x01 && inv_threewire_ok)) {
      state.saturn_hits = 1;
      #ifdef AUTODETECT_DEBUG
      state.saturn_inv_fallback = true;
      #endif
    }
  }
}

void applySaturnSatlibStableFallback(SaturnProbeState& state) {
  if (state.saturn_hits == 0 && state.megadrive_hits == 0 &&
      state.satlib_confirmed_saturn &&
      state.stable_nibble_0 && state.stable_nibble_1 &&
      state.stable_tl_0 && state.stable_tr_0 && state.stable_tl_1 && state.stable_tr_1 &&
      state.first_tl_0 == 1 && state.first_tr_0 == 1 && state.first_tl_1 == 1 && state.first_tr_1 == 1 &&
      state.first_nibble_0 == 0x0E &&
      (state.first_nibble_1 == 0x0F || state.first_nibble_1 == 0x0E) &&
      state.last_nibble_0 == state.first_nibble_0 &&
      state.last_nibble_1 == state.first_nibble_1 &&
      state.activity_hits >= (kSaturnProbeSampleCount - 1)) {
    state.saturn_hits = 1;
    #ifdef AUTODETECT_DEBUG
    state.saturn_inv_fallback = true;
    #endif
  }
}
#endif
