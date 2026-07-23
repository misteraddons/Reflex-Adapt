#include "input_overlay_runtime.h"

#include "../firmware_platform_config.h"
#include "controller_frame_state.h"
#include "neutral_frame_packet.h"

namespace {

constexpr uint8_t kInputOverlayZero = 0x00;
constexpr uint8_t kInputOverlayOne = '1';
constexpr uint8_t kInputOverlaySplit = '\n';

bool inputOverlayEnabled = false;
uint16_t inputOverlayRate = INPUT_OVERLAY_DEFAULT_RATE_HZ;
uint32_t inputOverlayLastWriteMs = 0;
uint32_t inputOverlayFrames = 0;

uint16_t clampRate(uint16_t rateHz) {
  if (rateHz < INPUT_OVERLAY_MIN_RATE_HZ) {
    return INPUT_OVERLAY_MIN_RATE_HZ;
  }
  if (rateHz > INPUT_OVERLAY_MAX_RATE_HZ) {
    return INPUT_OVERLAY_MAX_RATE_HZ;
  }
  return rateHz;
}

uint16_t intervalForRate(uint16_t rateHz) {
  const uint16_t clamped = clampRate(rateHz);
  const uint16_t interval = (uint16_t)(1000u / clamped);
  return interval == 0 ? 1 : interval;
}

void writeInputOverlayBit(Print& stream, bool pressed) {
  stream.write(pressed ? kInputOverlayOne : kInputOverlayZero);
}

void writeArcadeFrameBits(Print& stream, const controller_state_t& frame) {
  const uint16_t mask = neutralFrameArcadeOverlayMask(frame);
  for (uint8_t bit = 0; bit < 16; ++bit) {
    writeInputOverlayBit(stream, (mask & ((uint16_t)1u << bit)) != 0);
  }
}

}  // namespace

void inputOverlaySetEnabled(bool enabled) {
  inputOverlayEnabled = enabled;
  inputOverlayLastWriteMs = 0;
}

bool inputOverlayIsEnabled() {
  return inputOverlayEnabled;
}

void inputOverlaySetRateHz(uint16_t rateHz) {
  inputOverlayRate = clampRate(rateHz);
}

uint16_t inputOverlayRateHz() {
  return inputOverlayRate;
}

uint32_t inputOverlayFrameCount() {
  return inputOverlayFrames;
}

void inputOverlayWriteStatus(Print& stream) {
  stream.print(F("OVERLAY ENABLED="));
  stream.print(inputOverlayEnabled ? 1 : 0);
  stream.print(F(" RATE_HZ="));
  stream.print((int)inputOverlayRate);
  stream.print(F(" PLAYERS="));
  stream.print((int)max_devices);
  stream.print(F(" BITS_PER_PLAYER=16"));
  stream.print(F(" FRAMES="));
  stream.println(inputOverlayFrames);
  stream.print(F("OVERLAY ORDER="));
  stream.println(kNeutralFrameArcadeOverlayOrder);
}

void inputOverlayTask(Print& stream) {
  if (!inputOverlayEnabled) {
    return;
  }

  const uint32_t now = millis();
  const uint16_t intervalMs = intervalForRate(inputOverlayRate);
  if (inputOverlayLastWriteMs != 0 &&
      (uint32_t)(now - inputOverlayLastWriteMs) < intervalMs) {
    return;
  }
  inputOverlayLastWriteMs = now;

  const uint8_t players = min(max_devices, (uint8_t)MAX_USB_OUT);
  for (uint8_t player = 0; player < players; ++player) {
    writeArcadeFrameBits(stream, controllerFrameConst(player));
  }
  stream.write(kInputOverlaySplit);
  ++inputOverlayFrames;
}
