#pragma once

#include <stdint.h>

struct RumbleRuntimePortDiag {
  uint8_t raw_left;
  uint8_t raw_right;
  uint8_t scaled_left;
  uint8_t scaled_right;
  uint8_t test_left;
  uint8_t test_right;
  uint8_t test_active;
  uint8_t zero_pending;
  uint32_t update_count;
};

void rumbleRuntimeSetHostFeedback(uint8_t port, uint8_t left, uint8_t right);
void rumbleRuntimeGetEffectiveFeedback(uint8_t port, uint8_t* left, uint8_t* right);
void rumbleRuntimeStartTest(uint32_t portMask, uint8_t left, uint8_t right, uint16_t durationMs);
bool rumbleRuntimeGetPortDiag(uint8_t port, RumbleRuntimePortDiag* out);

void rumbleTestStart(uint8_t left, uint8_t right, uint16_t durationMs);
void rumbleTestStop();
void rumbleTestUpdate();
bool rumbleTestActive();
