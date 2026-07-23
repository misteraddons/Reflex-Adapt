#include "Input_Intellivision.h"

const input_intv_config_t input_intv_config[2] = {
  { .b0 = HDMI_1_04, .b1 = HDMI_1_03, .b2 = HDMI_1_02, .b3 = HDMI_1_01,
    .b4 = HDMI_1_09, .b5 = HDMI_1_08, .b6 = HDMI_1_07, .b7 = HDMI_1_06 },
  { .b0 = HDMI_2_04, .b1 = HDMI_2_03, .b2 = HDMI_2_02, .b3 = HDMI_2_01,
    .b4 = HDMI_2_09, .b5 = HDMI_2_08, .b6 = HDMI_2_07, .b7 = HDMI_2_06 },
};

RZInputIntv::RZInputIntv() : RZInputModule() {}

const char* RZInputIntv::getUsbId() {
  return "RZRINTV";
}

void RZInputIntv::configureBcdDeviceVersion() {
  bcd_device_version.platform = BCD_PLAT_INTV;
  bcd_device_version.platform_sub = 0;
}

const char* RZInputIntv::getDescription() {
  return "INTELLIVISION";
}

void RZInputIntv::setup() {
  pollInterval = 1000;
  setInputPortCount(input_ports);

  for (uint8_t p = 0; p < input_ports; ++p) {
    const input_intv_config_t& pins = input_intv_config[p];

    pinMode(pins.b0, INPUT_PULLUP);
    pinMode(pins.b1, INPUT_PULLUP);
    pinMode(pins.b2, INPUT_PULLUP);
    pinMode(pins.b3, INPUT_PULLUP);
    pinMode(pins.b4, INPUT_PULLUP);
    pinMode(pins.b5, INPUT_PULLUP);
    pinMode(pins.b6, INPUT_PULLUP);
    pinMode(pins.b7, INPUT_PULLUP);

    if (p < MAX_USB_OUT) {
      controller_state_t& frame = inputFrame(p);
      frame.HAS_BTN_SELECT = 1;
      frame.HAS_BTN_START = 1;
      setInputFrameConnected(frame, true);
    }
  }
}

void RZInputIntv::setup2() {
  for (uint8_t i = 0; i < input_ports; ++i)
    setPortLed(i, HIGH);
}

uint8_t RZInputIntv::readController(uint8_t port) {
  const input_intv_config_t& pins = input_intv_config[port];
  uint8_t data = 0;

  if (!digitalRead(pins.b0)) data |= 0x01;
  if (!digitalRead(pins.b1)) data |= 0x02;
  if (!digitalRead(pins.b2)) data |= 0x04;
  if (!digitalRead(pins.b3)) data |= 0x08;
  if (!digitalRead(pins.b4)) data |= 0x10;
  if (!digitalRead(pins.b5)) data |= 0x20;
  if (!digitalRead(pins.b6)) data |= 0x40;
  if (!digitalRead(pins.b7)) data |= 0x80;

  return data;
}
