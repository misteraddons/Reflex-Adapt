#include <cstdint>
#include <cstdlib>
#include <iostream>

#include "firmware/core/button_map_mode.h"

namespace {

void check(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    std::exit(1);
  }
}

void expectForceName(DeviceEnum input, outputMode_t output, const char* label) {
  check(buttonMapModePolicy(input, output, false) ==
          ButtonMapModePolicy::ForceName,
        label);
  check(!buttonMapModeIsUserSelectable(input, output, false), label);
  check(!effectivePositionButtonMap(input, output, false, 1), label);
}

void expectSelectable(DeviceEnum input, outputMode_t output, const char* label) {
  check(buttonMapModePolicy(input, output, false) ==
          ButtonMapModePolicy::UserSelectable,
        label);
  check(buttonMapModeIsUserSelectable(input, output, false), label);
  check(!effectivePositionButtonMap(input, output, false, 0), label);
  check(effectivePositionButtonMap(input, output, false, 1), label);
}

void testMatchingNintendoLayouts() {
  expectForceName(RZORD_SNES, OUTPUT_SWITCH,
                  "SNES to Pokken must use canonical names");
  expectForceName(RZORD_SNES, OUTPUT_SWITCHPRO,
                  "SNES to Switch Pro must use canonical names");
  expectForceName(RZORD_WII, OUTPUT_SWITCHPRO,
                  "Wii Classic to Switch Pro must use canonical names");

  expectForceName(RZORD_NES, OUTPUT_CONSOLE_NES,
                  "NES to NES must use canonical names");
  expectForceName(RZORD_SNES, OUTPUT_CONSOLE_SNES,
                  "SNES to SNES must use canonical names");
  expectForceName(RZORD_N64, OUTPUT_CONSOLE_N64,
                  "N64 to N64 must use canonical names");
  expectForceName(RZORD_GAMECUBE, OUTPUT_GCWIIU,
                  "GameCube to WUP-028 must use canonical names");
  expectForceName(RZORD_GAMECUBE, OUTPUT_CONSOLE_GC,
                  "GameCube to GameCube must use canonical names");
  expectForceName(RZORD_WII, OUTPUT_CONSOLE_WII,
                  "Wii Classic to Wii must use canonical names");
}

void testTranslatedLayoutsRemainSelectable() {
  expectSelectable(RZORD_NES, OUTPUT_SWITCHPRO,
                   "NES to Switch Pro needs Name/Position");
  expectSelectable(RZORD_N64, OUTPUT_SWITCHPRO,
                   "N64 to Switch Pro needs Name/Position");
  expectSelectable(RZORD_GAMECUBE, OUTPUT_SWITCHPRO,
                   "GameCube to Switch Pro needs Name/Position");
  expectSelectable(RZORD_VBOY, OUTPUT_SWITCHPRO,
                   "Virtual Boy to Switch Pro needs Name/Position");
  expectSelectable(RZORD_SNES, OUTPUT_XINPUT2P,
                   "SNES to XInput needs Name/Position");
  expectSelectable(RZORD_WII, OUTPUT_PS4,
                   "Wii Classic to PS4 needs Name/Position");
}

void testNativeNsoForcesNames() {
  for (DeviceEnum input : {RZORD_NES, RZORD_SNES, RZORD_N64}) {
    check(buttonMapModePolicy(input, OUTPUT_SWITCHPRO, true) ==
            ButtonMapModePolicy::ForceName,
          "native NSO identity must force canonical names");
    check(!buttonMapModeIsUserSelectable(input, OUTPUT_SWITCHPRO, true),
          "native NSO identity must hide Name/Position");
    check(!effectivePositionButtonMap(input, OUTPUT_SWITCHPRO, true, 1),
          "native NSO identity must ignore stale Position");
  }

  check(buttonMapModeIsUserSelectable(
          RZORD_GAMECUBE, OUTPUT_SWITCHPRO, true),
        "stale NSO state must not hide GameCube Name/Position");
}

void testUnsupportedInputsIgnoreSavedSetting() {
  check(buttonMapModePolicy(RZORD_PSX, OUTPUT_SWITCHPRO, false) ==
          ButtonMapModePolicy::NotApplicable,
        "PSX must not expose Nintendo Name/Position");
  check(!effectivePositionButtonMap(
          RZORD_PSX, OUTPUT_SWITCHPRO, false, 1),
        "PSX must ignore a stale Nintendo Position value");
}

void testPositionTransform() {
  const uint32_t original = INPUT_A | INPUT_X | INPUT_L1;
  const uint32_t mapped = applyNintendoPositionButtonMap(original, true);
  check((mapped & INPUT_B) != 0 && (mapped & INPUT_Y) != 0,
        "Position must swap A/B and X/Y");
  check((mapped & INPUT_A) == 0 && (mapped & INPUT_X) == 0,
        "Position must clear the original face bits");
  check((mapped & INPUT_L1) != 0,
        "Position must preserve non-face buttons");
  check(applyNintendoPositionButtonMap(original, false) == original,
        "Name must preserve every input bit");
}

}  // namespace

int main() {
  testMatchingNintendoLayouts();
  testTranslatedLayoutsRemainSelectable();
  testNativeNsoForcesNames();
  testUnsupportedInputsIgnoreSavedSetting();
  testPositionTransform();
  std::cout << "OK: button-map input/output policy tests passed\n";
  return 0;
}
