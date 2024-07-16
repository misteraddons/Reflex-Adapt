#include <Arduino.h>
#include "RZInputModule.h"

void RZInputModule::resetState(const uint8_t index) {
  if (index >= totalUsb)
    return;
  state[index] = {};
}

bool RZInputModule::canChangeMode() {
  return options.inputMode == INPUT_MODE_HID;
}

uint16_t RZInputModule::convertAnalog(uint8_t value, bool applyDeadzone = true) {
  if (applyDeadzone && (value > 118 && value < 138))
    value = 128;
  return (value << 8) + value;
}

bool RZInputModule::canUseAnalogTrigger() {
  //return options.inputMode == INPUT_MODE_XINPUT || options.inputMode == INPUT_MODE_PS3; //|| options.inputMode == INPUT_MODE_HID;
  return options.inputMode == INPUT_MODE_XINPUT || (isPS3 && options.inputMode == INPUT_MODE_SWITCH);
}

//Static members
uint8_t RZInputModule::totalUsb {1};
uint16_t RZInputModule::sleepTime {DEFAULT_SLEEP_TIME};
bool RZInputModule::isPS3 {false};
