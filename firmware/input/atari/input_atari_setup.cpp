#include "Input_Atari.h"

const input_atari_config_t input_atari_config[2] = {
  { .up_pin = HDMI_1_01, .down_pin = HDMI_1_02, .left_pin = HDMI_1_05, .right_pin = HDMI_1_06, .trigger_pin = HDMI_1_07,
    .paddle_a_pin = HDMI_1_03, .paddle_b_pin = HDMI_1_04,
    .paddle_a_btn = HDMI_1_01, .paddle_b_btn = HDMI_1_02 },
  { .up_pin = HDMI_2_01, .down_pin = HDMI_2_02, .left_pin = HDMI_2_05, .right_pin = HDMI_2_06, .trigger_pin = HDMI_2_07,
    .paddle_a_pin = HDMI_2_03, .paddle_b_pin = HDMI_2_04,
    .paddle_a_btn = HDMI_2_01, .paddle_b_btn = HDMI_2_02 },
};

RZInputAtari::RZInputAtari() : RZInputModule() {}

uint16_t RZInputAtari::measurePaddleRC(uint8_t pin) {
  if (pin == 255) return 0;

  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  delayMicroseconds(10);
  pinMode(pin, INPUT);

  uint32_t start_time = micros();
  uint32_t timeout = start_time + RC_TIMEOUT_MICROS;
  while (digitalRead(pin) == LOW && micros() < timeout) {
  }

  uint32_t charge_time = micros() - start_time;
  if (charge_time < RC_MIN_TIME) charge_time = RC_MIN_TIME;
  if (charge_time > RC_MAX_TIME) charge_time = RC_MAX_TIME;
  return (uint16_t)charge_time;
}

uint8_t RZInputAtari::rcTimeToPaddleValue(uint16_t rc_time) {
  uint32_t mapped = ((uint32_t)(rc_time - RC_MIN_TIME) * 255) / (RC_MAX_TIME - RC_MIN_TIME);
  return (uint8_t)(mapped > 255 ? 255 : mapped);
}

const char* RZInputAtari::getUsbId() { return "RZRAtari"; }

void RZInputAtari::configureBcdDeviceVersion() {
  bcd_device_version.platform = BCD_PLAT_ATARI;
  bcd_device_version.platform_sub = 0;
}

const char* RZInputAtari::getDescription() { return "Atari Joysticks & Paddles"; }

void RZInputAtari::setup() {
  setInputPortCount(input_ports);
  pollInterval = 1000;

  for (uint8_t port = 0; port < input_ports; ++port) {
    const input_atari_config_t& c = input_atari_config[port];
    if (c.up_pin != 255) pinMode(c.up_pin, INPUT_PULLUP);
    if (c.down_pin != 255) pinMode(c.down_pin, INPUT_PULLUP);
    if (c.left_pin != 255) pinMode(c.left_pin, INPUT_PULLUP);
    if (c.right_pin != 255) pinMode(c.right_pin, INPUT_PULLUP);
    if (c.trigger_pin != 255) pinMode(c.trigger_pin, INPUT_PULLUP);
    if (c.paddle_a_pin != 255) pinMode(c.paddle_a_pin, INPUT);
    if (c.paddle_b_pin != 255) pinMode(c.paddle_b_pin, INPUT);
    if (c.paddle_a_btn != 255) pinMode(c.paddle_a_btn, INPUT_PULLUP);
    if (c.paddle_b_btn != 255) pinMode(c.paddle_b_btn, INPUT_PULLUP);
  }

  for (uint8_t i = 0; i < MAX_USB_OUT && i < input_ports; ++i) {
    controller_state_t& frame = inputFrame(i);
    frame.HAS_ANALOG_STICK_MAIN = 1;
    setInputFrameConnected(frame, true);
  }
}

void RZInputAtari::setup2() {
  for (uint8_t i = 0; i < input_ports; ++i)
    setPortLed(i, HIGH);
}
