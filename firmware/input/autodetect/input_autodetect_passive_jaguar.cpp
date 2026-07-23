#include "../../product_config.h"
#include "../../firmware_platform_config.h"

#include <Arduino.h>

#include "../../platform/button_handler.h"
#include "../../platform/buzzer.h"

#include "input_autodetect_support.h"
#include "input_autodetect_passive_internal.h"

#ifdef ENABLE_INPUT_AUTODETECT
namespace input_autodetect_passive_internal {
#ifdef ENABLE_INPUT_JAGUAR
namespace {

constexpr uint8_t kJaguarAssistPauseMask = 0x01;

uint8_t sampleJaguarAssistMask(const autodetect_pins_t& pins) {
  gpio_init(pins.jag_j0);
  gpio_init(pins.jag_j1);
  gpio_init(pins.jag_j2);
  gpio_init(pins.jag_j3);
  gpio_set_dir(pins.jag_j0, GPIO_OUT);
  gpio_set_dir(pins.jag_j1, GPIO_OUT);
  gpio_set_dir(pins.jag_j2, GPIO_OUT);
  gpio_set_dir(pins.jag_j3, GPIO_OUT);
  gpio_put(pins.jag_j0, HIGH);
  gpio_put(pins.jag_j1, HIGH);
  gpio_put(pins.jag_j2, HIGH);
  gpio_put(pins.jag_j3, HIGH);

  gpio_init(pins.jag_b0);
  gpio_init(pins.jag_b1);
  gpio_init(pins.jag_j8);
  gpio_init(pins.jag_j9);
  gpio_init(pins.jag_j10);
  gpio_init(pins.jag_j11);
  gpio_set_dir(pins.jag_b0, GPIO_IN);
  gpio_set_dir(pins.jag_b1, GPIO_IN);
  gpio_set_dir(pins.jag_j8, GPIO_IN);
  gpio_set_dir(pins.jag_j9, GPIO_IN);
  gpio_set_dir(pins.jag_j10, GPIO_IN);
  gpio_set_dir(pins.jag_j11, GPIO_IN);
  gpio_pull_up(pins.jag_b0);
  gpio_pull_up(pins.jag_b1);
  gpio_pull_up(pins.jag_j8);
  gpio_pull_up(pins.jag_j9);
  gpio_pull_up(pins.jag_j10);
  gpio_pull_up(pins.jag_j11);

  auto read_row = [&](uint8_t row) -> uint8_t {
    switch (row) {
      case 0: gpio_put(pins.jag_j0, LOW); break;
      case 1: gpio_put(pins.jag_j1, LOW); break;
      case 2: gpio_put(pins.jag_j2, LOW); break;
      case 3: gpio_put(pins.jag_j3, LOW); break;
    }
    delayMicroseconds(8);
    uint8_t nibble =
      (gpio_get(pins.jag_b0) << 5) |
      (gpio_get(pins.jag_b1) << 4) |
      (gpio_get(pins.jag_j8) << 3) |
      (gpio_get(pins.jag_j9) << 2) |
      (gpio_get(pins.jag_j10) << 1) |
      gpio_get(pins.jag_j11);
    switch (row) {
      case 0: gpio_put(pins.jag_j0, HIGH); break;
      case 1: gpio_put(pins.jag_j1, HIGH); break;
      case 2: gpio_put(pins.jag_j2, HIGH); break;
      case 3: gpio_put(pins.jag_j3, HIGH); break;
    }
    return nibble;
  };

  uint8_t nibbles_a[4];
  uint8_t nibbles_b[4];
  for (uint8_t row = 0; row < 4; ++row) {
    nibbles_a[row] = read_row(row);
  }
  delayMicroseconds(8);
  for (uint8_t row = 0; row < 4; ++row) {
    nibbles_b[row] = read_row(row);
  }

  gpio_set_dir(pins.jag_j0, GPIO_IN);
  gpio_set_dir(pins.jag_j1, GPIO_IN);
  gpio_set_dir(pins.jag_j2, GPIO_IN);
  gpio_set_dir(pins.jag_j3, GPIO_IN);
  gpio_pull_up(pins.jag_j0);
  gpio_pull_up(pins.jag_j1);
  gpio_pull_up(pins.jag_j2);
  gpio_pull_up(pins.jag_j3);

  auto has_non_idle_activity = [](const uint8_t* nibbles) -> bool {
    for (uint8_t row = 0; row < 4; ++row) {
      if (nibbles[row] != 0x3F) {
        return true;
      }
    }
    return false;
  };
  auto has_row_variant = [](const uint8_t* nibbles) -> bool {
    return !(nibbles[0] == nibbles[1] && nibbles[0] == nibbles[2] && nibbles[0] == nibbles[3]);
  };

  bool non_idle_a = has_non_idle_activity(nibbles_a);
  bool non_idle_b = has_non_idle_activity(nibbles_b);
  bool row_variant_a = has_row_variant(nibbles_a);
  bool row_variant_b = has_row_variant(nibbles_b);
  uint8_t diff_bits = 0;
  for (uint8_t row = 0; row < 4; ++row) {
    diff_bits += __builtin_popcount((unsigned)(nibbles_a[row] ^ nibbles_b[row]));
  }

  bool row_activity = ((row_variant_a && non_idle_a) || (row_variant_b && non_idle_b));
  bool stable = diff_bits <= 2;
  if (!row_activity || !stable) {
    return 0;
  }

  const uint8_t* nibbles = non_idle_b ? nibbles_b : nibbles_a;
  uint8_t page0 = nibbles[0];
  const bool pause_held = (page0 & 0x20) == 0;

  // Jaguar shares enough lines with Saturn that broad "any button" assisted
  // AUTO can classify held Saturn states as Jaguar. Require a specific Jaguar
  // Pause hold so assisted detection stays intentional.
  return pause_held ? kJaguarAssistPauseMask : 0;
}

}  // namespace

bool jaguarAssistHoldPresent(uint8_t port) {
  if (port >= kAutoDetectPortCount) {
    return false;
  }
  const autodetect_pins_t& pins = autodetect_pins[port];
  return sampleJaguarAssistMask(pins) != 0;
}

AutoDetectResult probeJaguarAssistHeldInternal(uint8_t port) {
  if (port >= kAutoDetectPortCount) {
    return AUTODETECT_NONE;
  }

  const autodetect_pins_t& pins = autodetect_pins[port];
  uint8_t mask = sampleJaguarAssistMask(pins);
  uint32_t now = millis();

  if (mask == 0) {
    AutoDetectResult releaseResult = AUTODETECT_NONE;
    if (!jaguar_assist_hold_latched[port] &&
        jaguar_assist_hold_last_mask[port] != 0 &&
        jaguar_assist_hold_stable_since[port] != 0 &&
        (now - jaguar_assist_hold_stable_since[port]) >= ASSISTED_ROUTE_STABLE_MS) {
      releaseResult = AUTODETECT_JAGUAR;
    }
    jaguar_assist_hold_last_mask[port] = 0;
    jaguar_assist_hold_stable_since[port] = 0;
    jaguar_assist_hold_latched[port] = false;
    return releaseResult;
  }

  if (jaguar_assist_hold_latched[port]) {
    return AUTODETECT_NONE;
  }

  if (mask != jaguar_assist_hold_last_mask[port]) {
    jaguar_assist_hold_last_mask[port] = mask;
    jaguar_assist_hold_stable_since[port] = now;
    return AUTODETECT_NONE;
  }

  if (jaguar_assist_hold_stable_since[port] == 0) {
    jaguar_assist_hold_stable_since[port] = now;
    return AUTODETECT_NONE;
  }

  if (now - jaguar_assist_hold_stable_since[port] < ASSISTED_ROUTE_STABLE_MS) {
    return AUTODETECT_NONE;
  }

  jaguar_assist_hold_latched[port] = true;
  return AUTODETECT_JAGUAR;
}
#endif
}  // namespace input_autodetect_passive_internal
#endif
