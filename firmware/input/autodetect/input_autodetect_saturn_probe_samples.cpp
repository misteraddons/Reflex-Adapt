#include "../../product_config.h"
#include "../../firmware_platform_config.h"

#include <Arduino.h>

#include "../../platform/button_handler.h"
#include "../../platform/buzzer.h"
#include "Input_AutoDetect.h"
#include "input_autodetect_saturn_probe_internal.h"

#ifdef ENABLE_INPUT_AUTODETECT
namespace {

void recordSaturnPrimarySample(
    SaturnProbeState& state,
    uint8_t sample,
    uint8_t nibble_0,
    uint8_t nibble_1,
    uint8_t tl_0,
    uint8_t tr_0,
    uint8_t tl_1,
    uint8_t tr_1) {
  if (sample == 0) {
    state.first_nibble_0 = nibble_0;
    state.first_nibble_1 = nibble_1;
    state.first_tl_0 = tl_0;
    state.first_tr_0 = tr_0;
    state.first_tl_1 = tl_1;
    state.first_tr_1 = tr_1;
  }
  if (sample + 1 == kSaturnProbeSampleCount) {
    state.last_nibble_0 = nibble_0;
    state.last_nibble_1 = nibble_1;
    state.last_tl_0 = tl_0;
    state.last_tr_0 = tr_0;
    state.last_tl_1 = tl_1;
    state.last_tr_1 = tr_1;
  } else {
    if (nibble_0 != state.first_nibble_0) state.stable_nibble_0 = false;
    if (nibble_1 != state.first_nibble_1) state.stable_nibble_1 = false;
    if (tl_0 != state.first_tl_0) state.stable_tl_0 = false;
    if (tr_0 != state.first_tr_0) state.stable_tr_0 = false;
    if (tl_1 != state.first_tl_1) state.stable_tl_1 = false;
    if (tr_1 != state.first_tr_1) state.stable_tr_1 = false;
  }

  if (!(nibble_0 == 0x0F && nibble_1 == 0x0F && tl_0 == 1 && tl_1 == 1) || (tr_0 != tr_1)) {
    state.activity_hits++;
  }

  if ((nibble_0 & 0x07) == 0x04) {
    state.saturn_hits++;
  } else if ((nibble_0 & 0x0F) == 0x01 && (nibble_1 & 0x0F) == 0x01) {
    state.saturn_hits++;
  } else if ((nibble_0 & 0x0F) == 0x02 && (nibble_1 & 0x0F) == 0x03) {
  } else if ((nibble_1 & 0x0C) == 0x00) {
    if (nibble_0 != 0x0F || nibble_1 != 0x0F || tl_0 != tl_1 || tr_0 != tr_1) {
      state.megadrive_hits++;
    }
  }
}

}  // namespace

void initializeSaturnProbeBus(const autodetect_pins_t& pins, bool is_hotswap) {
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

  gpio_init(pins.sat_th);
  gpio_init(pins.sat_tr);
  gpio_init(pins.sat_tl);

  gpio_set_dir(pins.sat_th, GPIO_OUT);
  gpio_set_dir(pins.sat_tr, GPIO_IN);
  gpio_set_dir(pins.sat_tl, GPIO_IN);
  gpio_pull_up(pins.sat_tr);
  gpio_pull_up(pins.sat_tl);

  gpio_put(pins.sat_th, 1);
  gpio_set_dir(pins.sat_tr, GPIO_OUT);
  gpio_put(pins.sat_tr, 1);
  autoDetectDelay(is_hotswap ? 2 : 4);
  for (uint8_t warm = 0; warm < 8; ++warm) {
    gpio_put(pins.sat_th, 0);
    delayMicroseconds(20);
    gpio_put(pins.sat_th, 1);
    delayMicroseconds(20);
  }
  gpio_set_dir(pins.sat_tr, GPIO_IN);
  gpio_pull_up(pins.sat_tr);
}

