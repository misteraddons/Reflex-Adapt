#include "rumble_test_runtime.h"

#include <Arduino.h>

#include "../firmware_platform_config.h"
#include "controller_settings_state.h"

namespace {
// Some Windows/XInput tester paths interleave zero-strength OUT reports with
// sparse nonzero motor commands while an effect is visually held.  XInput rumble
// is a state, so bridge brief zero chatter while still stopping promptly.
constexpr uint16_t kHostZeroStopConfirmMs = 1000;
constexpr uint32_t kAllRumblePortsMask = 0xFFFFFFFFu;

struct HostRumblePortState {
  uint8_t rawLeft = 0;
  uint8_t rawRight = 0;
  uint8_t effectiveLeft = 0;
  uint8_t effectiveRight = 0;
  uint8_t scaledLeft = 0;
  uint8_t scaledRight = 0;
  uint32_t updateCount = 0;
  uint32_t lastNonzeroMs = 0;
  uint32_t zeroSinceMs = 0;
  bool zeroPending = false;
};

struct TestRumblePortState {
  uint8_t left = 0;
  uint8_t right = 0;
};

HostRumblePortState hostRumble[MAX_USB_OUT] = {};
TestRumblePortState testRumble[MAX_USB_OUT] = {};
uint32_t testPortMask = 0;
uint32_t testEndMs = 0;
bool testActive = false;

uint8_t scaleRumble(uint8_t value) {
  static const uint8_t rumbleScale[] = { 0, 128, 192, 255 };
  const uint8_t scale = rumbleScale[rumble_level & 0x03];
  return (uint8_t)(((uint16_t)value * scale) / 255u);
}

bool portMaskContains(uint32_t mask, uint8_t port) {
  return port < 32 && ((mask & (1UL << port)) != 0);
}

bool testTargetsPort(uint8_t port) {
  return testActive && portMaskContains(testPortMask, port);
}

void expirePendingHostZero(HostRumblePortState& state) {
  if (!state.zeroPending) return;
  const uint32_t referenceMs = state.lastNonzeroMs != 0
    ? state.lastNonzeroMs
    : state.zeroSinceMs;
  if ((int32_t)(millis() - referenceMs) < kHostZeroStopConfirmMs) return;

  state.effectiveLeft = 0;
  state.effectiveRight = 0;
  state.zeroPending = false;
}

void applyRumblePort(uint8_t port) {
  if (port >= MAX_USB_OUT) return;

  HostRumblePortState& host = hostRumble[port];
  expirePendingHostZero(host);
  host.scaledLeft = scaleRumble(host.effectiveLeft);
  host.scaledRight = scaleRumble(host.effectiveRight);

  if (testTargetsPort(port)) {
    rumble_left[port] = testRumble[port].left;
    rumble_right[port] = testRumble[port].right;
    return;
  }

  rumble_left[port] = host.scaledLeft;
  rumble_right[port] = host.scaledRight;
}

void applyAllRumblePorts() {
  for (uint8_t port = 0; port < MAX_USB_OUT; ++port) {
    applyRumblePort(port);
  }
}
}  // namespace

void rumbleRuntimeSetHostFeedback(uint8_t port, uint8_t left, uint8_t right) {
  if (port >= MAX_USB_OUT) return;
  HostRumblePortState& state = hostRumble[port];
  state.rawLeft = left;
  state.rawRight = right;
  state.updateCount++;

  if ((left | right) != 0) {
    state.effectiveLeft = left;
    state.effectiveRight = right;
    state.lastNonzeroMs = millis();
    state.zeroPending = false;
  } else if ((state.effectiveLeft | state.effectiveRight) != 0) {
    if (!state.zeroPending) {
      state.zeroPending = true;
      state.zeroSinceMs = millis();
    }
  } else {
    state.zeroPending = false;
  }

  applyRumblePort(port);
}

void rumbleRuntimeGetEffectiveFeedback(uint8_t port, uint8_t* left, uint8_t* right) {
  uint8_t effectiveLeft = 0;
  uint8_t effectiveRight = 0;

  if (port < MAX_USB_OUT) {
    applyRumblePort(port);
    effectiveLeft = rumble_left[port];
    effectiveRight = rumble_right[port];
  }

  if (effectiveLeft == 0 && effectiveRight == 0) {
    for (uint8_t i = 0; i < MAX_USB_OUT; ++i) {
      applyRumblePort(i);
      if (rumble_left[i] > effectiveLeft) {
        effectiveLeft = rumble_left[i];
      }
      if (rumble_right[i] > effectiveRight) {
        effectiveRight = rumble_right[i];
      }
    }
  }

  if (left) {
    *left = effectiveLeft;
  }
  if (right) {
    *right = effectiveRight;
  }
}

void rumbleRuntimeStartTest(uint32_t portMask, uint8_t left, uint8_t right, uint16_t durationMs) {
  if (durationMs == 0 || portMask == 0 || (left == 0 && right == 0)) {
    rumbleTestStop();
    return;
  }

  testPortMask = portMask;
  for (uint8_t port = 0; port < MAX_USB_OUT; ++port) {
    if (portMaskContains(portMask, port)) {
      testRumble[port].left = left;
      testRumble[port].right = right;
    } else {
      testRumble[port].left = 0;
      testRumble[port].right = 0;
    }
  }
  testEndMs = millis() + durationMs;
  testActive = true;
  applyAllRumblePorts();
}

bool rumbleRuntimeGetPortDiag(uint8_t port, RumbleRuntimePortDiag* out) {
  if (port >= MAX_USB_OUT || out == nullptr) return false;
  applyRumblePort(port);
  const HostRumblePortState& host = hostRumble[port];
  out->raw_left = host.rawLeft;
  out->raw_right = host.rawRight;
  out->scaled_left = host.scaledLeft;
  out->scaled_right = host.scaledRight;
  out->test_left = testRumble[port].left;
  out->test_right = testRumble[port].right;
  out->test_active = testTargetsPort(port) ? 1 : 0;
  out->zero_pending = host.zeroPending ? 1 : 0;
  out->update_count = host.updateCount;
  return true;
}

void rumbleTestStart(uint8_t left, uint8_t right, uint16_t durationMs) {
  rumbleRuntimeStartTest(kAllRumblePortsMask, left, right, durationMs);
}

void rumbleTestStop() {
  for (uint8_t port = 0; port < MAX_USB_OUT; ++port) {
    testRumble[port].left = 0;
    testRumble[port].right = 0;
  }
  testPortMask = 0;
  testEndMs = 0;
  testActive = false;
  applyAllRumblePorts();
}

void rumbleTestUpdate() {
  applyAllRumblePorts();
  if (!testActive) return;

  if ((int32_t)(millis() - testEndMs) >= 0) {
    rumbleTestStop();
    return;
  }
}

bool rumbleTestActive() {
  return testActive;
}
