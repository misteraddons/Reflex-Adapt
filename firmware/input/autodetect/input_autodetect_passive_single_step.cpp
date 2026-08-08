#include "../../product_config.h"
#include "../../firmware_platform_config.h"

#include <Arduino.h>

#include "../../platform/button_handler.h"
#include "../../platform/buzzer.h"

#include "input_autodetect_support.h"
#include "input_autodetect_passive_internal.h"

#ifdef ENABLE_INPUT_AUTODETECT
namespace input_autodetect_passive_internal {
namespace {

constexpr uint16_t kPassiveNeoActivityMask = 0x01;
constexpr uint16_t kPassiveLeftDirectMask = 0x02;
constexpr uint16_t kPassiveLeftWithTrMask = 0x04;
constexpr uint16_t kPassiveFireMask = 0x08;
constexpr uint16_t kPassiveTrLowMask = 0x10;
constexpr uint16_t kPassiveSmsDirectionActivityMask = 0x20;
constexpr uint16_t kPassiveSmsRightMask = 0x40;
constexpr uint16_t kPassiveSmsUpMask = 0x80;
constexpr uint16_t kPassiveSmsDownMask = 0x100;
constexpr uint16_t kPassiveSmsFireToleratedMask =
  kPassiveFireMask |
  kPassiveLeftDirectMask |
  kPassiveLeftWithTrMask |
  kPassiveTrLowMask |
  kPassiveSmsDirectionActivityMask |
  kPassiveSmsRightMask |
  kPassiveSmsUpMask |
  kPassiveSmsDownMask;

uint16_t samplePassiveSingleStepMask(const autodetect_pins_t& pins) {
  const uint8_t idPins[] = {
    pins.neo_start,
    pins.sms_up, pins.sms_down, pins.sms_left, pins.sms_right,
    pins.sms_tl, pins.sms_tr,
    pins.sat_th,
  };
  for (uint8_t i = 0; i < sizeof(idPins); i++) {
    gpio_set_function(idPins[i], GPIO_FUNC_SIO);
    gpio_init(idPins[i]);
  }

  gpio_set_dir(pins.sms_up, GPIO_IN);    gpio_pull_up(pins.sms_up);
  gpio_set_dir(pins.sms_down, GPIO_IN);  gpio_pull_up(pins.sms_down);
  gpio_set_dir(pins.neo_start, GPIO_IN); gpio_pull_up(pins.neo_start);
  gpio_set_dir(pins.sms_left, GPIO_IN);  gpio_pull_up(pins.sms_left);
  gpio_set_dir(pins.sms_right, GPIO_IN); gpio_pull_up(pins.sms_right);
  gpio_set_dir(pins.sms_tl, GPIO_IN);    gpio_pull_up(pins.sms_tl);
  gpio_set_dir(pins.sat_th, GPIO_IN);    gpio_pull_up(pins.sat_th);

  gpio_set_dir(pins.sms_tr, GPIO_IN);
  gpio_pull_up(pins.sms_tr);
  autoDetectDelay(3);
  bool sms_up_low = !gpio_get(pins.sms_up);
  bool sms_down_low = !gpio_get(pins.sms_down);
  bool neo_start_low = !gpio_get(pins.neo_start);
  bool left_direct_low = !gpio_get(pins.sms_left);
  bool sms_right_low = !gpio_get(pins.sms_right);
  bool fire_low = !gpio_get(pins.sms_tl);
  bool tr_released_low = !gpio_get(pins.sms_tr);
  bool pce_clr_high = gpio_get(pins.sat_th);

  if (pce_clr_high && sms_up_low && sms_down_low && left_direct_low && sms_right_low) {
    return 0;
  }

  gpio_set_dir(pins.sms_tr, GPIO_OUT);
  gpio_put(pins.sms_tr, 0);
  autoDetectDelay(3);
  bool left_with_tr_low = !gpio_get(pins.sms_left);

  gpio_set_dir(pins.sms_tr, GPIO_IN);
  gpio_pull_up(pins.sms_tr);

  uint16_t mask = 0;
  if (neo_start_low) mask |= kPassiveNeoActivityMask;
  if (left_direct_low)  mask |= kPassiveLeftDirectMask;
  if (left_with_tr_low) mask |= kPassiveLeftWithTrMask;
  if (fire_low)         mask |= kPassiveFireMask;
  if (tr_released_low)  mask |= kPassiveTrLowMask;
  if (sms_right_low)    mask |= kPassiveSmsRightMask;
  if (sms_up_low)       mask |= kPassiveSmsUpMask;
  if (sms_down_low)     mask |= kPassiveSmsDownMask;
  if (sms_up_low || sms_down_low || left_direct_low || sms_right_low) {
    mask |= kPassiveSmsDirectionActivityMask;
  }
  return mask;
}

bool hasPassiveSmsDirectionConflict(uint16_t mask) {
  const bool left_right_conflict =
    (mask & kPassiveLeftDirectMask) != 0 &&
    (mask & kPassiveSmsRightMask) != 0;
  const bool up_down_conflict =
    (mask & kPassiveSmsUpMask) != 0 &&
    (mask & kPassiveSmsDownMask) != 0;
  return left_right_conflict || up_down_conflict;
}

bool isPassiveSmsFireAssist(uint16_t mask) {
  // Assisted Atari/C64/SMS is an intentional hold-Fire route. Real passive wiring
  // can show direction/TR artifacts while Fire is held; opposing directions are
  // not valid SMS-family input and are usually another controller on the bus.
  return (mask & kPassiveFireMask) != 0 &&
         (mask & (uint16_t)(~kPassiveSmsFireToleratedMask)) == 0 &&
         !hasPassiveSmsDirectionConflict(mask);
}

AutoDetectResult classifyPassiveSingleStepMask(uint16_t mask) {
  if (mask & kPassiveNeoActivityMask) return AUTODETECT_NEOGEO;

  bool left_direct = (mask & kPassiveLeftDirectMask) != 0;
  bool left_with_tr = (mask & kPassiveLeftWithTrMask) != 0;
  bool tr_released = (mask & kPassiveTrLowMask) != 0;
  if (isPassiveSmsFireAssist(mask)) return AUTODETECT_SMS;
  if (left_direct && tr_released) return AUTODETECT_JPC;
  if (!left_direct && left_with_tr) return AUTODETECT_JPC;
  return AUTODETECT_NONE;
}

AutoDetectResult probePassiveSingleStep(uint8_t port) {
  if (port >= kAutoDetectPortCount) {
    return AUTODETECT_NONE;
  }

  const autodetect_pins_t& pins = autodetect_pins[port];
  uint16_t mask = samplePassiveSingleStepMask(pins);

  if (mask == 0) {
    passive_single_step_prev_mask[port] = 0;
    passive_single_step_armed[port] = true;
    return AUTODETECT_NONE;
  }

  if (!passive_single_step_armed[port]) {
    passive_single_step_prev_mask[port] = mask;
    return AUTODETECT_NONE;
  }

  uint16_t edgeMask = (mask & (uint16_t)(~passive_single_step_prev_mask[port]));
  if (edgeMask == 0) {
    passive_single_step_prev_mask[port] = mask;
    return AUTODETECT_NONE;
  }

  autoDetectDelay(20);
  uint16_t confirmMask = samplePassiveSingleStepMask(pins);
  if (confirmMask == 0 || confirmMask != mask) {
    passive_single_step_prev_mask[port] = confirmMask;
    return AUTODETECT_NONE;
  }

  uint16_t confirmEdgeMask = (confirmMask & (uint16_t)(~passive_single_step_prev_mask[port]));
  passive_single_step_prev_mask[port] = confirmMask;
  passive_single_step_armed[port] = false;

  if (confirmEdgeMask == 0) {
    return AUTODETECT_NONE;
  }
  AutoDetectResult edgeResult = classifyPassiveSingleStepMask(confirmEdgeMask);
  if (edgeResult == AUTODETECT_SMS && !isPassiveSmsFireAssist(confirmMask)) {
    return AUTODETECT_NONE;
  }
  return edgeResult;
}

AutoDetectResult probePassiveSingleStepHeld(uint8_t port) {
  if (port >= kAutoDetectPortCount) {
    return AUTODETECT_NONE;
  }

  const autodetect_pins_t& pins = autodetect_pins[port];
  uint16_t mask = samplePassiveSingleStepMask(pins);
  AutoDetectResult result = classifyPassiveSingleStepMask(mask);
  uint32_t now = millis();

  if (mask == 0 || result == AUTODETECT_NONE) {
    AutoDetectResult releaseResult = AUTODETECT_NONE;
    AutoDetectResult previousResult = classifyPassiveSingleStepMask(passive_assisted_hold_last_mask[port]);
    if (!passive_assisted_hold_latched[port] &&
        passive_assisted_hold_last_mask[port] != 0 &&
        previousResult != AUTODETECT_NONE &&
        passive_assisted_hold_stable_since[port] != 0 &&
        (now - passive_assisted_hold_stable_since[port]) >= ASSISTED_ROUTE_STABLE_MS) {
      releaseResult = previousResult;
    }
    passive_assisted_hold_last_mask[port] = mask;
    passive_assisted_hold_stable_since[port] = 0;
    passive_assisted_hold_latched[port] = false;
    return releaseResult;
  }

  if (passive_assisted_hold_latched[port]) {
    return AUTODETECT_NONE;
  }

  if (mask != passive_assisted_hold_last_mask[port]) {
    passive_assisted_hold_last_mask[port] = mask;
    passive_assisted_hold_stable_since[port] = now;
    return AUTODETECT_NONE;
  }

  if (passive_assisted_hold_stable_since[port] == 0) {
    passive_assisted_hold_stable_since[port] = now;
    return AUTODETECT_NONE;
  }

  if (now - passive_assisted_hold_stable_since[port] < ASSISTED_ROUTE_STABLE_MS) {
    return AUTODETECT_NONE;
  }

  passive_assisted_hold_latched[port] = true;
  return result;
}

}  // namespace

AutoDetectResult probePassiveOnlyAssistedRoute(uint8_t port) {
  AutoDetectResult result = probePassiveSingleStep(port);
  if (result != AUTODETECT_NONE) {
    return result;
  }
  return probePassiveSingleStepHeld(port);
}

bool passiveHoldRoutePresent(uint8_t port) {
  if (port >= kAutoDetectPortCount) {
    return false;
  }

  const autodetect_pins_t& pins = autodetect_pins[port];
  return classifyPassiveSingleStepMask(samplePassiveSingleStepMask(pins)) != AUTODETECT_NONE;
}

}  // namespace input_autodetect_passive_internal
#endif
