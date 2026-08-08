#include <cstdint>
#include <cstdlib>
#include <iostream>

#include "firmware/menu/rumble_capabilities.h"

namespace {

void check(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    std::exit(1);
  }
}

void checkCapabilities(
    RumbleCapabilities actual,
    RumbleStrengthSupport strength,
    uint8_t motors,
    const char* message) {
  check(actual.strength == strength && actual.motor_count == motors, message);
}

void testPotentialModeCapabilities() {
  checkCapabilities(
    potentialRumbleCapabilitiesForMode(RZORD_PSX),
    RumbleStrengthSupport::Variable,
    2,
    "PSX must allow DualShock variable strength"
  );
  checkCapabilities(
    potentialRumbleCapabilitiesForMode(RZORD_SNES),
    RumbleStrengthSupport::Variable,
    2,
    "SNES must allow RumbleTech variable strength"
  );
  checkCapabilities(
    potentialRumbleCapabilitiesForMode(RZORD_N64),
    RumbleStrengthSupport::Binary,
    1,
    "N64 Rumble Pak must be binary"
  );
  checkCapabilities(
    potentialRumbleCapabilitiesForMode(RZORD_GAMECUBE),
    RumbleStrengthSupport::Binary,
    1,
    "GameCube rumble must be binary"
  );
  checkCapabilities(
    potentialRumbleCapabilitiesForMode(RZORD_DREAMCAST),
    RumbleStrengthSupport::None,
    0,
    "Dreamcast rumble must remain hidden until commands are implemented"
  );
  checkCapabilities(
    potentialRumbleCapabilitiesForMode(RZORD_PSX_JOG),
    RumbleStrengthSupport::None,
    0,
    "JogCon force must not use the generic rumble setting"
  );
}

void testDetectedControllerCapabilities() {
  checkCapabilities(
    rumbleCapabilitiesForController(RZORD_PSX, true, "DualShock"),
    RumbleStrengthSupport::Variable,
    2,
    "DualShock must expose variable strength"
  );
  checkCapabilities(
    rumbleCapabilitiesForController(RZORD_PSX, true, "DualShock2"),
    RumbleStrengthSupport::Variable,
    2,
    "DualShock2 must expose variable strength"
  );
  checkCapabilities(
    rumbleCapabilitiesForController(RZORD_PSX, true, "Digital"),
    RumbleStrengthSupport::None,
    0,
    "digital PSX pads must not expose rumble"
  );
  checkCapabilities(
    rumbleCapabilitiesForController(RZORD_PSX, true, "JogCon"),
    RumbleStrengthSupport::None,
    0,
    "JogCon must use its separate force setting"
  );
  checkCapabilities(
    rumbleCapabilitiesForController(RZORD_SNES, true, "SNES Pad"),
    RumbleStrengthSupport::Variable,
    2,
    "SNES must remain RumbleTech-compatible because it has no unique ID"
  );
  checkCapabilities(
    rumbleCapabilitiesForController(RZORD_SNES, true, "SNES Mouse"),
    RumbleStrengthSupport::None,
    0,
    "SNES Mouse must not expose RumbleTech settings"
  );
  checkCapabilities(
    rumbleCapabilitiesForController(RZORD_SNES, true, "Multitap"),
    RumbleStrengthSupport::None,
    0,
    "SNES multitap must not expose RumbleTech settings"
  );
  checkCapabilities(
    rumbleCapabilitiesForController(RZORD_SNES, true, "NTT Data"),
    RumbleStrengthSupport::None,
    0,
    "NTT Data controller must not expose RumbleTech settings"
  );
  checkCapabilities(
    rumbleCapabilitiesForController(RZORD_N64, true, "N64 Rumble"),
    RumbleStrengthSupport::Binary,
    1,
    "N64 Rumble Pak must expose only on/off"
  );
  checkCapabilities(
    rumbleCapabilitiesForController(RZORD_N64, true, "N64 Pad"),
    RumbleStrengthSupport::None,
    0,
    "N64 pads without a Rumble Pak must hide rumble"
  );
  checkCapabilities(
    rumbleCapabilitiesForController(RZORD_GAMECUBE, true, "GC Pad"),
    RumbleStrengthSupport::Binary,
    1,
    "wired GameCube pads must expose only on/off"
  );
  checkCapabilities(
    rumbleCapabilitiesForController(RZORD_GAMECUBE, true, "WaveBird"),
    RumbleStrengthSupport::None,
    0,
    "WaveBird must hide rumble"
  );
  checkCapabilities(
    rumbleCapabilitiesForController(RZORD_GAMECUBE, false, "GC Pad"),
    RumbleStrengthSupport::None,
    0,
    "disconnected controllers must not contribute capabilities"
  );
}

