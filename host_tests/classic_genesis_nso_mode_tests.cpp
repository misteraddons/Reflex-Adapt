#include <cstdlib>
#include <cstdint>
#include <iostream>

#include "firmware/output/switch/output_switch_gamecube_mapping.h"
#include "firmware/output/switch/output_switch_genesis_nso_mapping.h"
#include "firmware/input/saturn/m30_identity_latch.h"
#include "third_party/firmware_libraries/SaturnLib/MegadriveSixButtonDecode.h"

namespace {

struct TestFrame {
  bool A = false;
  bool B = false;
  bool X = false;
  bool Y = false;
  bool L1 = false;
  bool R1 = false;
  bool L2 = false;
  bool R2 = false;
  bool L3 = false;
  bool R3 = false;
  bool START = false;
  bool SELECT = false;
  bool HOME = false;
  bool CAPTURE = false;
  bool PAD_D = false;
  bool PAD_U = false;
  bool PAD_R = false;
  bool PAD_L = false;
};

struct TestReport {
  uint8_t buttons[3] = {};
};

void check(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    std::exit(1);
  }
}

uint32_t reportBits(const TestReport& report) {
  return static_cast<uint32_t>(report.buttons[0]) |
         (static_cast<uint32_t>(report.buttons[1]) << 8) |
         (static_cast<uint32_t>(report.buttons[2]) << 16);
}

uint32_t emit(const TestFrame& frame) {
  TestReport report = {{0xFF, 0xFF, 0xFF}};
  switch_genesis_nso::apply_button_bits(frame, report);
  return reportBits(report);
}

void testSwitchProIdentityButtonMatrix() {
  struct Case {
    bool TestFrame::*member;
    uint32_t bit;
    const char* message;
  };
  const Case cases[] = {
      {&TestFrame::A, 1u << 0, "Genesis A must emit Switch Pro Y"},
      {&TestFrame::Y, 1u << 1, "Genesis Y must emit Switch Pro X"},
      {&TestFrame::B, 1u << 2, "Genesis B must emit Switch Pro B"},
      {&TestFrame::R1, 1u << 3, "Genesis C must emit Switch Pro A"},
      {&TestFrame::L1, 1u << 6, "Genesis Z must emit Switch Pro R"},
      {&TestFrame::R2, 1u << 7, "Genesis Mode must emit Switch Pro ZR"},
      {&TestFrame::START, 1u << 9, "Genesis Start must emit Switch Pro Plus"},
      {&TestFrame::HOME, 1u << 12, "Genesis Home must emit Switch Pro Home"},
      {&TestFrame::CAPTURE, 1u << 13,
       "Genesis Capture must emit Switch Pro Capture"},
      {&TestFrame::PAD_D, 1u << 16, "Genesis Down must emit Switch Pro Down"},
      {&TestFrame::PAD_U, 1u << 17, "Genesis Up must emit Switch Pro Up"},
      {&TestFrame::PAD_R, 1u << 18, "Genesis Right must emit Switch Pro Right"},
      {&TestFrame::PAD_L, 1u << 19, "Genesis Left must emit Switch Pro Left"},
      {&TestFrame::X, 1u << 22, "Genesis X must emit Switch Pro L"},
  };

  for (const Case& test : cases) {
    TestFrame frame;
    frame.*(test.member) = true;
    check(emit(frame) == test.bit, test.message);
  }
}

void testReservedBitsAndM30Home() {
  TestFrame frame;
  frame.SELECT = true;
  frame.L3 = true;
  frame.R3 = true;
  frame.L2 = true;
  check(emit(frame) == 0,
        "Genesis must not leak unavailable Minus, L2, L3, or R3 buttons");

  frame = {};
  frame.HOME = true;
  check(emit(frame) == (1u << 12),
        "M30 auxiliary Home must emit the firmware Home bit");

  frame = {};
  frame.CAPTURE = true;
  check(emit(frame) == (1u << 13),
        "M30 auxiliary Star must emit the firmware Capture bit");
}

