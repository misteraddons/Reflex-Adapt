#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "firmware/core/controller_frame_state.h"
#include "firmware/core/controller_output_cache_state.h"
#include "firmware/menu/analog_diagnostic.h"
#include "firmware/menu/analog_stick_trace.h"

namespace {

void check(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    std::exit(1);
  }
}

void checkPoint(const AnalogStickTrace& trace, AnalogTraceDirection direction,
                int16_t x, int16_t y, const char* message) {
  const AnalogTracePoint& point = trace.point(direction);
  check(point.valid && point.x == x && point.y == y, message);
}

controller_state_t diagnosticFrame(const char* type, bool main = false,
                                   bool aux = false, bool connected = true) {
  controller_state_t frame{};
  frame.connected = connected;
  frame.HAS_ANALOG_STICK_MAIN = main;
  frame.HAS_ANALOG_STICK_AUX = aux;
  if (type != nullptr) {
    std::strncpy(frame.controller_type_name, type,
                 sizeof(frame.controller_type_name) - 1);
  }
  return frame;
}

void checkKind(DeviceEnum mode, const controller_state_t& frame,
               AnalogDiagnosticKind expected, const char* message) {
  check(analogDiagnosticKindForFrame(mode, frame) == expected, message);
}

void testAnalogDiagnosticClassification() {
  checkKind(RZORD_PSX, diagnosticFrame("DualShock", true, true),
            AnalogDiagnosticKind::Stick, "PSX DualShock should be a stick");
  checkKind(RZORD_PSX, diagnosticFrame("DualShock2", true, true),
            AnalogDiagnosticKind::Stick, "PSX DualShock2 should be a stick");
  checkKind(RZORD_PSX, diagnosticFrame("FlightStick", true, true),
            AnalogDiagnosticKind::Stick, "PSX FlightStick should be a stick");
  checkKind(RZORD_PSX, diagnosticFrame("neGcon", true),
            AnalogDiagnosticKind::NeGcon, "neGcon should use its own test");
  checkKind(RZORD_PSX_JOG, diagnosticFrame("JogCon-P", true),
            AnalogDiagnosticKind::Wheel, "analog JogCon should be a wheel");
  checkKind(RZORD_PSX_JOG, diagnosticFrame("JogCon-S", true),
            AnalogDiagnosticKind::Wheel, "JogCon spinner mode should be a wheel");
  checkKind(RZORD_PSX_JOG, diagnosticFrame("JogCon-W", true),
            AnalogDiagnosticKind::Wheel, "JogCon wheel mode should be a wheel");
  checkKind(RZORD_PSX_JOG, diagnosticFrame("JogCon-Dig"),
            AnalogDiagnosticKind::None, "digital JogCon should be hidden");
  checkKind(RZORD_PSX, diagnosticFrame("Digital", true),
            AnalogDiagnosticKind::None,
            "PSX digital subtype should override a stale stick flag");
  checkKind(RZORD_PSX, diagnosticFrame("GunCon", true),
            AnalogDiagnosticKind::None, "GunCon should not be a stick test");
  checkKind(RZORD_PSX, diagnosticFrame("Fishing", true),
            AnalogDiagnosticKind::None,
            "PSX fishing controller needs a dedicated diagnostic");

  checkKind(RZORD_SATURN, diagnosticFrame("3D Pad", true),
            AnalogDiagnosticKind::Stick, "Saturn 3D Pad should be a stick");
  checkKind(RZORD_SATURN, diagnosticFrame("Mission", true),
            AnalogDiagnosticKind::Stick, "Saturn Mission should be a stick");
  checkKind(RZORD_SATURN, diagnosticFrame("Mission6", true, true),
            AnalogDiagnosticKind::Stick, "Saturn Mission6 should be a stick");
  checkKind(RZORD_SATURN, diagnosticFrame("Wheel", true),
            AnalogDiagnosticKind::Wheel, "Saturn wheel should be a wheel");
  checkKind(RZORD_SATURN, diagnosticFrame("Saturn", true),
            AnalogDiagnosticKind::None,
            "digital Saturn pad should override a stale stick flag");

  const char* dreamcastPads[] = {
    "Pad", "Pad + VMU", "Pad + Rmb", "Pad + Both",
    "Mission", "Mission+VMU",
  };
  for (const char* type : dreamcastPads) {
    checkKind(RZORD_DREAMCAST, diagnosticFrame(type, true, true),
              AnalogDiagnosticKind::Stick,
              "Dreamcast pad subtype should be a stick");
  }
  const char* dreamcastWheels[] = {
    "Wheel", "Whl+VMU", "Whl+Rmb", "Whl+Both",
  };
  for (const char* type : dreamcastWheels) {
    checkKind(RZORD_DREAMCAST, diagnosticFrame(type, true),
              AnalogDiagnosticKind::Wheel,
              "Dreamcast wheel subtype should be a wheel");
  }
  checkKind(RZORD_DREAMCAST, diagnosticFrame("DC Dev ns", true),
            AnalogDiagnosticKind::None,
            "Dreamcast status labels must not expose stale analog state");

  checkKind(RZORD_WII, diagnosticFrame("Classic", true, true),
            AnalogDiagnosticKind::Stick, "Wii Classic should be a stick");
  checkKind(RZORD_WII, diagnosticFrame("ClassicPro", true, true),
            AnalogDiagnosticKind::Stick, "Wii Classic Pro should be a stick");
  checkKind(RZORD_WII, diagnosticFrame("Nunchuk", true),
            AnalogDiagnosticKind::Stick, "Wii Nunchuk should be a stick");
  checkKind(RZORD_WII, diagnosticFrame("Wii", true),
            AnalogDiagnosticKind::None,
            "unresolved Wii subtype should not expose a stick test");
  checkKind(RZORD_WII, diagnosticFrame("Guitar", true, true),
            AnalogDiagnosticKind::None,
            "Wii guitar needs a dedicated diagnostic");

  const char* joybusPads[] = {
    "N64 Pad", "N64 Rumble", "GC Pad", "WaveBird",
  };
  for (const char* type : joybusPads) {
    checkKind(RZORD_N64, diagnosticFrame(type, true, true),
              AnalogDiagnosticKind::Stick,
              "supported Joybus subtype should be a stick");
  }
  checkKind(RZORD_N64, diagnosticFrame("GBA", true),
            AnalogDiagnosticKind::None,
            "GBA should override the Joybus setup stick flag");

  checkKind(RZORD_DRIVING, diagnosticFrame("Driving"),
            AnalogDiagnosticKind::Wheel,
            "Atari driving controller should be a wheel");
  checkKind(RZORD_PADDLE, diagnosticFrame("Paddle"),
            AnalogDiagnosticKind::Paddle,
            "Atari paddle should be a paddle");
  checkKind(RZORD_GAMEPORT, diagnosticFrame("Gameport"),
            AnalogDiagnosticKind::Stick,
            "gameport axes should be a stick diagnostic");
  checkKind(RZORD_SMS, diagnosticFrame("Driving", true),
            AnalogDiagnosticKind::Wheel,
            "SMS driving fallback should use the wheel diagnostic");
  checkKind(RZORD_VBOY, diagnosticFrame("VB Pad", false, true),
            AnalogDiagnosticKind::None,
            "Virtual Boy right d-pad must not be treated as an analog stick");
  checkKind(RZORD_CUSTOM, diagnosticFrame("Custom", false, true),
            AnalogDiagnosticKind::Stick,
            "generic auxiliary-only analog input should be a stick");
  checkKind(RZORD_CUSTOM, diagnosticFrame("Custom"),
            AnalogDiagnosticKind::None,
            "generic digital-only input should be hidden");
  checkKind(RZORD_PSX, diagnosticFrame("DualShock", true, true, false),
            AnalogDiagnosticKind::None,
            "disconnected analog subtype should be hidden");
}

