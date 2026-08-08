#pragma once

#include <cstdint>

// N64 C-Down and C-Right use L3/R3 only as unique neutral-frame backing
// bits. Console outputs may consume those bits as face buttons, or legacy
// paths may expose them as R2/Select. Ordinary controllers must always retain
// their native R2 and Select controls regardless of the N64 compatibility
// policy selected for the current output.
constexpr uint8_t output_n64_c_backing_r2(
    bool frameIsN64,
    bool spatialized,
    uint8_t nativeR2,
    uint8_t cDownBacking) {
  if (!frameIsN64) {
    return nativeR2;
  }
  return spatialized ? 0 : cDownBacking;
}

constexpr uint8_t output_n64_c_backing_select(
    bool frameIsN64,
    bool spatialized,
    uint8_t nativeSelect,
    uint8_t cRightBacking) {
  if (!frameIsN64) {
    return nativeSelect;
  }
  return spatialized ? 0 : cRightBacking;
}