#include "../../product_config.h"
#include "../../firmware_platform_config.h"

#include <Arduino.h>

#include "../../core/firmware_support.h"
#include "../../platform/button_handler.h"
#include "../../platform/buzzer.h"
#include "Input_AutoDetect.h"

#ifdef ENABLE_INPUT_AUTODETECT
namespace {

constexpr uint8_t kSnesProbePasses = 3;

struct SnesProbeSample {
  uint16_t data;
  uint16_t extended;
  uint8_t transitions;
  uint8_t id;
  AutoDetectResult result;
};

uint8_t reverseSnesId(uint8_t rawId) {
  return (uint8_t)(((rawId & 1) << 3) | ((rawId & 2) << 1) |
                   ((rawId & 4) >> 1) | ((rawId & 8) >> 3));
}

SnesProbeSample readSnesProbeSample(const autodetect_pins_t& pins) {
  gpio_init(pins.snes_clk);
  gpio_init(pins.snes_lat);
  gpio_init(pins.snes_dat);

  gpio_set_dir(pins.snes_clk, GPIO_OUT);
  gpio_set_dir(pins.snes_lat, GPIO_OUT);
  gpio_set_dir(pins.snes_dat, GPIO_IN);
  gpio_pull_up(pins.snes_dat);

  gpio_put(pins.snes_clk, 0);
  gpio_put(pins.snes_lat, 0);
  delayMicroseconds(12);

  gpio_put(pins.snes_lat, 1);
  delayMicroseconds(12);
  gpio_put(pins.snes_lat, 0);
  delayMicroseconds(6);

  uint16_t data = 0;
  uint8_t transitions = 0;
  bool lastRaw = gpio_get(pins.snes_dat);
  data = lastRaw ? 0 : 1;

  for (int i = 1; i < 16; i++) {
    gpio_put(pins.snes_clk, 1);
    delayMicroseconds(6);
    gpio_put(pins.snes_clk, 0);
    delayMicroseconds(6);

    bool raw = gpio_get(pins.snes_dat);
    if (raw != lastRaw) {
      transitions++;
      lastRaw = raw;
    }
    bool bit = !raw;
    data = (data << 1) | (bit ? 1 : 0);
  }

  uint16_t extended = 0;
  for (int i = 0; i < 16; i++) {
    gpio_put(pins.snes_clk, 1);
    delayMicroseconds(6);
    gpio_put(pins.snes_clk, 0);
    delayMicroseconds(6);

    bool raw = gpio_get(pins.snes_dat);
    if (raw != lastRaw) {
      transitions++;
      lastRaw = raw;
    }
    bool bit = !raw;
    extended = (extended << 1) | (bit ? 1 : 0);
  }

  gpio_set_dir(pins.snes_clk, GPIO_IN);
  gpio_set_dir(pins.snes_lat, GPIO_IN);

  uint8_t rawId = data & 0x0F;
  uint8_t id = reverseSnesId(rawId);

  return {data, extended, transitions, id, AUTODETECT_NONE};
}

AutoDetectResult classifySnesProbeSample(SnesProbeSample& sample) {
  const uint16_t data = sample.data;
  const uint16_t extended = sample.extended;
  const uint8_t transitions = sample.transitions;
  const uint8_t id = sample.id;

  sample.result = AUTODETECT_NONE;

  if (data == 0xFFFF && extended == 0xFFFF && transitions == 0) {
    return sample.result;
  }

  if (data == 0x0000 && (extended == 0x0000 || extended == 0xFFFE)) {
    return sample.result;
  }

  if (id == 0x0 && (extended == 0xFFFF || extended == 0xFFFE)) {
    sample.result = AUTODETECT_SNES;
    return sample.result;
  }

  if (id == 0xF && (data & 0xFF) == 0xFF) {
    if (transitions > 0 && ((data >> 8) & 0xFF) != 0xFF) {
      sample.result = AUTODETECT_NES;
    }
    return sample.result;
  }

  if (id == 0x2) {
    sample.result = AUTODETECT_SNES;
    return sample.result;
  }

  if (id == 0xE || id == 0x8) {
    #ifdef ENABLE_EXPERIMENTAL_CONSOLE_MOUSE
    sample.result = AUTODETECT_SNES;
    #endif
    return sample.result;
  }

  if ((id & 0x4) == 0x4) {
    sample.result = AUTODETECT_VBOY;
  }

  return sample.result;
}

}  // namespace

AutoDetectResult AutoDetector::probeSNES(const autodetect_pins_t& pins) {
  return probeSNES(pins, 0xFF);
}