void captureSaturnProbeSamples(const autodetect_pins_t& pins, uint8_t sat_settle_us,
                               SaturnProbeState& state) {
  for (uint8_t sample = 0; sample < kSaturnProbeSampleCount; ++sample) {
    gpio_put(pins.sat_th, 1);
    delayMicroseconds(sat_settle_us);

    uint8_t nibble_0 = (gpio_get(pins.sat_d3) << 3) | (gpio_get(pins.sat_d2) << 2) |
                       (gpio_get(pins.sat_d1) << 1) | gpio_get(pins.sat_d0);
    uint8_t tl_0 = gpio_get(pins.sat_tl);
    uint8_t tr_0 = gpio_get(pins.sat_tr);

    gpio_put(pins.sat_th, 0);
    delayMicroseconds(sat_settle_us);

    uint8_t nibble_1 = (gpio_get(pins.sat_d3) << 3) | (gpio_get(pins.sat_d2) << 2) |
                       (gpio_get(pins.sat_d1) << 1) | gpio_get(pins.sat_d0);
    uint8_t tl_1 = gpio_get(pins.sat_tl);
    uint8_t tr_1 = gpio_get(pins.sat_tr);

    recordSaturnPrimarySample(state, sample, nibble_0, nibble_1, tl_0, tr_0, tl_1, tr_1);
    delayMicroseconds(30);
  }

  gpio_put(pins.sat_th, 1);
  delayMicroseconds(5);
  gpio_set_dir(pins.sat_th, GPIO_IN);
  gpio_pull_up(pins.sat_th);
}

void retrySaturnQuietWindow(const autodetect_pins_t& pins, uint8_t sat_settle_us,
                            SaturnProbeState& state) {
  if (state.saturn_hits != 0 || state.megadrive_hits != 0 || state.activity_hits != 0) {
    return;
  }

  gpio_set_function(pins.sat_th, GPIO_FUNC_SIO);
  gpio_init(pins.sat_th);
  gpio_set_dir(pins.sat_th, GPIO_OUT);
  gpio_set_function(pins.sat_tr, GPIO_FUNC_SIO);
  gpio_init(pins.sat_tr);
  gpio_set_dir(pins.sat_tr, GPIO_OUT);
  gpio_put(pins.sat_tr, 1);
  delayMicroseconds(20);

  uint8_t retry_activity = 0;
  uint8_t retry_n0_first = 0x0F;
  uint8_t retry_n1_first = 0x0F;
  uint8_t retry_n0_last = 0x0F;
  uint8_t retry_n1_last = 0x0F;

  for (uint8_t sample = 0; sample < 4; ++sample) {
    gpio_put(pins.sat_th, 1);
    delayMicroseconds(sat_settle_us);
    uint8_t n0 = (gpio_get(pins.sat_d3) << 3) | (gpio_get(pins.sat_d2) << 2) |
                 (gpio_get(pins.sat_d1) << 1) | gpio_get(pins.sat_d0);

    gpio_put(pins.sat_th, 0);
    delayMicroseconds(sat_settle_us);
    uint8_t n1 = (gpio_get(pins.sat_d3) << 3) | (gpio_get(pins.sat_d2) << 2) |
                 (gpio_get(pins.sat_d1) << 1) | gpio_get(pins.sat_d0);

    if (sample == 0) {
      retry_n0_first = n0;
      retry_n1_first = n1;
    }
    retry_n0_last = n0;
    retry_n1_last = n1;

    if (!(n0 == 0x0F && n1 == 0x0F)) {
      retry_activity++;
    }
  }

  gpio_put(pins.sat_th, 1);
  gpio_set_dir(pins.sat_th, GPIO_IN);
  gpio_pull_up(pins.sat_th);
  gpio_set_dir(pins.sat_tr, GPIO_IN);
  gpio_pull_up(pins.sat_tr);

  if (retry_activity > 0) {
    state.activity_hits = retry_activity;
    state.first_nibble_0 = retry_n0_first;
    state.first_nibble_1 = retry_n1_first;
    state.last_nibble_0 = retry_n0_last;
    state.last_nibble_1 = retry_n1_last;

    if ((retry_n0_first & 0x07) == 0x04 ||
        ((retry_n0_first & 0x0F) == 0x01 && (retry_n1_first & 0x0F) == 0x01) ||
        ((retry_n0_first & 0x0F) == 0x02 && (retry_n1_first & 0x0F) == 0x03)) {
      state.saturn_hits = 1;
    } else if ((retry_n1_first & 0x0C) == 0x00) {
      state.megadrive_hits = 1;
    }
  }
}