void testAnalogDiagnosticTargetSelection() {
  controller_state_t frames[MAX_USB_OUT] = {};
  frames[0] = diagnosticFrame("Digital", true);
  frames[1] = diagnosticFrame("DualShock", true, true);
  frames[2] = diagnosticFrame("Digital");
  frames[3] = diagnosticFrame("neGcon", true);

  AnalogDiagnosticTarget target =
      analogDiagnosticDefaultTarget(RZORD_PSX, frames, MAX_USB_OUT);
  check(analogDiagnosticTargetIsValid(target) && target.port == 1 &&
        target.kind == AnalogDiagnosticKind::Stick &&
        target.stick == AnalogDiagnosticStick::Main,
        "default target should skip digital and disconnected ports");

  target = analogDiagnosticNextStickTarget(
      RZORD_PSX, frames, MAX_USB_OUT, target);
  check(target.port == 1 && target.stick == AnalogDiagnosticStick::Aux,
        "next stick should select the auxiliary stick");
  target = analogDiagnosticNextStickTarget(
      RZORD_PSX, frames, MAX_USB_OUT, target);
  check(target.port == 1 && target.stick == AnalogDiagnosticStick::Main,
        "next stick should wrap to the main stick");

  target = analogDiagnosticNextPortTarget(
      RZORD_PSX, frames, MAX_USB_OUT, target);
  check(target.port == 3 && target.kind == AnalogDiagnosticKind::NeGcon,
        "next port should skip ports without a supported analog diagnostic");
  target = analogDiagnosticNextPortTarget(
      RZORD_PSX, frames, MAX_USB_OUT, target);
  check(target.port == 1 && target.kind == AnalogDiagnosticKind::Stick,
        "next port should wrap");
  target = analogDiagnosticNextPortTarget(
      RZORD_PSX, frames, MAX_USB_OUT, target, -1);
  check(target.port == 3 && target.kind == AnalogDiagnosticKind::NeGcon,
        "previous port should wrap in reverse");

  AnalogDiagnosticTarget specialty =
      analogDiagnosticNextStickTarget(RZORD_PSX, frames, MAX_USB_OUT, target);
  check(specialty.port == 3 &&
        specialty.kind == AnalogDiagnosticKind::NeGcon &&
        specialty.stick == AnalogDiagnosticStick::Main,
        "specialty diagnostic should ignore stick selection");

  frames[1] = {};
  frames[3] = {};
  target = analogDiagnosticDefaultTarget(RZORD_PSX, frames, MAX_USB_OUT);
  check(!analogDiagnosticTargetIsValid(target) &&
        target.port == kAnalogDiagnosticNoPort,
        "empty frame set should have no target");

  frames[0] = {};
  frames[2] = {};
  frames[4] = diagnosticFrame("Custom", false, true);
  target = analogDiagnosticDefaultTarget(RZORD_CUSTOM, frames, MAX_USB_OUT);
  check(target.port == 4 && target.stick == AnalogDiagnosticStick::Aux,
        "auxiliary-only controller should default to its available stick");

  target = analogDiagnosticDefaultTarget(RZORD_CUSTOM, frames, 4);
  check(!analogDiagnosticTargetIsValid(target),
        "selection must respect the live frame count");
  target = analogDiagnosticDefaultTarget(
      RZORD_CUSTOM, nullptr, MAX_USB_OUT);
  check(!analogDiagnosticTargetIsValid(target),
        "null frame collection should have no target");

  frames[4] = {};
  frames[2] = diagnosticFrame("Gameport");
  target = analogDiagnosticDefaultTarget(
      RZORD_GAMEPORT, frames, MAX_USB_OUT);
  check(target.port == 2 && target.kind == AnalogDiagnosticKind::Stick &&
        target.stick == AnalogDiagnosticStick::Main,
        "gameport should expose its implicit main stick");
}