AutoDetectResult AutoDetector::probeSNES(const autodetect_pins_t& pins, uint8_t debug_port) {
  SnesProbeSample samples[kSnesProbePasses] = {};
  uint8_t snes_hits = 0;
  uint8_t nes_hits = 0;
  uint8_t vboy_hits = 0;
  AutoDetectResult first_result = AUTODETECT_NONE;
  AutoDetectResult final_result = AUTODETECT_NONE;
  uint8_t last_index = 0;

  for (uint8_t pass = 0; pass < kSnesProbePasses; ++pass) {
    samples[pass] = readSnesProbeSample(pins);
    AutoDetectResult result = classifySnesProbeSample(samples[pass]);
    last_index = pass;
    if (result != AUTODETECT_NONE && first_result == AUTODETECT_NONE) {
      first_result = result;
    }
    switch (result) {
      case AUTODETECT_SNES:
        ++snes_hits;
        break;
      case AUTODETECT_NES:
        ++nes_hits;
        break;
      case AUTODETECT_VBOY:
        ++vboy_hits;
        break;
      default:
        break;
    }
    if (snes_hits >= 2 || nes_hits >= 2 || vboy_hits >= 2) {
      break;
    }
    delayMicroseconds(250);
  }

  if (snes_hits >= 2) {
    final_result = AUTODETECT_SNES;
  } else if (nes_hits >= 2) {
    final_result = AUTODETECT_NES;
  } else if (vboy_hits >= 2) {
    final_result = AUTODETECT_VBOY;
  } else {
    final_result = first_result;
  }

  #ifdef AUTODETECT_DEBUG
  if (debug_port < kAutoDetectDebugPortCount) {
    const SnesProbeSample& sample = samples[last_index];
    lastDebug[debug_port].snes_data = sample.data;
    lastDebug[debug_port].snes_extended = sample.extended;
    lastDebug[debug_port].snes_id = sample.id;
    lastDebug[debug_port].snes_transitions = sample.transitions;
    lastDebug[debug_port].snes_result = (uint8_t)final_result;
    lastDebug[debug_port].snes_hits = snes_hits;
    lastDebug[debug_port].nes_hits = nes_hits;
    lastDebug[debug_port].vboy_hits = vboy_hits;
  }
  #else
  (void)debug_port;
  #endif

  return final_result;
}

AutoDetectResult AutoDetector::probePCE(const autodetect_pins_t& pins) {
  return probePCE(pins, 0xFF);
}

AutoDetectResult AutoDetector::probePCE(const autodetect_pins_t& pins, uint8_t debug_port) {
  const uint8_t pceSel = pins.sat_tl;
  const uint8_t pceClr = pins.sat_th;

  gpio_init(pins.sat_d0);
  gpio_init(pins.sat_d1);
  gpio_init(pins.sat_d2);
  gpio_init(pins.sat_d3);
  gpio_set_dir(pins.sat_d0, GPIO_IN);
  gpio_set_dir(pins.sat_d1, GPIO_IN);
  gpio_set_dir(pins.sat_d2, GPIO_IN);
  gpio_set_dir(pins.sat_d3, GPIO_IN);
  gpio_pull_up(pins.sat_d0);
  gpio_pull_up(pins.sat_d1);
  gpio_pull_up(pins.sat_d2);
  gpio_pull_up(pins.sat_d3);

  gpio_init(pceSel);
  gpio_init(pceClr);
  gpio_set_dir(pceSel, GPIO_OUT);
  gpio_set_dir(pceClr, GPIO_OUT);

  gpio_put(pceSel, 0);
  gpio_put(pceClr, 0);
  delayMicroseconds(50);

  gpio_put(pceSel, 1);
  gpio_put(pceClr, 1);
  delayMicroseconds(10);

  uint8_t nibble0 = (gpio_get(pins.sat_d3) << 3) | (gpio_get(pins.sat_d2) << 2) |
                    (gpio_get(pins.sat_d1) << 1) | gpio_get(pins.sat_d0);

  gpio_put(pceClr, 0);
  delayMicroseconds(10);

  uint8_t nibbleDpad = (gpio_get(pins.sat_d3) << 3) | (gpio_get(pins.sat_d2) << 2) |
                       (gpio_get(pins.sat_d1) << 1) | gpio_get(pins.sat_d0);

  gpio_put(pceSel, 0);
  delayMicroseconds(10);

  uint8_t nibbleBtn = (gpio_get(pins.sat_d3) << 3) | (gpio_get(pins.sat_d2) << 2) |
                      (gpio_get(pins.sat_d1) << 1) | gpio_get(pins.sat_d0);

  gpio_put(pceSel, 0);
  gpio_put(pceClr, 0);
  delayMicroseconds(5);
  gpio_set_dir(pceSel, GPIO_IN);
  gpio_set_dir(pceClr, GPIO_IN);

  const bool rawCandidate = (nibble0 == 0x0);
  #ifdef AUTODETECT_DEBUG
  if (debug_port < kAutoDetectDebugPortCount) {
    lastDebug[debug_port].pce_nibble0 = nibble0;
    lastDebug[debug_port].pce_nibble_dpad = nibbleDpad;
    lastDebug[debug_port].pce_nibble_btn = nibbleBtn;
    lastDebug[debug_port].pce_raw_candidate = rawCandidate;
  }
  #else
  (void)debug_port;
  #endif

  return rawCandidate ? AUTODETECT_PCE : AUTODETECT_NONE;
}