void testM30PhysicalAuxiliaryPage() {
  check(saturnlib_megadrive::six_button_id_phase(0x00),
        "standard Mega6 ID page must qualify");
  check(saturnlib_megadrive::six_button_id_phase(0x02),
        "observed OEM Mega6 ID page must qualify");
  check(!saturnlib_megadrive::six_button_id_phase(0x01),
        "unrecognized Mega6 ID page must not qualify");

  check(saturnlib_megadrive::six_button_marker_valid(0x0F, true),
        "released M30 marker page must qualify");
  check(saturnlib_megadrive::six_button_marker_valid(0x0E, true),
        "M30 Home must not invalidate the fixed marker bits");
  check(saturnlib_megadrive::six_button_marker_valid(0x0B, true),
        "M30 Star must not invalidate the fixed marker bits");
  check(saturnlib_megadrive::six_button_marker_valid(0x0A, true),
        "M30 Home plus Star must not invalidate the fixed marker bits");
  check(!saturnlib_megadrive::six_button_marker_valid(0x0E, false),
        "standard-pad validation must still require all marker bits");
  check(!saturnlib_megadrive::six_button_marker_valid(0x0B, false),
        "standard-pad validation must reject the M30 Star marker");
  check(!saturnlib_megadrive::six_button_marker_valid(0x0A, false),
        "standard-pad validation must reject both M30 auxiliary buttons");
  check(!saturnlib_megadrive::six_button_marker_valid(0x0D, true),
        "M30 support must still require fixed D1");
  check(!saturnlib_megadrive::six_button_marker_valid(0x07, true),
        "M30 support must still require fixed D3");

  struct MarkerCase {
    uint8_t marker;
    bool home_pressed;
    bool star_pressed;
  };
  const MarkerCase cases[] = {
      {0x0F, false, false},
      {0x0E, true, false},
      {0x0B, false, true},
      {0x0A, true, true},
  };
  for (const MarkerCase& test : cases) {
    const uint8_t control_page =
        saturnlib_megadrive::m30_aux_control_page(test.marker);
    check(control_page == test.marker,
          "M30 decoder must preserve the validated marker nibble");
    const uint16_t digital =
        static_cast<uint16_t>(0x0FFFu | (control_page << 12));
    check(((digital & 0x1000u) == 0) == test.home_pressed,
          "M30 D0 must decode as active-low Home");
    check(((digital & 0x4000u) == 0) == test.star_pressed,
          "M30 D2 must decode as active-low Star");
    check((digital & 0x8000u) != 0,
          "M30 auxiliary controls must not alias Saturn L");
  }
}

void testM30IdentityLatch() {
  M30IdentityLatch identity;
  check(!identity.identified(),
        "M30 identity must start unknown");

  identity.observe(false, true, true);
  check(!identity.identified(),
        "auxiliary-looking activity must not identify an unqualified pad");

  identity.observe(true, false, false);
  check(!identity.identified(),
        "a released Mega6 marker page must not identify an M30");

  identity.observe(true, true, false);
  check(identity.identified(),
        "qualified M30 Home activity must identify the controller");

  identity.observe(true, false, false);
  check(identity.identified(),
        "M30 identity must remain latched after Home is released");

  identity.reset();
  check(!identity.identified(),
        "physical-disconnect cleanup must clear M30 identity");

  identity.observe(true, false, true);
  check(identity.identified(),
        "qualified M30 Star activity must identify the controller");
}