void testGatePolicyByControllerType() {
  check(analogTraceUsesOctagonalGate(RZORD_N64, "N64"),
        "N64 should use an octagonal trace");
  check(analogTraceUsesOctagonalGate(RZORD_GAMECUBE, "GameCube"),
        "GameCube should use an octagonal trace");
  check(analogTraceUsesOctagonalGate(RZORD_WII, "Classic"),
        "Wii Classic should use an octagonal trace");
  check(analogTraceUsesOctagonalGate(RZORD_WII, "ClassicPro"),
        "Wii Classic Pro should use an octagonal trace");

  check(!analogTraceUsesOctagonalGate(RZORD_WII, "Nunchuk"),
        "Wii Nunchuk should remain round");
  check(!analogTraceUsesOctagonalGate(RZORD_WII, "Guitar"),
        "Wii guitar should remain round");
  check(!analogTraceUsesOctagonalGate(RZORD_WII, "Wii"),
        "Wii generic startup state should remain round");
  check(!analogTraceUsesOctagonalGate(RZORD_WII, nullptr),
        "Wii unknown controller state should remain round");
  check(!analogTraceUsesOctagonalGate(RZORD_SATURN, "3D Pad"),
        "Saturn 3D Pad should use a round trace");
  check(!analogTraceUsesOctagonalGate(RZORD_DREAMCAST, "Dreamcast"),
        "Dreamcast controllers should use a round trace");
  check(!analogTraceUsesOctagonalGate(RZORD_PSX, "DualShock"),
        "PlayStation controllers should use a round trace");
}