#ifdef ENABLE_INPUT_PCE
bool AutoDetector::confirmPCEViaPceLib(const autodetect_pins_t& pins, bool is_hotswap,
                                       uint8_t debug_port, bool allow_pad_confirm,
                                       bool* saw_multitap) {
  if (saw_multitap != nullptr) {
    *saw_multitap = false;
  }
  PcePort pceProbe(
    pins.sat_tl, pins.sat_th,
    pins.sat_d0, pins.sat_d1, pins.sat_d2, pins.sat_d3
  );
  pceProbe.begin();
  autoDetectDelay(is_hotswap ? 12 : 16);
  pceProbe.detectMultitap();
  uint8_t tapPorts = pceProbe.getMultitapPorts();
  #ifdef AUTODETECT_DEBUG
  if (debug_port < kAutoDetectDebugPortCount) {
    lastDebug[debug_port].pce_tap_ports = tapPorts;
  }
  #else
  (void)debug_port;
  #endif
  if (tapPorts >= TAP_PCE_PORTS) {
    if (saw_multitap != nullptr) {
      *saw_multitap = true;
    }
    return false;
  }

  if (!allow_pad_confirm) {
    return false;
  }

  autoDetectDelay(is_hotswap ? 30 : 60);

  uint8_t confirmedPasses = 0;
  uint8_t passes = is_hotswap ? 10 : 14;
  for (uint8_t pass = 0; pass < passes; ++pass) {
    pceProbe.update();

    bool validPce = false;
    uint8_t count = pceProbe.getControllerCount();
    #ifdef AUTODETECT_DEBUG
    if (debug_port < kAutoDetectDebugPortCount) {
      lastDebug[debug_port].pce_last_count = count;
    }
    #endif
    for (uint8_t i = 0; i < count; ++i) {
      const PceController& controller = pceProbe.getPceController(i);
      PceDeviceType_Enum dt = controller.deviceType();
      #ifdef AUTODETECT_DEBUG
      if (debug_port < kAutoDetectDebugPortCount && dt != PCE_DEVICE_NONE) {
        lastDebug[debug_port].pce_last_type = (uint8_t)dt;
      }
      #endif
      if (dt == PCE_DEVICE_PAD2 || dt == PCE_DEVICE_PAD6) {
        validPce = true;
        break;
      }
    }

    if (validPce) {
      if (++confirmedPasses >= 2) {
        return true;
      }
    } else if (confirmedPasses > 0) {
      --confirmedPasses;
    }

    autoDetectDelay(is_hotswap ? 4 : 6);
  }

  return false;
}

bool AutoDetector::confirmPCEViaRawProbe(const autodetect_pins_t& pins, bool is_hotswap) {
  uint8_t hits = 0;
  uint8_t passes = is_hotswap ? 3 : 4;

  for (uint8_t pass = 0; pass < passes; ++pass) {
    if (probePCE(pins) == AUTODETECT_PCE) {
      if (++hits >= 2) {
        return true;
      }
    }

    resetSharedPins(pins, is_hotswap);
    autoDetectDelay(is_hotswap ? 3 : 5);
  }

  return false;
}
#endif

#if defined(ENABLE_INPUT_SATURN) || defined(ENABLE_INPUT_MEGADRIVE)
bool AutoDetector::confirmSaturnViaSaturnLib(const autodetect_pins_t& pins,
                                             uint8_t port,
                                             bool is_hotswap) {
  SaturnPort satProbe(
    pins.sat_d0, pins.sat_d1, pins.sat_d2, pins.sat_d3,
    pins.sat_th, pins.sat_tr, pins.sat_tl,
    SatLibConfig_saturn_only
  );
  satProbe.begin();
  satProbe.detectMultitap();
  uint8_t tapPorts = satProbe.getMultitapPorts();
  #ifdef AUTODETECT_DEBUG
  if (port < kAutoDetectDebugPortCount) {
    lastDebug[port].satlib_tap_ports = tapPorts;
  }
  #endif
  if (tapPorts == TAP_SAT_PORTS) {
    return false;
  }

  autoDetectDelay(is_hotswap ? 90 : 140);

  uint8_t passes = is_hotswap ? 24 : 36;
  for (uint8_t pass = 0; pass < passes; ++pass) {
    satProbe.update();
    uint8_t satCount = satProbe.getControllerCount();
    for (uint8_t i = 0; i < satCount; ++i) {
      const SaturnController& controller = satProbe.getSaturnController(i);
      SatDeviceType_Enum dt = controller.deviceType();
      if (dt != SAT_DEVICE_NONE && dt != SAT_DEVICE_NOTSUPPORTED &&
          dt != SAT_DEVICE_MEGA3 && dt != SAT_DEVICE_MEGA6) {
        return true;
      }
    }
    autoDetectDelay(is_hotswap ? 6 : 4);
  }

  return false;
}
#endif
#endif
