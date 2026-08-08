#pragma once

#include <stdint.h>

extern "C++" {

namespace switch_genesis_nso {

// Switch full-input report button bits. Genesis, digital Saturn, and six-button
// PCE intentionally retain the shared 057E:2009 Switch Pro USB identity, so
// their production mapping compensates for how the Switch interprets that
// identity. The official 057E:201E Genesis firmware packet is different and is
// validated independently by tools/test_classic_genesis_nso_mode.py.
constexpr uint32_t kY = 1u << 0;
constexpr uint32_t kX = 1u << 1;
constexpr uint32_t kB = 1u << 2;
constexpr uint32_t kA = 1u << 3;
constexpr uint32_t kR = 1u << 6;
constexpr uint32_t kZr = 1u << 7;
constexpr uint32_t kMinus = 1u << 8;
constexpr uint32_t kPlus = 1u << 9;
constexpr uint32_t kHome = 1u << 12;
constexpr uint32_t kCapture = 1u << 13;
constexpr uint32_t kDown = 1u << 16;
constexpr uint32_t kUp = 1u << 17;
constexpr uint32_t kRight = 1u << 18;
constexpr uint32_t kLeft = 1u << 19;
constexpr uint32_t kL = 1u << 22;
constexpr uint32_t kGenesisSixButtonPositionMask =
    kY | kX | kB | kA | kR | kL;

template <typename Frame>
inline uint32_t pack_six_button_position_bits(const Frame& frame) {
  uint32_t bits = 0;
  if (frame.A) bits |= kY;       // Genesis/Saturn A -> Switch Pro Y.
  if (frame.B) bits |= kB;       // Genesis/Saturn B -> Switch Pro B.
  if (frame.R1) bits |= kA;      // Genesis/Saturn C -> Switch Pro A.
  if (frame.X) bits |= kL;       // Genesis/Saturn X -> Switch Pro L.
  if (frame.Y) bits |= kX;       // Genesis/Saturn Y -> Switch Pro X.
  if (frame.L1) bits |= kR;      // Genesis/Saturn Z -> Switch Pro R.
  return bits;
}

template <typename Frame>
inline uint32_t pack_button_bits(const Frame& frame) {
  uint32_t bits = pack_six_button_position_bits(frame);
  if (frame.R2) bits |= kZr;     // Genesis Mode.
  if (frame.START) bits |= kPlus;
  if (frame.HOME) bits |= kHome;
  if (frame.CAPTURE) bits |= kCapture;
  if (frame.PAD_D) bits |= kDown;
  if (frame.PAD_U) bits |= kUp;
  if (frame.PAD_R) bits |= kRight;
  if (frame.PAD_L) bits |= kLeft;
  return bits;
}

template <typename Frame, typename Report>
inline void apply_button_bits(const Frame& frame, Report& report) {
  const uint32_t bits = pack_button_bits(frame);
  report.buttons[0] = static_cast<uint8_t>(bits);
  report.buttons[1] = static_cast<uint8_t>(bits >> 8);
  report.buttons[2] = static_cast<uint8_t>(bits >> 16);
}

template <typename Frame, typename Report>
inline void apply_six_button_position_bits(
    const Frame& frame, Report& report) {
  uint32_t bits =
      static_cast<uint32_t>(report.buttons[0]) |
      (static_cast<uint32_t>(report.buttons[1]) << 8) |
      (static_cast<uint32_t>(report.buttons[2]) << 16);
  bits = (bits & ~kGenesisSixButtonPositionMask) |
         pack_six_button_position_bits(frame);
  report.buttons[0] = static_cast<uint8_t>(bits);
  report.buttons[1] = static_cast<uint8_t>(bits >> 8);
  report.buttons[2] = static_cast<uint8_t>(bits >> 16);
}

template <typename Frame>
inline uint32_t pack_pce_six_button_position_bits(const Frame& frame) {
  uint32_t bits = 0;
  if (frame.R1) bits |= kY;  // PCE III -> Switch Pro Y.
  if (frame.A) bits |= kB;   // PCE II  -> Switch Pro B.
  if (frame.B) bits |= kA;   // PCE I   -> Switch Pro A.
  if (frame.L1) bits |= kL;  // PCE IV  -> Switch Pro L.
  if (frame.X) bits |= kX;   // PCE V   -> Switch Pro X.
  if (frame.Y) bits |= kR;   // PCE VI  -> Switch Pro R.
  return bits;
}

template <typename Frame, typename Report>
inline void apply_pce_six_button_position_bits(
    const Frame& frame, Report& report) {
  uint32_t bits =
      static_cast<uint32_t>(report.buttons[0]) |
      (static_cast<uint32_t>(report.buttons[1]) << 8) |
      (static_cast<uint32_t>(report.buttons[2]) << 16);
  bits = (bits & ~kGenesisSixButtonPositionMask) |
         pack_pce_six_button_position_bits(frame);
  report.buttons[0] = static_cast<uint8_t>(bits);
  report.buttons[1] = static_cast<uint8_t>(bits >> 8);
  report.buttons[2] = static_cast<uint8_t>(bits >> 16);
}

}  // namespace switch_genesis_nso

}  // extern "C++"
