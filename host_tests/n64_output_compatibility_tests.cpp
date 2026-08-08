#include <cstdint>
#include <cstdlib>
#include <iostream>

#include "firmware/output/n64_output_compatibility.h"

namespace {

void check(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    std::exit(1);
  }
}

void testOrdinaryControllerControlsStayNative() {
  check(output_n64_c_backing_r2(false, false, 1, 0) == 1,
        "ordinary R2 must remain R2");
  check(output_n64_c_backing_r2(false, false, 0, 1) == 0,
        "ordinary L3 must not leak into R2");
  check(output_n64_c_backing_r2(false, true, 1, 1) == 1,
        "N64 spatialization must not suppress ordinary R2");

  check(output_n64_c_backing_select(false, false, 1, 0) == 1,
        "ordinary Select must remain Select");
  check(output_n64_c_backing_select(false, false, 0, 1) == 0,
        "ordinary R3 must not leak into Select or Share");
  check(output_n64_c_backing_select(false, true, 1, 1) == 1,
        "N64 spatialization must not suppress ordinary Select");
}

void testN64BackingCompatibility() {
  check(output_n64_c_backing_r2(true, false, 0, 1) == 1,
        "unspatialized N64 C-Down must reach the legacy R2 field");
  check(output_n64_c_backing_r2(true, true, 0, 1) == 0,
        "spatialized N64 C-Down must not also assert R2");
  check(output_n64_c_backing_r2(true, false, 1, 0) == 0,
        "N64 compatibility must use C-Down backing, not native R2");

  check(output_n64_c_backing_select(true, false, 0, 1) == 1,
        "unspatialized N64 C-Right must reach the legacy Select field");
  check(output_n64_c_backing_select(true, true, 0, 1) == 0,
        "spatialized N64 C-Right must not also assert Select or Share");
  check(output_n64_c_backing_select(true, false, 1, 0) == 0,
        "N64 compatibility must use C-Right backing, not native Select");
}

}  // namespace

int main() {
  testOrdinaryControllerControlsStayNative();
  testN64BackingCompatibility();
  std::cout << "OK: N64 output compatibility tests passed\n";
  return 0;
}