void testOctagonalDirectionalCapture() {
  AnalogStickTrace trace;
  trace.sample(2, -3, true, 7);
  trace.sample(0, -85, true, 7);
  check(!trace.point(AnalogTraceDirection::UpRight).valid &&
        !trace.point(AnalogTraceDirection::UpLeft).valid,
        "an up sample must not populate diagonal maxima");
  trace.sample(70, -70, true, 7);
  trace.sample(85, 0, true, 7);
  trace.sample(70, 70, true, 7);
  trace.sample(0, 85, true, 7);
  trace.sample(-70, 70, true, 7);
  trace.sample(-85, 0, true, 7);
  trace.sample(-70, -70, true, 7);
  trace.sample(30, -30, true, 7);

  checkPoint(trace, AnalogTraceDirection::Up, 0, -85, "octagon up");
  checkPoint(trace, AnalogTraceDirection::UpRight, 70, -70, "octagon up-right");
  checkPoint(trace, AnalogTraceDirection::Right, 85, 0, "octagon right");
  checkPoint(trace, AnalogTraceDirection::DownRight, 70, 70, "octagon down-right");
  checkPoint(trace, AnalogTraceDirection::Down, 0, 85, "octagon down");
  checkPoint(trace, AnalogTraceDirection::DownLeft, -70, 70, "octagon down-left");
  checkPoint(trace, AnalogTraceDirection::Left, -85, 0, "octagon left");
  checkPoint(trace, AnalogTraceDirection::UpLeft, -70, -70, "octagon up-left");
}

void testOctagonalSectorBoundaries() {
  AnalogStickTrace trace;
  trace.sample(100, 41, true, 0);
  checkPoint(trace, AnalogTraceDirection::Right, 100, 41,
             "41 percent slope should stay cardinal");
  trace.reset();
  trace.sample(100, 42, true, 0);
  checkPoint(trace, AnalogTraceDirection::DownRight, 100, 42,
             "42 percent slope should enter diagonal sector");
  trace.reset();
  trace.sample(41, 100, true, 0);
  checkPoint(trace, AnalogTraceDirection::Down, 41, 100,
             "inverse 41 percent slope should stay cardinal");
  trace.reset();
  trace.sample(42, 100, true, 0);
  checkPoint(trace, AnalogTraceDirection::DownRight, 42, 100,
             "inverse 42 percent slope should enter diagonal sector");
}

void testRoundCaptureUsesCardinalExtrema() {
  AnalogStickTrace trace;
  trace.sample(90, -90, false, 7);
  trace.sample(0, -127, false, 7);
  trace.sample(127, 0, false, 7);
  trace.sample(0, 127, false, 7);
  trace.sample(-127, 0, false, 7);

  checkPoint(trace, AnalogTraceDirection::Up, 0, -127, "round up");
  checkPoint(trace, AnalogTraceDirection::Right, 127, 0, "round right");
  checkPoint(trace, AnalogTraceDirection::Down, 0, 127, "round down");
  checkPoint(trace, AnalogTraceDirection::Left, -127, 0, "round left");
  check(!trace.point(AnalogTraceDirection::UpRight).valid,
        "round capture must not populate diagonal extrema");
}

