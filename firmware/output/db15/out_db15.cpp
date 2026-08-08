#include "../../product_config.h"

#ifdef ENABLE_OUTPUT_DB15

#include "out_db15.h"

#include <Arduino.h>

#include "../../core/controller_frame_state.h"

namespace {

struct Db15Signal {
  uint8_t pin;
  bool pressed;
};

void releaseLine(uint8_t pin) {
  digitalWrite(pin, LOW);
  pinMode(pin, INPUT);
}

void pressLine(uint8_t pin) {
  digitalWrite(pin, LOW);
  pinMode(pin, OUTPUT);
}

void driveLine(uint8_t pin, bool pressed) {
  if (pressed) {
    pressLine(pin);
  } else {
    releaseLine(pin);
  }
}

void driveSignals(const Db15Signal* signals, uint8_t count) {
  for (uint8_t i = 0; i < count; ++i) {
    driveLine(signals[i].pin, signals[i].pressed);
  }
}

}  // namespace

void db15_output_begin() {
  const uint8_t pins[] = {
    PIN_DB15_UP,
    PIN_DB15_DOWN,
    PIN_DB15_LEFT,
    PIN_DB15_RIGHT,
    PIN_DB15_B1,
    PIN_DB15_B2,
    PIN_DB15_B3,
    PIN_DB15_B4,
    PIN_DB15_B5,
    PIN_DB15_B6,
    PIN_DB15_START,
    PIN_DB15_COIN,
  };

  for (uint8_t pin : pins) {
    releaseLine(pin);
  }
}

void db15_output_update() {
  const controller_state_t& frame = controllerFrameConst(0);
  const bool connected = frame.connected;
  const Db15Signal signals[] = {
    {PIN_DB15_UP, connected && frame.PAD_U},
    {PIN_DB15_DOWN, connected && frame.PAD_D},
    {PIN_DB15_LEFT, connected && frame.PAD_L},
    {PIN_DB15_RIGHT, connected && frame.PAD_R},
    {PIN_DB15_B1, connected && frame.X},
    {PIN_DB15_B2, connected && frame.Y},
    {PIN_DB15_B3, connected && frame.R1},
    {PIN_DB15_B4, connected && frame.L1},
    {PIN_DB15_B5, connected && frame.A},
    {PIN_DB15_B6, connected && frame.B},
    {PIN_DB15_START, connected && frame.START},
    {PIN_DB15_COIN, connected && frame.SELECT},
  };
  driveSignals(signals, sizeof(signals) / sizeof(signals[0]));
}

#endif
