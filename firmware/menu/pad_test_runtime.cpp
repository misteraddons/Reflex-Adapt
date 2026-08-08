#include "../product_config.h"

#include "pad_test_runtime.h"

#include <Arduino.h>
#include <cstring>

#include "../core/controller_frame_state.h"
#include "../core/device_runtime_state.h"
#include "../core/rumble_test_runtime.h"
#include "../input/shared/input_button_bits.h"
#include "../output/output_runtime_state.h"
#include "rumble_capabilities.h"

namespace {

constexpr uint32_t kNoHostRumblePortMask = 1UL << 0;
constexpr uint16_t kNoHostFixedTestMs = 500;
constexpr uint16_t kNoHostBinaryTestMs = 750;

enum class NoHostRumbleAction : uint8_t {
  None = 0,
  PsxLight,
  PsxHeavy,
  PsxRamp,
  Binary,
  Variable,
};

bool noHostWasActive = false;
NoHostRumbleAction lastRumbleAction = NoHostRumbleAction::None;

bool exactCombo(uint32_t buttons, uint32_t combo) {
  constexpr uint32_t kComboButtons =
    INPUT_A | INPUT_B | INPUT_X | INPUT_Y |
    INPUT_L1 | INPUT_R1 | INPUT_L2 | INPUT_R2 |
    INPUT_START | INPUT_SELECT;
  return (buttons & kComboButtons) == combo;
}

bool isPsxDualShock(const controller_state_t& frame) {
#ifdef ENABLE_INPUT_PSX
  return deviceMode == RZORD_PSX &&
         (std::strcmp(frame.controller_type_name, "DualShock") == 0 ||
          std::strcmp(frame.controller_type_name, "DualShock2") == 0);
#else
  (void)frame;
  return false;
#endif
}

NoHostRumbleAction rumbleActionForFrame(const controller_state_t& frame) {
  const uint32_t buttons = frame.digital_buttons;

  if (isPsxDualShock(frame)) {
    // Physical PSX labels: frame A=Cross and B=Circle.
    if (exactCombo(buttons, INPUT_START | INPUT_A | INPUT_B)) {
      return NoHostRumbleAction::PsxRamp;
    }
    if (exactCombo(buttons, INPUT_START | INPUT_A)) {
      return NoHostRumbleAction::PsxLight;
    }
    if (exactCombo(buttons, INPUT_START | INPUT_B)) {
      return NoHostRumbleAction::PsxHeavy;
    }
    return NoHostRumbleAction::None;
  }

  const RumbleCapabilities capabilities = rumbleCapabilitiesForController(
    deviceMode, frame.connected, frame.controller_type_name);
  if (capabilities.strength == RumbleStrengthSupport::Binary &&
      exactCombo(buttons, INPUT_START | INPUT_A)) {
    return NoHostRumbleAction::Binary;
  }
  if (capabilities.strength == RumbleStrengthSupport::Variable &&
      exactCombo(buttons, INPUT_START | INPUT_A)) {
    return NoHostRumbleAction::Variable;
  }
  return NoHostRumbleAction::None;
}

void startNoHostRumbleAction(NoHostRumbleAction action) {
  switch (action) {
    case NoHostRumbleAction::PsxLight:
      rumbleRuntimeStartTest(
        kNoHostRumblePortMask, 0, 255, kNoHostFixedTestMs);
      break;
    case NoHostRumbleAction::PsxHeavy:
      rumbleRuntimeStartTest(
        kNoHostRumblePortMask, 255, 0, kNoHostFixedTestMs);
      break;
    case NoHostRumbleAction::PsxRamp:
      rumbleRuntimeStartHeavyRamp(kNoHostRumblePortMask);
      break;
    case NoHostRumbleAction::Binary:
      rumbleRuntimeStartTest(
        kNoHostRumblePortMask, 255, 255, kNoHostBinaryTestMs);
      break;
    case NoHostRumbleAction::Variable:
      rumbleRuntimeStartTest(
        kNoHostRumblePortMask, 192, 192, kNoHostFixedTestMs);
      break;
    default:
      break;
  }
}

}  // namespace

bool noHostControllerTestActive() {
  return auto_detect_no_data_host_fallback_active();
}

const char* noHostControllerRumbleHint() {
  if (!noHostControllerTestActive()) {
    return nullptr;
  }
  const controller_state_t& frame = controllerFrameConst(0);
  if (!frame.connected) {
    return nullptr;
  }
  if (isPsxDualShock(frame)) {
    return "Start+X/O = Rumble";
  }
  const RumbleCapabilities capabilities = rumbleCapabilitiesForController(
    deviceMode, frame.connected, frame.controller_type_name);
  return capabilities.strength == RumbleStrengthSupport::None
    ? nullptr
    : "Start+A = Rumble";
}

bool updateNoHostControllerTest() {
  const bool active = noHostControllerTestActive();
  if (!active) {
    if (noHostWasActive) {
      rumbleTestStop();
    }
    noHostWasActive = false;
    lastRumbleAction = NoHostRumbleAction::None;
    return false;
  }

  noHostWasActive = true;
  const controller_state_t& frame = controllerFrameConst(0);
  const NoHostRumbleAction action = frame.connected
    ? rumbleActionForFrame(frame)
    : NoHostRumbleAction::None;
  if (action != NoHostRumbleAction::None && action != lastRumbleAction) {
    startNoHostRumbleAction(action);
  }
  lastRumbleAction = action;
  return true;
}