void testCombinedPacket() {
  TestFrame frame;
  frame.A = true;
  frame.B = true;
  frame.X = true;
  frame.Y = true;
  frame.L1 = true;
  frame.R1 = true;
  frame.R2 = true;
  frame.START = true;
  frame.HOME = true;
  frame.CAPTURE = true;
  frame.PAD_D = true;
  frame.PAD_U = true;
  frame.PAD_R = true;
  frame.PAD_L = true;

  const uint32_t expected =
      (1u << 0) | (1u << 1) | (1u << 2) | (1u << 3) |
      (1u << 6) | (1u << 7) | (1u << 9) | (1u << 12) |
      (1u << 13) | (1u << 16) | (1u << 17) | (1u << 18) |
      (1u << 19) | (1u << 22);
  check(emit(frame) == expected,
        "combined Genesis packet must match the Switch Pro compensation");
  check(switch_genesis_nso::pack_button_bits(frame) == expected,
        "production packer and emitted packet must agree");
}

void testPceSixButtonPositionMatrix() {
  struct Case {
    bool TestFrame::*member;
    uint32_t bit;
    const char* message;
  };
  const Case cases[] = {
      {&TestFrame::R1, 1u << 0, "PCE III must emit Switch Pro Y"},
      {&TestFrame::A, 1u << 2, "PCE II must emit Switch Pro B"},
      {&TestFrame::B, 1u << 3, "PCE I must emit Switch Pro A"},
      {&TestFrame::L1, 1u << 22, "PCE IV must emit Switch Pro L"},
      {&TestFrame::X, 1u << 1, "PCE V must emit Switch Pro X"},
      {&TestFrame::Y, 1u << 6, "PCE VI must emit Switch Pro R"},
  };

  for (const Case& test : cases) {
    TestFrame frame;
    frame.*(test.member) = true;
    TestReport report{};
    switch_genesis_nso::apply_pce_six_button_position_bits(frame, report);
    check(reportBits(report) == test.bit, test.message);
  }
}

void testPceSixButtonMappingPreservesOtherControls() {
  const uint32_t retained =
      switch_genesis_nso::kZr |
      switch_genesis_nso::kMinus |
      switch_genesis_nso::kPlus |
      switch_genesis_nso::kHome |
      switch_genesis_nso::kCapture |
      switch_genesis_nso::kDown |
      switch_genesis_nso::kUp |
      switch_genesis_nso::kRight |
      switch_genesis_nso::kLeft;
  const uint32_t seeded =
      retained | switch_genesis_nso::kGenesisSixButtonPositionMask;
  TestReport report = {{
      static_cast<uint8_t>(seeded),
      static_cast<uint8_t>(seeded >> 8),
      static_cast<uint8_t>(seeded >> 16),
  }};
  TestFrame frame;
  switch_genesis_nso::apply_pce_six_button_position_bits(frame, report);
  check(reportBits(report) == retained,
        "PCE position mapping must clear only the six positional buttons");

  frame.A = true;
  frame.B = true;
  frame.X = true;
  frame.Y = true;
  frame.L1 = true;
  frame.R1 = true;
  switch_genesis_nso::apply_pce_six_button_position_bits(frame, report);
  check(
      reportBits(report) ==
          (retained | switch_genesis_nso::kGenesisSixButtonPositionMask),
      "PCE six-button chords must preserve Select, Run, and D-pad controls");
}

void testSaturnMatchesGenesisSixButtonPositions() {
  struct Case {
    bool TestFrame::*member;
    uint32_t bit;
    const char* message;
  };
  const Case cases[] = {
      {&TestFrame::A, 1u << 0, "Saturn A must emit Switch Pro Y"},
      {&TestFrame::B, 1u << 2, "Saturn B must emit Switch Pro B"},
      {&TestFrame::R1, 1u << 3, "Saturn C must emit Switch Pro A"},
      {&TestFrame::X, 1u << 22, "Saturn X must emit Switch Pro L"},
      {&TestFrame::Y, 1u << 1, "Saturn Y must emit Switch Pro X"},
      {&TestFrame::L1, 1u << 6, "Saturn Z must emit Switch Pro R"},
  };

  for (const Case& test : cases) {
    TestFrame frame;
    frame.*(test.member) = true;
    TestReport report{};
    switch_genesis_nso::apply_six_button_position_bits(frame, report);
    check(reportBits(report) == test.bit, test.message);
  }

  constexpr uint32_t kReportMask = 0x00FFFFFFu;
  const uint32_t retained =
      kReportMask & ~switch_genesis_nso::kGenesisSixButtonPositionMask;
  TestReport report = {{
      static_cast<uint8_t>(kReportMask),
      static_cast<uint8_t>(kReportMask >> 8),
      static_cast<uint8_t>(kReportMask >> 16),
  }};
  TestFrame frame;
  switch_genesis_nso::apply_six_button_position_bits(frame, report);
  check(reportBits(report) == retained,
        "Saturn mapping must preserve shoulders, system buttons, and D-pad");
}

