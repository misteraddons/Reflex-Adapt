#include "Input_Neogeo.h"

#include <Arduino.h>

#include "../shared/input_button_bits.h"
#include "../../platform/latency_trace_gpio.h"
#include "../../platform/latency_test.h"

uint32_t __not_in_flash_func(RZInputNeoGeo::filterImmediatePressDebounce)(uint8_t port, uint32_t rawState) {
  const uint8_t debounceMs = input_neogeo_config[port].debounceMs;
  if (debounceMs == 0) {
    uint32_t changed = (rawState ^ acceptedRawState[port]) & buttonMask[port];
    while (changed) {
      const uint8_t pin = __builtin_ctz(changed);
      const uint8_t shift = outputButtonShiftByPin[port][pin];
      if (shift != 0) {
        acceptedButtons[port] ^= 1UL << (shift - 1U);
      }
      changed &= changed - 1U;
    }
    acceptedRawState[port] = rawState;
    return rawState;
  }

  const uint32_t changed = (rawState ^ acceptedRawState[port]) & buttonMask[port];
  if (changed == 0) {
    return acceptedRawState[port];
  }

  const uint32_t nowMs = millis();
  uint32_t pending = changed;
  while (pending) {
    const uint8_t pin = __builtin_ctz(pending);
    const uint32_t bit = 1UL << pin;
    pending &= ~bit;

    if ((int32_t)(nowMs - debounceBlockedUntilMs[port][pin]) < 0) {
      continue;
    }

    // Accept the first edge immediately, then hold this bit stable for the
    // debounce window so bounce does not create extra press/release reports.
    acceptedRawState[port] ^= bit;
    const uint8_t shift = outputButtonShiftByPin[port][pin];
    if (shift != 0) {
      acceptedButtons[port] ^= 1UL << (shift - 1U);
    }
    debounceBlockedUntilMs[port][pin] = nowMs + debounceMs;
  }

  return acceptedRawState[port];
}

void __not_in_flash_func(RZInputNeoGeo::stageWebhidRawData)(uint8_t port, uint32_t rawState) {
  if (port != 0) {
    return;
  }
  pendingWebhidRawPort = port;
  pendingWebhidRawState = rawState;
  pendingWebhidRawDirty = true;
}

void RZInputNeoGeo::flushPendingWebhidRawData() {
  if (!pendingWebhidRawDirty) {
    return;
  }
  const uint32_t rawState = pendingWebhidRawState;
  uint8_t raw[16] = {0};
  raw[0] = 1;
  raw[1] = rawState >> 24;
  raw[2] = (rawState >> 16) & 0xFF;
  raw[3] = (rawState >> 8) & 0xFF;
  raw[4] = rawState & 0xFF;
  raw[5] = pendingWebhidRawPort;
  webhid_store_raw_data(raw, 16);
  pendingWebhidRawDirty = false;
}

bool __not_in_flash_func(RZInputNeoGeo::poll)() {
  beginPollCycle();
  uint32_t gpioState = 0;
  {
    LatencyPhaseTraceScope neogeoReadTrace(LATENCY_TRACE_PHASE_NEOGEO_READ);
    gpioState = ~gpio_get_all();
  }
  {
    LatencyPhaseTraceScope neogeoMapTrace(LATENCY_TRACE_PHASE_NEOGEO_MAP);
    for (uint8_t port = 0; port < input_ports; ++port) {
      uint8_t i = port;
      const uint32_t pinState = gpioState & buttonMask[port];
      if (port == 0 && latencyTest.isEnabled()) {
        latencyTest.observeRawButtons(0, pinState, true);
      }
      const uint32_t rawState = filterImmediatePressDebounce(port, pinState);
      if (rawState != lastRawState[port]) {
        lastRawState[port] = rawState;
        const uint32_t buttons = acceptedButtons[port];

        controller_state_t& frame = inputFrame(i);
        frame.digital_buttons = buttons;
        if (i == 0 && latencyTest.isEnabled()) {
          latencyTest.observeButtons(0, buttons, true);
        }
        setUpdated(i);

        stageWebhidRawData(port, rawState);
      }
    }
  }
  return endPollCycle();
}

void RZInputNeoGeo::afterOutputFrameSent(bool polled, bool updated) {
  (void)polled;
  (void)updated;
  flushPendingWebhidRawData();
}