void retrySaturnToggleWindow(const autodetect_pins_t& pins, uint8_t sat_settle_us,
                             SaturnProbeState& state) {
  if (state.saturn_hits != 0 || state.megadrive_hits != 0 || state.activity_hits != 0) {
    return;
  }

  gpio_set_function(pins.sat_th, GPIO_FUNC_SIO);
  gpio_init(pins.sat_th);
  gpio_set_dir(pins.sat_th, GPIO_OUT);
  gpio_set_function(pins.sat_tr, GPIO_FUNC_SIO);
  gpio_init(pins.sat_tr);
  gpio_set_dir(pins.sat_tr, GPIO_OUT);
  gpio_put(pins.sat_tr, 1);
  gpio_put(pins.sat_th, 1);
  autoDetectDelay(1);

  for (uint8_t warm = 0; warm < 3; ++warm) {
    gpio_put(pins.sat_th, 0);
    delayMicroseconds(20);
    gpio_put(pins.sat_th, 1);
    delayMicroseconds(20);
  }

  uint8_t retry_activity = 0;
  uint8_t retry_n0_first = 0x0F;
  uint8_t retry_n1_first = 0x0F;
  uint8_t retry_tl0_first = 1;
  uint8_t retry_tr0_first = 1;
  uint8_t retry_tl1_first = 1;
  uint8_t retry_tr1_first = 1;
  uint8_t retry_n0_last = 0x0F;
  uint8_t retry_n1_last = 0x0F;
  uint8_t retry_tl0_last = 1;
  uint8_t retry_tr0_last = 1;
  uint8_t retry_tl1_last = 1;
  uint8_t retry_tr1_last = 1;

  for (uint8_t sample = 0; sample < 8; ++sample) {
    gpio_put(pins.sat_tr, (sample & 1) ? 1 : 0);
    delayMicroseconds(4);

    gpio_put(pins.sat_th, 1);
    delayMicroseconds(sat_settle_us);
    uint8_t n0 = (gpio_get(pins.sat_d3) << 3) | (gpio_get(pins.sat_d2) << 2) |
                 (gpio_get(pins.sat_d1) << 1) | gpio_get(pins.sat_d0);
    uint8_t tl0 = gpio_get(pins.sat_tl);
    uint8_t tr0 = gpio_get(pins.sat_tr);

    gpio_put(pins.sat_th, 0);
    delayMicroseconds(sat_settle_us);
    uint8_t n1 = (gpio_get(pins.sat_d3) << 3) | (gpio_get(pins.sat_d2) << 2) |
                 (gpio_get(pins.sat_d1) << 1) | gpio_get(pins.sat_d0);
    uint8_t tl1 = gpio_get(pins.sat_tl);
    uint8_t tr1 = gpio_get(pins.sat_tr);

    if (sample == 0) {
      retry_n0_first = n0;
      retry_n1_first = n1;
      retry_tl0_first = tl0;
      retry_tr0_first = tr0;
      retry_tl1_first = tl1;
      retry_tr1_first = tr1;
    }
    retry_n0_last = n0;
    retry_n1_last = n1;
    retry_tl0_last = tl0;
    retry_tr0_last = tr0;
    retry_tl1_last = tl1;
    retry_tr1_last = tr1;

    if (!(n0 == 0x0F && n1 == 0x0F && tl0 == 1 && tl1 == 1) || (tr0 != tr1)) {
      retry_activity++;
    }

    delayMicroseconds(80);
  }

  gpio_put(pins.sat_th, 1);
  gpio_set_dir(pins.sat_th, GPIO_IN);
  gpio_pull_up(pins.sat_th);
  gpio_set_dir(pins.sat_tr, GPIO_IN);
  gpio_pull_up(pins.sat_tr);

  if (retry_activity > 0) {
    state.activity_hits = retry_activity;
    state.first_nibble_0 = retry_n0_first;
    state.first_nibble_1 = retry_n1_first;
    state.last_nibble_0 = retry_n0_last;
    state.last_nibble_1 = retry_n1_last;
    state.first_tl_0 = retry_tl0_first;
    state.first_tr_0 = retry_tr0_first;
    state.first_tl_1 = retry_tl1_first;
    state.first_tr_1 = retry_tr1_first;
    state.last_tl_0 = retry_tl0_last;
    state.last_tr_0 = retry_tr0_last;
    state.last_tl_1 = retry_tl1_last;
    state.last_tr_1 = retry_tr1_last;

    if ((retry_n0_first & 0x07) == 0x04 ||
        ((retry_n0_first & 0x0F) == 0x01 && (retry_n1_first & 0x0F) == 0x01) ||
        ((retry_n0_first & 0x0F) == 0x02 && (retry_n1_first & 0x0F) == 0x03)) {
      state.saturn_hits = 1;
    } else if ((retry_n1_first & 0x0C) == 0x00) {
      state.megadrive_hits = 1;
    }
  }
}
#endif
