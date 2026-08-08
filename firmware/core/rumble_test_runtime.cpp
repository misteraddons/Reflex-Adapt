#include "rumble_test_runtime.h"

#include <Arduino.h>

#include "../firmware_platform_config.h"
#include "../output/output_capabilities.h"
#include "controller_settings_state.h"

namespace {
constexpr uint16_t kHostLightMinimumPulseMs = 40;
constexpr uint32_t kAllRumblePortsMask = 0xFFFFFFFFu;
constexpr uint16_t kHeavyRampStepMs = 500;
constexpr uint8_t kHeavyRampStepCount = 17;

struct HostRumblePortState {
  uint8_t rawLeft = 0;
  uint8_t rawRight = 0;
  uint8_t effectiveLeft = 0;
  uint8_t effectiveRight = 0;
  uint8_t scaledLeft = 0;
  uint8_t scaledRight = 0;
  uint32_t updateCount = 0;
  uint32_t lightPulseUntilMs = 0;
  bool lightPulsePending = false;
};

struct TestRumblePortState {
  uint8_t left = 0;
  uint8_t right = 0;
};

HostRumblePortState hostRumble[MAX_USB_OUT] = {};
TestRumblePortState testRumble[MAX_USB_OUT] = {};
uint32_t testPortMask = 0;
uint32_t testEndMs = 0;
uint32_t testStartMs = 0;
bool testActive = false;
bool testHeavyRamp = false;
uint8_t testHeavyRampLevel = 0;
bool hostFeedbackSuppressed = false;

uint8_t scaleRumble(uint8_t value) {
  if (rumble_level == RUMBLE_MODE_OFF || value == 0) return 0;
  if (rumble_level == RUMBLE_MODE_MAX) return 255;
  return value;
}

bool portMaskContains(uint32_t mask, uint8_t port) {
  return port < 32 && ((mask & (1UL << port)) != 0);
}

bool testTargetsPort(uint8_t port) {
  return testActive && portMaskContains(testPortMask, port);
}

void expireShortLightPulse(HostRumblePortState& state) {
  if (!state.lightPulsePending ||
      (int32_t)(millis() - state.lightPulseUntilMs) < 0) return;
  state.effectiveRight = 0;
  state.lightPulseUntilMs = 0;
  state.lightPulsePending = false;
}

void applyRumblePort(uint8_t port) {
  if (port >= MAX_USB_OUT) return;

  HostRumblePortState& host = hostRumble[port];
  expireShortLightPulse(host);
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

void rumbleRuntimeSetHostFeedback(uint8_t port, uint8_t left, uint8_t right,
                                  bool extendShortLightPulse) {
  if (port >= MAX_USB_OUT) return;
  HostRumblePortState& state = hostRumble[port];
  state.rawLeft = left;
  state.rawRight = right;
  state.updateCount++;

  if (hostFeedbackSuppressed) {
    state.effectiveLeft = 0;
    state.effectiveRight = 0;
    state.lightPulsePending = false;
    applyRumblePort(port);
    return;
  }

  state.effectiveLeft = left;
  if (right != 0) {
    state.effectiveRight = right;
    state.lightPulseUntilMs = millis() + kHostLightMinimumPulseMs;
    state.lightPulsePending = false;
  } else if (extendShortLightPulse && state.effectiveRight != 0 &&
             (int32_t)(millis() - state.lightPulseUntilMs) < 0) {
    // MiSTer DInput and XInput can emit extremely short right-small pulses.
    // Preserve only the
    // remainder of a 40 ms perceptible pulse; never extend repeated zeros.
    state.lightPulsePending = true;
  } else {
    state.effectiveRight = 0;
    state.lightPulseUntilMs = 0;
    state.lightPulsePending = false;
  }

  applyRumblePort(port);
}

void rumbleRuntimeSetHostFeedbackSuppressed(bool suppressed) {
  if (hostFeedbackSuppressed == suppressed) return;
  hostFeedbackSuppressed = suppressed;
  if (suppressed) {
    rumbleRuntimeClearAllHostFeedback();
  }
}

void rumbleRuntimeClearAllHostFeedback() {
  for (uint8_t port = 0; port < MAX_USB_OUT; ++port) {
    hostRumble[port] = HostRumblePortState{};
  }
  applyAllRumblePorts();
}

void rumbleRuntimeGetEffectiveFeedback(uint8_t port, uint8_t* left, uint8_t* right) {
  uint8_t effectiveLeft = 0;
  uint8_t effectiveRight = 0;

  if (port < MAX_USB_OUT) {
    applyRumblePort(port);
    effectiveLeft = rumble_left[port];
    effectiveRight = rumble_right[port];
  }

  // A single USB endpoint may represent a controller connected to a nonzero
  // physical input port, so retain the legacy any-port fallback there. Every
  // multi-player output has explicit endpoint-to-source routing; falling back
  // across ports would mirror one player's rumble onto every other pad.
  if (effectiveLeft == 0 && effectiveRight == 0 &&
      !output_runtime_has_secondary_player_slot()) {
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
  testStartMs = 0;
  testHeavyRamp = false;
  testActive = true;
  applyAllRumblePorts();
}

void rumbleRuntimeStartHeavyRamp(uint32_t portMask) {
  if (portMask == 0) {
    rumbleTestStop();
    return;
  }

  testPortMask = portMask;
  for (uint8_t port = 0; port < MAX_USB_OUT; ++port) {
    testRumble[port].left = 0;
    testRumble[port].right = 0;
  }

  testStartMs = millis();
  testEndMs = testStartMs +
    (static_cast<uint32_t>(kHeavyRampStepCount) * kHeavyRampStepMs);
  testHeavyRampLevel = 0;
  testHeavyRamp = true;
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
  out->update_count = host.updateCount;
  return true;
}

void rumbleTestStart(uint8_t left, uint8_t right, uint16_t durationMs) {
  rumbleRuntimeStartTest(kAllRumblePortsMask, left, right, durationMs);
}

void rumbleTestStartHeavyRamp() {
  rumbleRuntimeStartHeavyRamp(kAllRumblePortsMask);
}

void rumbleTestStop() {
  for (uint8_t port = 0; port < MAX_USB_OUT; ++port) {
    testRumble[port].left = 0;
    testRumble[port].right = 0;
  }
  testPortMask = 0;
  testEndMs = 0;
  testStartMs = 0;
  testHeavyRamp = false;
  testHeavyRampLevel = 0;
  testActive = false;
  applyAllRumblePorts();
}

void rumbleTestUpdate() {
  if (!testActive) {
    applyAllRumblePorts();
    return;
  }

  if ((int32_t)(millis() - testEndMs) >= 0) {
    rumbleTestStop();
    return;
  }

  if (testHeavyRamp) {
    const uint32_t elapsedMs = millis() - testStartMs;
    const uint32_t step = elapsedMs / kHeavyRampStepMs;
    const uint8_t level = (step >= 16u)
      ? 255u
      : static_cast<uint8_t>(step * 16u);
    testHeavyRampLevel = level;
    for (uint8_t port = 0; port < MAX_USB_OUT; ++port) {
      if (portMaskContains(testPortMask, port)) {
        testRumble[port].left = level;
        testRumble[port].right = 0;
      }
    }
  }
  applyAllRumblePorts();
}

bool rumbleTestActive() {
  return testActive;
}

bool rumbleTestGetHeavyRampLevel(uint8_t* level) {
  if (!testActive || !testHeavyRamp) return false;
  if (level != nullptr) {
    *level = testHeavyRampLevel;
  }
  return true;
}