void testMultiplePortsAggregate() {
  RumbleCapabilities aggregate{};
  aggregate = combineRumbleCapabilities(
    aggregate,
    rumbleCapabilitiesForController(RZORD_N64, true, "N64 Pad")
  );
  aggregate = combineRumbleCapabilities(
    aggregate,
    rumbleCapabilitiesForController(RZORD_N64, true, "N64 Rumble")
  );
  checkCapabilities(
    aggregate,
    RumbleStrengthSupport::Binary,
    1,
    "a binary controller on port two must be detected"
  );

  aggregate = {};
  aggregate = combineRumbleCapabilities(
    aggregate,
    rumbleCapabilitiesForController(RZORD_PSX, true, "Digital")
  );
  aggregate = combineRumbleCapabilities(
    aggregate,
    rumbleCapabilitiesForController(RZORD_PSX, true, "DualShock2")
  );
  checkCapabilities(
    aggregate,
    RumbleStrengthSupport::Variable,
    2,
    "a variable controller on port two must be detected"
  );

  aggregate = combineRumbleCapabilities(
    {RumbleStrengthSupport::Binary, 1},
    aggregate
  );
  checkCapabilities(
    aggregate,
    RumbleStrengthSupport::Variable,
    2,
    "variable support must win over binary support"
  );
}

void testLevelPolicy() {
  check(!rumbleHasVariableStrength({RumbleStrengthSupport::None, 0}),
        "no-rumble controllers must hide strength");
  check(!rumbleHasVariableStrength({RumbleStrengthSupport::Binary, 1}),
        "binary controllers must hide strength");
  check(rumbleHasVariableStrength({RumbleStrengthSupport::Variable, 1}),
        "variable controllers must show strength");

  check(normalizeRumbleLevelForSupport(RumbleStrengthSupport::Binary, 0) == RUMBLE_MODE_OFF,
        "binary off must stay off");
  check(normalizeRumbleLevelForSupport(RumbleStrengthSupport::Binary, 2) == RUMBLE_MODE_VARIABLE,
        "binary nonzero must normalize to on");

  for (uint8_t level = 0; level < 3; ++level) {
    const uint8_t next =
      cycleRumbleLevelForSupport(RumbleStrengthSupport::Binary, level, true);
    const uint8_t previous =
      cycleRumbleLevelForSupport(RumbleStrengthSupport::Binary, level, false);
    check(next == RUMBLE_MODE_OFF || next == RUMBLE_MODE_VARIABLE,
          "binary forward cycling must remain Off or On");
    check(previous == RUMBLE_MODE_OFF || previous == RUMBLE_MODE_VARIABLE,
          "binary reverse cycling must remain Off or On");
  }

  check(cycleRumbleLevelForSupport(RumbleStrengthSupport::Variable, RUMBLE_MODE_OFF, true) == RUMBLE_MODE_VARIABLE,
        "variable forward cycling must include Variable");
  check(cycleRumbleLevelForSupport(RumbleStrengthSupport::Variable, RUMBLE_MODE_VARIABLE, true) == RUMBLE_MODE_MAX,
        "variable forward cycling must include Max");
  check(cycleRumbleLevelForSupport(RumbleStrengthSupport::Variable, RUMBLE_MODE_MAX, true) == RUMBLE_MODE_OFF,
        "variable forward cycling must wrap to Off");
  check(cycleRumbleLevelForSupport(RumbleStrengthSupport::Variable, RUMBLE_MODE_OFF, false) == RUMBLE_MODE_MAX,
        "variable reverse cycling must wrap to Max");
  check(cycleRumbleLevelForSupport(RumbleStrengthSupport::None, RUMBLE_MODE_MAX, true) == RUMBLE_MODE_OFF,
        "controllers without rumble must remain Off");
}

}  // namespace

int main() {
  testPotentialModeCapabilities();
  testDetectedControllerCapabilities();
  testMultiplePortsAggregate();
  testLevelPolicy();
  std::cout << "OK: rumble capability policy tests passed\n";
  return 0;
}
