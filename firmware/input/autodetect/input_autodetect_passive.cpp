#include "../../product_config.h"
#include "../../firmware_platform_config.h"

#include <Arduino.h>

#include "../../platform/button_handler.h"
#include "../../platform/buzzer.h"
#include "../../platform/display_runtime_state.h"

#include "input_autodetect_support.h"
#include "input_autodetect_passive_internal.h"

#ifdef ENABLE_INPUT_AUTODETECT
namespace {

void restoreSharedInputLinesForMode(const autodetect_pins_t& pins, DeviceEnum mode) {
  #if defined(ENABLE_INPUT_SNES) || defined(ENABLE_INPUT_NES) || defined(ENABLE_INPUT_VBOY)
  if (mode == RZORD_SNES || mode == RZORD_NES || mode == RZORD_VBOY) {
    pinMode(pins.snes_clk, OUTPUT);
    digitalWrite(pins.snes_clk, HIGH);
    pinMode(pins.snes_lat, OUTPUT);
    digitalWrite(pins.snes_lat, LOW);
    pinMode(pins.snes_dat, INPUT_PULLUP);
  }
  #endif

  #if defined(ENABLE_INPUT_SATURN) || defined(ENABLE_INPUT_MEGADRIVE)
  if (mode == RZORD_SATURN || mode == RZORD_MEGADRIVE) {
    pinMode(pins.sat_d0, INPUT_PULLUP);
    pinMode(pins.sat_d1, INPUT_PULLUP);
    pinMode(pins.sat_d2, INPUT_PULLUP);
    pinMode(pins.sat_d3, INPUT_PULLUP);
    pinMode(pins.sat_th, OUTPUT);
    digitalWrite(pins.sat_th, HIGH);
    pinMode(pins.sat_tr, INPUT_PULLUP);
    pinMode(pins.sat_tl, INPUT_PULLUP);
  }
  #endif

  #if defined(ENABLE_INPUT_PCE)
  if (mode == RZORD_PCE) {
    pinMode(pins.sat_d0, INPUT_PULLUP);
    pinMode(pins.sat_d1, INPUT_PULLUP);
    pinMode(pins.sat_d2, INPUT_PULLUP);
    pinMode(pins.sat_d3, INPUT_PULLUP);
    pinMode(pins.sat_tl, OUTPUT);
    digitalWrite(pins.sat_tl, HIGH);
    pinMode(pins.sat_th, OUTPUT);
    digitalWrite(pins.sat_th, LOW);
  }
  #endif
}

AutoDetectResult probePassiveAssistedRoute(uint8_t port) {
  #ifdef ENABLE_INPUT_JAGUAR
  AutoDetectResult jaguarResult = input_autodetect_passive_internal::probeJaguarAssistHeldInternal(port);
  if (jaguarResult != AUTODETECT_NONE) {
    return jaguarResult;
  }
  #endif
  AutoDetectResult result = input_autodetect_passive_internal::probePassiveOnlyAssistedRoute(port);
  if (result != AUTODETECT_NONE) {
    return result;
  }
  return AUTODETECT_NONE;
}

bool assistedHoldRoutePresent(uint8_t port) {
  #ifdef ENABLE_INPUT_JAGUAR
  if (input_autodetect_passive_internal::jaguarAssistHoldPresent(port)) {
    return true;
  }
  #endif
  return input_autodetect_passive_internal::passiveHoldRoutePresent(port);
}

PassiveAssistScanResult scanPassiveAssistedRoutesInternal() {
  PassiveAssistScanResult scan = { AUTODETECT_NONE, 0xFF, false };
  for (uint8_t p = 0; p < kAutoDetectPortCount; ++p) {
    scan.result = probePassiveAssistedRoute(p);
    if (scan.result != AUTODETECT_NONE) {
      scan.port = p;
      break;
    }
    if (assistedHoldRoutePresent(p)) {
      scan.assist_candidate_pending = true;
    }
  }
  return scan;
}

PassiveAssistScanResult scanPassiveOnlyRoutesInternal() {
  PassiveAssistScanResult scan = { AUTODETECT_NONE, 0xFF, false };
  for (uint8_t p = 0; p < kAutoDetectPortCount; ++p) {
    scan.result = input_autodetect_passive_internal::probePassiveOnlyAssistedRoute(p);
    if (scan.result != AUTODETECT_NONE) {
      scan.port = p;
      break;
    }
  }
  return scan;
}

}  // namespace

PassiveAssistScanResult runPassiveAssistedAutoDetectScan() {
  return scanPassiveAssistedRoutesInternal();
}

PassiveAssistScanResult runPassiveOnlyAutoDetectScan() {
  return scanPassiveOnlyRoutesInternal();
}

AutoDetectResult detectJaguarAssistHold(uint8_t port) {
  #ifdef ENABLE_INPUT_JAGUAR
  // This is the only Jaguar AUTO path on the retro build.
  return input_autodetect_passive_internal::probeJaguarAssistHeldInternal(port);
  #else
  (void)port;
  return AUTODETECT_NONE;
  #endif
}

void resetAutoDetectPins() {
  const uint8_t hdmi_pins[] = {
    HDMI_1_01, HDMI_1_02, HDMI_1_03, HDMI_1_04, HDMI_1_05, HDMI_1_06, HDMI_1_07,
    HDMI_1_08, HDMI_1_09, HDMI_1_10, HDMI_1_11, HDMI_1_12, HDMI_1_13,
    HDMI_2_01, HDMI_2_02, HDMI_2_03, HDMI_2_04, HDMI_2_05, HDMI_2_06, HDMI_2_07,
    HDMI_2_08, HDMI_2_09, HDMI_2_10, HDMI_2_11, HDMI_2_12, HDMI_2_13
  };
  for (uint8_t i = 0; i < sizeof(hdmi_pins); ++i) {
    gpio_init(hdmi_pins[i]);
    gpio_set_dir(hdmi_pins[i], GPIO_IN);
    gpio_pull_up(hdmi_pins[i]);
  }
}

void restoreAutoDetectSharedInputLines(DeviceEnum mode) {
  for (uint8_t port = 0; port < kAutoDetectPortCount; ++port) {
    restoreSharedInputLinesForMode(autodetect_pins[port], mode);
  }
}
#endif