void testJaguarMatchesGenesisSixButtonPositions() {
  struct Case {
    bool TestFrame::*member;
    uint32_t bit;
    const char* message;
  };
  const Case cases[] = {
      {&TestFrame::X, 1u << 0, "Jaguar C must emit Genesis A position"},
      {&TestFrame::A, 1u << 2, "Jaguar B must emit Genesis B position"},
      {&TestFrame::B, 1u << 3, "Jaguar A must emit Genesis C position"},
      {&TestFrame::L1, 1u << 22, "Jaguar 7 must emit Genesis X position"},
      {&TestFrame::Y, 1u << 1, "Jaguar 8 must emit Genesis Y position"},
      {&TestFrame::R1, 1u << 6, "Jaguar 9 must emit Genesis Z position"},
  };
  for (const Case& test : cases) {
    TestFrame frame;
    frame.*(test.member) = true;
    TestReport report{};
    switch_genesis_nso::apply_jaguar_six_button_position_bits(frame, report);
    check(reportBits(report) == test.bit, test.message);
  }
}

void testGameCubeLeftShoulderAssignment() {
  auto output = switch_gamecube::map_left_shoulder(
      true, false, false, GAMECUBE_L_SWITCH_ZL);
  check(!output.l && output.zl,
        "GameCube left shoulder must default to ZL only");

  output = switch_gamecube::map_left_shoulder(
      true, false, false, GAMECUBE_L_SWITCH_L);
  check(output.l && !output.zl,
        "GameCube left shoulder alternate must emit L only");

  output = switch_gamecube::map_left_shoulder(
      false, false, false, GAMECUBE_L_SWITCH_ZL);
  check(!output.l && !output.zl,
        "released GameCube left shoulder must emit neither L nor ZL");

  output = switch_gamecube::map_left_shoulder(
      false, true, false, GAMECUBE_L_SWITCH_L);
  check(output.l && !output.zl,
        "GameCube analog threshold must follow the selected assignment");

  output = switch_gamecube::map_left_shoulder(
      false, true, true, GAMECUBE_L_SWITCH_ZL);
  check(!output.l && !output.zl,
        "RTrig=RStick mode must keep analog travel off shoulder buttons");

  output = switch_gamecube::map_left_shoulder(
      true, true, true, GAMECUBE_L_SWITCH_L);
  check(output.l && !output.zl,
        "GameCube hard switch must remain available in RTrig=RStick mode");

  output = switch_gamecube::map_left_shoulder(true, false, false, 0xFF);
  check(!output.l && output.zl,
        "invalid GameCube left-shoulder settings must fail safe to ZL");
}

}  // namespace

int main() {
  testSwitchProIdentityButtonMatrix();
  testReservedBitsAndM30Home();
  testM30PhysicalAuxiliaryPage();
  testM30IdentityLatch();
  testCombinedPacket();
  testPceSixButtonPositionMatrix();
  testPceSixButtonMappingPreservesOtherControls();
  testSaturnMatchesGenesisSixButtonPositions();
  testJaguarMatchesGenesisSixButtonPositions();
  testGameCubeLeftShoulderAssignment();
  std::cout
      << "OK: Classic2USB empirical Switch Pro mappings verified; "
         "official Genesis firmware capture is tested separately\n";
  return 0;
}