void testDirectionalCaptureKeepsFarthestSample() {
  AnalogStickTrace trace;
  trace.sample(0, -80, true, 7);
  trace.sample(0, -85, true, 7);
  trace.sample(0, -82, true, 7);
  checkPoint(trace, AnalogTraceDirection::Up, 0, -85,
             "directional capture should keep the farthest sample");
}

void testInt16ProjectionAndReset() {
  AnalogStickTrace trace;
  trace.sample(INT16_MIN, INT16_MIN, true, 100);
  checkPoint(trace, AnalogTraceDirection::UpLeft, INT16_MIN, INT16_MIN,
             "int16 projection must not overflow");
  trace.reset();
  for (uint8_t i = 0; i < AnalogStickTrace::directionCount(true); ++i) {
    check(!trace.point(AnalogStickTrace::orderedDirection(true, i)).valid,
          "reset must clear every direction");
  }
}

void testRawSnapshotPreTransformSemantics() {
  max_devices = 2;
  controller_frames[0] = {};
  controller_frames[0].connected = true;
  controller_frames[0].HAS_ANALOG_STICK_MAIN = true;
  controller_frames[0].HAS_ANALOG_STICK_AUX = true;
  controller_frames[0].sticks_precision_bits = ANALOG_STICK_PRECISION_8;
  controller_frames[0].LX = -101;
  controller_frames[0].LY = 102;
  controller_frames[0].RX = -103;
  controller_frames[0].RY = 104;

  captureRawAnalogInputSnapshots(RZORD_PSX);
  controller_frames[0].LX = 0;
  controller_frames[0].LY = 0;
  controller_frames[0].RX = 0;
  controller_frames[0].RY = 0;

  RawAnalogInputSnapshot snapshot{};
  check(getRawAnalogInputSnapshot(RZORD_PSX, 0, snapshot),
        "captured PSX snapshot should be available");
  check(snapshot.lx == -101 && snapshot.ly == 102 &&
        snapshot.rx == -103 && snapshot.ry == 104,
        "snapshot must survive later frame transforms");
  check(snapshot.has_main_stick && snapshot.has_aux_stick &&
        snapshot.precision == ANALOG_STICK_PRECISION_8,
        "snapshot must retain stick capabilities and precision");
  check(!getRawAnalogInputSnapshot(RZORD_N64, 0, snapshot),
        "mode mismatch must reject stale axes");

  controller_frames[0].connected = false;
  captureRawAnalogInputSnapshots(RZORD_PSX);
  check(!getRawAnalogInputSnapshot(RZORD_PSX, 0, snapshot),
        "disconnect must invalidate raw axes");

  controller_frames[1] = {};
  controller_frames[1].connected = true;
  controller_frames[1].LX = 55;
  max_devices = 2;
  captureRawAnalogInputSnapshots(RZORD_PSX);
  check(getRawAnalogInputSnapshot(RZORD_PSX, 1, snapshot),
        "active secondary port should be captured");
  max_devices = 1;
  captureRawAnalogInputSnapshots(RZORD_PSX);
  check(!getRawAnalogInputSnapshot(RZORD_PSX, 1, snapshot),
        "unused ports must be cleared on every capture");
}

}  // namespace

int main() {
  testAnalogDiagnosticClassification();
  testAnalogDiagnosticTargetSelection();
  testGatePolicyByControllerType();
  testOctagonalDirectionalCapture();
  testOctagonalSectorBoundaries();
  testRoundCaptureUsesCardinalExtrema();
  testDirectionalCaptureKeepsFarthestSample();
  testInt16ProjectionAndReset();
  testRawSnapshotPreTransformSemantics();
  std::cout << "OK: analog stick trace tests passed\n";
  return 0;
}
