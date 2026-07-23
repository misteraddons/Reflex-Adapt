#include "Input_Dummy.h"

RZInputDummy::RZInputDummy() : RZInputModule() {}

const char* RZInputDummy::getUsbId() {
  return "Dummy";
}

void RZInputDummy::configureBcdDeviceVersion() {
}

const char* RZInputDummy::getDescription() {
  return "DUMMY";
}

void RZInputDummy::setup() {
  pollInterval = 10000;
  setInputPortCount(4);
  for (uint8_t i = 0; i < inputPortCount(); ++i) {
    controller_state_t& frame = inputFrame(i);
    frame.sticks_precision_bits = ANALOG_STICK_PRECISION_8;
    frame.HAS_ANALOG_STICK_MAIN = 1;
  }
}

void RZInputDummy::setup2() {
}

bool RZInputDummy::poll() {
  beginPollCycle();

  for (uint8_t port = 0; port < inputPortCount(); ++port) {
    uint8_t i = port;
    controller_state_t& frame = inputFrame(i);
    if (i == 0)
      frame.LX += 10;
    else if (i == 1)
      frame.LY += 10;
    else if (i == 2)
      frame.RX += 10;
    else if (i == 3)
      frame.RY += 10;
    setUpdated(i);
  }

  return endPollCycle();
}
