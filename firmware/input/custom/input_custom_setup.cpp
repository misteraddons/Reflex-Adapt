#include "Input_Custom.h"

const input_custom_config_t input_custom_config[1] = {
  {
    .buttons = {
      .up = 2, .down = 3, .left = 5, .right = 4,
      .a = 6, .b = 7, .x = 10, .y = 11,
      .l1 = 18, .r1 = 19, .l2 = -1, .r2 = -1,
      .l3 = -1, .r3 = -1, .select = 16, .start = 17, .home = -1
    },
    .sticks = { .stick_lx = -1, .stick_ly = -1, .stick_rx = -1, .stick_ry = -1 },
    .debounceMs = 8
  },
};

RZInputCustom::RZInputCustom() : RZInputModule() {}

const char* RZInputCustom::getUsbId() {
  return "RZRCUSTOM";
}

void RZInputCustom::configureBcdDeviceVersion() {
  bcd_device_version.platform = 0;
  bcd_device_version.platform_sub = 0;
}

const char* RZInputCustom::getDescription() {
  return "CUSTOM";
}

void RZInputCustom::setup() {
  pollInterval = 250;
  setInputPortCount(input_ports);

  for (uint8_t i = 0; i < input_ports; ++i) {
    input_custom_config_t c = input_custom_config[i];
    uint32_t buttonsMask = 0;
    for (uint8_t j = 0; j < sizeof(c.btnArray); ++j) {
      if (c.btnArray[j] != -1)
        buttonsMask |= (1UL << c.btnArray[j]);
    }
    debouncer[i] = new ButtonDebouncer(buttonsMask, c.debounceMs);

    for (uint8_t k = 0; k < sizeof(c.sticksArray); ++k) {
      if (c.sticksArray[k] != -1)
        pinMode(c.sticksArray[k], INPUT);
    }

    if (i < MAX_USB_OUT) {
      controller_state_t& frame = inputFrame(i);
      frame.HAS_BTN_SELECT = input_custom_config[i].buttons.select != -1;
      frame.HAS_BTN_START = input_custom_config[i].buttons.start != -1;
      frame.HAS_BTN_HOME = input_custom_config[i].buttons.home != -1;
      frame.HAS_ANALOG_STICK_MAIN = input_custom_config[i].sticks.stick_lx != -1;
      frame.HAS_ANALOG_STICK_AUX = input_custom_config[i].sticks.stick_rx != -1;
      frame.sticks_precision_bits = ANALOG_STICK_PRECISION_12;
      setInputFrameConnected(frame, true);
    }

    analogReadResolution(12);
  }
}

void RZInputCustom::setup2() {
  for (uint8_t i = 0; i < input_ports; ++i)
    setPortLed(i, HIGH);
}

uint16_t RZInputCustom::tryReadAnalog(uint8_t pin) {
  return pin == static_cast<uint8_t>(-1) ? 0 : analogRead(pin);
}
