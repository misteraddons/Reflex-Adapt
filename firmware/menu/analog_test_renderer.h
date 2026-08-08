#pragma once

#include <stdint.h>

#include "../core/controller_state.h"
#include "analog_stick_trace.h"

static constexpr uint16_t ANALOG_TEST_FRAME_INTERVAL_MS = 50;

struct AnalogStickTraceScreen {
  const AnalogStickTrace* trace;
  bool octagonal;
  int16_t current_x;
  int16_t current_y;
  analog_stick_precision precision;
  bool connected;
  bool right_stick;
  bool gamecube;
  uint8_t port;
  bool multiple_ports;
  bool multiple_sticks;
};

struct AnalogTestValue {
  const char* label;
  int16_t value;
};

int16_t analogTraceFullScale(analog_stick_precision precision);
int16_t analogTraceCenterThreshold(int16_t fullScale);
void renderAnalogStickTraceScreen(const AnalogStickTraceScreen& screen);
void renderAnalogValueTestScreen(const char* title, uint8_t port,
                                 const AnalogTestValue* values,
                                 uint8_t valueCount, bool multiplePorts);
