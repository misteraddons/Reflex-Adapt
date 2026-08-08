#include "../product_config.h"

#include "menu_analog_test.h"

#include <string.h>

#include "../core/classic_analog_range.h"
#include "../core/controller_frame_state.h"
#include "../core/controller_output_cache_state.h"
#include "../core/device_runtime_state.h"
#include "../platform/buzzer.h"
#include "analog_diagnostic.h"
#include "analog_test_renderer.h"
#include "menu_ui_state.h"

namespace {

AnalogDiagnosticTarget analogTestTarget = analogDiagnosticNoTarget();
AnalogStickTrace analogTestTrace[2];
DeviceEnum analogTestMode = RZORD_NONE;
AnalogDiagnosticKind analogTestKind = AnalogDiagnosticKind::None;
char analogTestTypeName[
  sizeof(((controller_state_t*)0)->controller_type_name)] = {};
bool analogTestOctagonal = false;
bool analogTestDirty = false;
uint32_t analogTestLastDrawMs = 0;
int16_t analogTestLastX = 0;
int16_t analogTestLastY = 0;
bool analogTestPrevUp = false;
bool analogTestPrevDown = false;
bool analogTestPrevLeft = false;
bool analogTestPrevRight = false;

uint8_t analogTestFrameCount() {
  return max_devices < MAX_USB_OUT ? max_devices : MAX_USB_OUT;
}

uint8_t analogTestTargetCount(DeviceEnum mode) {
  uint8_t count = 0;
  const uint8_t frameCount = analogTestFrameCount();
  for (uint8_t port = 0; port < frameCount; ++port) {
    if (analogDiagnosticTargetIsValid(
          analogDiagnosticTargetForPort(
            mode, controller_frames, frameCount, port))) {
      ++count;
    }
  }
  return count;
}

bool analogTestIsGameCube(DeviceEnum mode) {
#ifdef ENABLE_INPUT_GAMECUBE
  return mode == RZORD_GAMECUBE;
#else
  (void)mode;
  return false;
#endif
}

void resetAnalogTestTraceState(const AnalogDiagnosticTarget& target) {
  analogTestTarget = target;
  analogTestTrace[0].reset();
  analogTestTrace[1].reset();
  analogTestMode = deviceMode;
  analogTestKind = target.kind;
  analogTestOctagonal = false;
  analogTestTypeName[0] = '\0';
  if (analogDiagnosticTargetIsValid(target)) {
    const controller_state_t& frame = controllerFrameConst(target.port);
    strncpy(analogTestTypeName, frame.controller_type_name,
            sizeof(analogTestTypeName) - 1);
    analogTestTypeName[sizeof(analogTestTypeName) - 1] = '\0';
    analogTestOctagonal = analogTraceUsesOctagonalGate(
      deviceMode, frame.controller_type_name);
    analogTestPrevUp = frame.PAD_U;
    analogTestPrevDown = frame.PAD_D;
    analogTestPrevLeft = frame.PAD_L;
    analogTestPrevRight = frame.PAD_R;
  } else {
    analogTestPrevUp = false;
    analogTestPrevDown = false;
    analogTestPrevLeft = false;
    analogTestPrevRight = false;
  }
  analogTestDirty = true;
  analogTestLastDrawMs = 0;
  analogTestLastX = 0;
  analogTestLastY = 0;
}

bool analogTestContextChanged(const AnalogDiagnosticTarget& target) {
  if (!analogDiagnosticTargetIsValid(target)) {
    return analogDiagnosticTargetIsValid(analogTestTarget);
  }
  const controller_state_t& frame = controllerFrameConst(target.port);
  return !analogDiagnosticTargetIsValid(analogTestTarget) ||
         analogTestMode != deviceMode ||
         analogTestTarget.port != target.port ||
         analogTestKind != target.kind ||
         analogTestOctagonal != analogTraceUsesOctagonalGate(
           deviceMode, frame.controller_type_name) ||
         strncmp(analogTestTypeName, frame.controller_type_name,
                 sizeof(analogTestTypeName)) != 0;
}

void exitAnalogTest() {
  analogTestActive = false;
  analogTestInitialized = false;
  resetAnalogTestTraceState(analogDiagnosticNoTarget());
}

AnalogDiagnosticTarget refreshAnalogTestTarget() {
  const uint8_t frameCount = analogTestFrameCount();
  AnalogDiagnosticTarget target = analogDiagnosticTargetForPort(
    deviceMode, controller_frames, frameCount, analogTestTarget.port,
    analogTestTarget.stick);
  if (!analogDiagnosticTargetIsValid(target)) {
    target = analogDiagnosticDefaultTarget(
      deviceMode, controller_frames, frameCount);
  }
  if (analogTestContextChanged(target)) {
    resetAnalogTestTraceState(target);
  } else if (analogDiagnosticTargetIsValid(target) &&
             target.stick != analogTestTarget.stick) {
    analogTestTarget.stick = target.stick;
    analogTestDirty = true;
  }
  return analogTestTarget;
}

void handleAnalogTestNavigation(const controller_state_t& frame) {
  const bool upJust = frame.PAD_U && !analogTestPrevUp;
  const bool downJust = frame.PAD_D && !analogTestPrevDown;
  const bool leftJust = frame.PAD_L && !analogTestPrevLeft;
  const bool rightJust = frame.PAD_R && !analogTestPrevRight;
  analogTestPrevUp = frame.PAD_U;
  analogTestPrevDown = frame.PAD_D;
  analogTestPrevLeft = frame.PAD_L;
  analogTestPrevRight = frame.PAD_R;

  AnalogDiagnosticTarget next = analogTestTarget;
  const uint8_t frameCount = analogTestFrameCount();
  if (upJust || downJust) {
    next = analogDiagnosticNextPortTarget(
      deviceMode, controller_frames, frameCount, analogTestTarget,
      upJust ? -1 : 1);
  } else if (leftJust || rightJust) {
    next = analogDiagnosticNextStickTarget(
      deviceMode, controller_frames, frameCount, analogTestTarget);
  }

  if (analogDiagnosticTargetIsValid(next) &&
      (next.port != analogTestTarget.port ||
       next.stick != analogTestTarget.stick ||
       next.kind != analogTestTarget.kind)) {
    if (next.port != analogTestTarget.port || next.kind != analogTestTarget.kind) {
      resetAnalogTestTraceState(next);
    } else {
      analogTestTarget = next;
      analogTestDirty = true;
    }
    buzzer.playMenuNav();
  }
}

void readAnalogStickRawValues(const AnalogDiagnosticTarget& target,
                              int16_t& x, int16_t& y,
                              analog_stick_precision& precision,
                              bool& hasMain, bool& hasAux) {
  const controller_state_t& frame = controllerFrameConst(target.port);
  x = target.stick == AnalogDiagnosticStick::Aux ? frame.RX : frame.LX;
  y = target.stick == AnalogDiagnosticStick::Aux ? frame.RY : frame.LY;
  precision = frame.sticks_precision_bits;
  hasMain = frame.HAS_ANALOG_STICK_MAIN;
  hasAux = frame.HAS_ANALOG_STICK_AUX;

  RawAnalogInputSnapshot raw{};
  if (getRawAnalogInputSnapshot(deviceMode, target.port, raw)) {
    x = target.stick == AnalogDiagnosticStick::Aux ? raw.rx : raw.lx;
    y = target.stick == AnalogDiagnosticStick::Aux ? raw.ry : raw.ly;
    precision = raw.precision;
    hasMain = raw.has_main_stick;
    hasAux = raw.has_aux_stick;
  }

  ClassicAnalogRangeSnapshot range{};
  if (getClassicAnalogRangeSnapshot(deviceMode, target.port, range)) {
    const ClassicAnalogAxis xAxis =
      target.stick == AnalogDiagnosticStick::Aux
        ? CLASSIC_ANALOG_AXIS_RX : CLASSIC_ANALOG_AXIS_LX;
    const ClassicAnalogAxis yAxis =
      target.stick == AnalogDiagnosticStick::Aux
        ? CLASSIC_ANALOG_AXIS_RY : CLASSIC_ANALOG_AXIS_LY;
    if (range.valid[xAxis]) {
      x = range.raw[xAxis];
      precision = ANALOG_STICK_PRECISION_8;
    }
    if (range.valid[yAxis]) {
      y = range.raw[yAxis];
      precision = ANALOG_STICK_PRECISION_8;
    }
  }
}

void renderAnalogStickTarget(const AnalogDiagnosticTarget& target,
                             bool multiplePorts) {
  const controller_state_t& frame = controllerFrameConst(target.port);
  int16_t x = 0;
  int16_t y = 0;
  analog_stick_precision precision = ANALOG_STICK_PRECISION_8;
  bool hasMain = false;
  bool hasAux = false;
  readAnalogStickRawValues(
    target, x, y, precision, hasMain, hasAux);

  const bool octagonal = analogTraceUsesOctagonalGate(
    deviceMode, frame.controller_type_name);
  const int16_t threshold =
    analogTraceCenterThreshold(analogTraceFullScale(precision));
  const uint8_t stickIndex =
    target.stick == AnalogDiagnosticStick::Aux ? 1 : 0;
  analogTestDirty =
    analogTestTrace[stickIndex].sample(
      x, y, octagonal, threshold) ||
    analogTestDirty ||
    x != analogTestLastX || y != analogTestLastY;
  analogTestLastX = x;
  analogTestLastY = y;

  const uint32_t now = millis();
  if (!analogTestDirty ||
      (analogTestLastDrawMs != 0 &&
       (uint32_t)(now - analogTestLastDrawMs) <
         ANALOG_TEST_FRAME_INTERVAL_MS)) {
    return;
  }

  const AnalogStickTraceScreen screen{
    &analogTestTrace[stickIndex],
    octagonal,
    x,
    y,
    precision,
    frame.connected,
    target.stick == AnalogDiagnosticStick::Aux,
    analogTestIsGameCube(deviceMode),
    target.port,
    multiplePorts,
    hasMain && hasAux,
  };
  renderAnalogStickTraceScreen(screen);
  analogTestDirty = false;
  analogTestLastDrawMs = now;
}

void renderAnalogSpecialTarget(const AnalogDiagnosticTarget& target,
                               bool multiplePorts) {
  const controller_state_t& frame = controllerFrameConst(target.port);
  AnalogTestValue values[5] = {};
  uint8_t valueCount = 0;
  const char* title = "Analog Test";

  switch (target.kind) {
    case AnalogDiagnosticKind::Wheel:
      title = "Wheel Test";
      values[valueCount++] = { "Raw", frame.paddle };
      values[valueCount++] = {
        "Centered", (int16_t)((int16_t)frame.paddle - 0x80)
      };
      values[valueCount++] = { "Axis", frame.LX };
      break;
    case AnalogDiagnosticKind::NeGcon:
      title = "neGcon Test";
      values[valueCount++] = { "Twist", frame.LX };
      values[valueCount++] = { "I", frame.ANALOG_A };
      values[valueCount++] = { "II", frame.ANALOG_X };
      values[valueCount++] = { "L", frame.ANALOG_L1 };
      break;
    case AnalogDiagnosticKind::Paddle:
      title = "Paddle Test";
      values[valueCount++] = { "Position", frame.LX };
      values[valueCount++] = {
        "Button", (int16_t)(frame.A ? 1 : 0)
      };
      break;
    default:
      return;
  }

  analogTestDirty = true;

  const uint32_t now = millis();
  if (!analogTestDirty ||
      (analogTestLastDrawMs != 0 &&
       (uint32_t)(now - analogTestLastDrawMs) <
         ANALOG_TEST_FRAME_INTERVAL_MS)) {
    return;
  }
  renderAnalogValueTestScreen(
    title, target.port, values, valueCount, multiplePorts);
  analogTestDirty = false;
  analogTestLastDrawMs = now;
}

}  // namespace

