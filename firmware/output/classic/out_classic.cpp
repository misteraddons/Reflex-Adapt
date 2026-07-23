#include "../../product_config.h"

#ifdef ENABLE_OUTPUT_CONSOLE

#include "out_classic.h"

#include <Arduino.h>

#include "../../core/controller_frame_state.h"
#include "../output_runtime_state.h"

namespace {

uint16_t latchedBits = 0xFFFF;
uint8_t shiftIndex = 0;
bool lastLatch = false;
bool lastClock = false;
outputMode_t lastMode = OUTPUT_LAST;

void releaseData() {
  digitalWrite(PIN_CON_DATA0, LOW);
  pinMode(PIN_CON_DATA0, INPUT);
}

void driveDataLow() {
  digitalWrite(PIN_CON_DATA0, LOW);
  pinMode(PIN_CON_DATA0, OUTPUT);
}

void driveReleasedBit(bool released) {
  if (released) {
    releaseData();
  } else {
    driveDataLow();
  }
}

void setReleasedBit(uint16_t& bits, uint8_t index, bool pressed) {
  if (!pressed) {
    bits |= (uint16_t)(1u << index);
  } else {
    bits &= (uint16_t)~(1u << index);
  }
}

uint16_t buildNesBits(const controller_state_t& frame) {
  uint16_t bits = 0x00FF;
  const bool connected = frame.connected;
  setReleasedBit(bits, 0, connected && frame.A);
  setReleasedBit(bits, 1, connected && frame.B);
  setReleasedBit(bits, 2, connected && frame.SELECT);
  setReleasedBit(bits, 3, connected && frame.START);
  setReleasedBit(bits, 4, connected && frame.PAD_U);
  setReleasedBit(bits, 5, connected && frame.PAD_D);
  setReleasedBit(bits, 6, connected && frame.PAD_L);
  setReleasedBit(bits, 7, connected && frame.PAD_R);
  return bits;
}

uint16_t buildSnesBits(const controller_state_t& frame) {
  uint16_t bits = 0xFFFF;
  const bool connected = frame.connected;
  setReleasedBit(bits, 0, connected && frame.B);
  setReleasedBit(bits, 1, connected && frame.Y);
  setReleasedBit(bits, 2, connected && frame.SELECT);
  setReleasedBit(bits, 3, connected && frame.START);
  setReleasedBit(bits, 4, connected && frame.PAD_U);
  setReleasedBit(bits, 5, connected && frame.PAD_D);
  setReleasedBit(bits, 6, connected && frame.PAD_L);
  setReleasedBit(bits, 7, connected && frame.PAD_R);
  setReleasedBit(bits, 8, connected && frame.A);
  setReleasedBit(bits, 9, connected && frame.X);
  setReleasedBit(bits, 10, connected && frame.L1);
  setReleasedBit(bits, 11, connected && frame.R1);
  return bits;
}

uint8_t bitCountForMode(outputMode_t mode) {
  return mode == OUTPUT_CONSOLE_NES ? 8 : 16;
}

uint16_t buildBitsForMode(outputMode_t mode) {
  const controller_state_t& frame = controllerFrameConst(0);
  if (mode == OUTPUT_CONSOLE_NES) {
    return buildNesBits(frame);
  }
  return buildSnesBits(frame);
}

void outputCurrentBit(outputMode_t mode) {
  const uint8_t bitCount = bitCountForMode(mode);
  const bool released = shiftIndex < bitCount ? ((latchedBits >> shiftIndex) & 0x01u) != 0 : true;
  driveReleasedBit(released);
}

void resetProtocolState(outputMode_t mode) {
  lastMode = mode;
  lastLatch = digitalRead(PIN_CON_LATCH) != LOW;
  lastClock = digitalRead(PIN_CON_CLK) != LOW;
  shiftIndex = 0;
  latchedBits = buildBitsForMode(mode);
  outputCurrentBit(mode);
}

}  // namespace

void classic_output_begin() {
  pinMode(PIN_CON_LATCH, INPUT_PULLUP);
  pinMode(PIN_CON_CLK, INPUT_PULLUP);
  releaseData();
  resetProtocolState(sanitizeRuntimeOutputMode(outputMode));
}

void classic_output_update() {
  const outputMode_t mode = sanitizeRuntimeOutputMode(outputMode);
  if (mode != lastMode) {
    resetProtocolState(mode);
  }

  const bool latch = digitalRead(PIN_CON_LATCH) != LOW;
  const bool clock = digitalRead(PIN_CON_CLK) != LOW;

  if (latch && !lastLatch) {
    latchedBits = buildBitsForMode(mode);
    shiftIndex = 0;
    outputCurrentBit(mode);
  } else if (!latch && clock && !lastClock) {
    if (shiftIndex < 16) {
      ++shiftIndex;
    }
    outputCurrentBit(mode);
  } else if (latch) {
    latchedBits = buildBitsForMode(mode);
    shiftIndex = 0;
    outputCurrentBit(mode);
  }

  lastLatch = latch;
  lastClock = clock;
}

#endif