void renderAnalogTest(bool modeBtnJustPressed) {
#ifndef USE_I2C_DISPLAY
  (void)modeBtnJustPressed;
  exitAnalogTest();
  return;
#else
  if (modeBtnJustPressed) {
    buzzer.playMenuNav();
    exitAnalogTest();
    return;
  }

  if (!analogTestInitialized) {
    analogTestInitialized = true;
    resetAnalogTestTraceState(
      analogDiagnosticDefaultTarget(
        deviceMode, controller_frames, analogTestFrameCount()));
  }

  AnalogDiagnosticTarget target = refreshAnalogTestTarget();
  if (!analogDiagnosticTargetIsValid(target)) {
    exitAnalogTest();
    return;
  }

  const controller_state_t& frame = controllerFrameConst(target.port);
  handleAnalogTestNavigation(frame);
  target = refreshAnalogTestTarget();
  if (!analogDiagnosticTargetIsValid(target)) {
    exitAnalogTest();
    return;
  }

  const bool multiplePorts = analogTestTargetCount(deviceMode) > 1;
  if (target.kind == AnalogDiagnosticKind::Stick) {
    renderAnalogStickTarget(target, multiplePorts);
  } else {
    renderAnalogSpecialTarget(target, multiplePorts);
  }
#endif
}
