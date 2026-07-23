#include "Input_Gameport.h"

namespace {
constexpr uint32_t GAMEPORT_DISCHARGE_US = 10;
constexpr uint32_t GAMEPORT_TIMEOUT_US = 1000;
constexpr uint32_t GAMEPORT_MIN_US = 10;
constexpr uint32_t GAMEPORT_MAX_US = 700;
}

const input_gameport_config_t input_gameport_config[2] = {
  {
    .pinBtn1 = HDMI_1_01,
    .pinBtn2 = HDMI_1_02,
    .pinBtn3 = HDMI_1_05,
    .pinBtn4 = HDMI_1_06,
    .pinAxisX = HDMI_1_03,
    .pinAxisY = HDMI_1_04,
  },
  {
    .pinBtn1 = HDMI_2_01,
    .pinBtn2 = HDMI_2_02,
    .pinBtn3 = HDMI_2_05,
    .pinBtn4 = HDMI_2_06,
    .pinAxisX = HDMI_2_03,
    .pinAxisY = HDMI_2_04,
  },
};

RZInputGameport::RZInputGameport() : RZInputModule() {}

uint32_t RZInputGameport::readAxisTiming(uint8_t pin) {
  gpio_set_dir(pin, GPIO_OUT);
  gpio_put(pin, LOW);
  delayMicroseconds(GAMEPORT_DISCHARGE_US);

  uint32_t startTime = micros();
  gpio_set_dir(pin, GPIO_IN);

  while (!gpio_get(pin)) {
    if (micros() - startTime > GAMEPORT_TIMEOUT_US) {
      return GAMEPORT_TIMEOUT_US;
    }
  }

  return micros() - startTime;
}

int16_t RZInputGameport::timingToAxis(uint32_t timing) {
  if (timing >= GAMEPORT_TIMEOUT_US) {
    return 0;
  }

  if (timing < GAMEPORT_MIN_US) timing = GAMEPORT_MIN_US;
  if (timing > GAMEPORT_MAX_US) timing = GAMEPORT_MAX_US;

  uint32_t range = GAMEPORT_MAX_US - GAMEPORT_MIN_US;
  uint32_t normalized = timing - GAMEPORT_MIN_US;
  int32_t axis = ((int32_t)normalized * 65535 / range) - 32768;

  return (int16_t)axis;
}

bool RZInputGameport::checkConnected(uint8_t port) {
  uint32_t xTime = readAxisTiming(input_gameport_config[port].pinAxisX);
  uint32_t yTime = readAxisTiming(input_gameport_config[port].pinAxisY);
  return (xTime < GAMEPORT_TIMEOUT_US || yTime < GAMEPORT_TIMEOUT_US);
}

const char* RZInputGameport::getUsbId() {
  return "RZRGP";
}

void RZInputGameport::configureBcdDeviceVersion() {
  bcd_device_version.platform = BCD_PLAT_GAMEPORT;
  bcd_device_version.platform_sub = 0;
}

const char* RZInputGameport::getDescription() {
  return "PC Gameport";
}

void RZInputGameport::setup() {
  setInputPortCount(input_ports);

  for (uint8_t i = 0; i < input_ports; ++i) {
    const input_gameport_config_t& cfg = input_gameport_config[i];

    gpio_init(cfg.pinBtn1);
    gpio_set_dir(cfg.pinBtn1, GPIO_IN);
    gpio_pull_up(cfg.pinBtn1);

    gpio_init(cfg.pinBtn2);
    gpio_set_dir(cfg.pinBtn2, GPIO_IN);
    gpio_pull_up(cfg.pinBtn2);

    gpio_init(cfg.pinBtn3);
    gpio_set_dir(cfg.pinBtn3, GPIO_IN);
    gpio_pull_up(cfg.pinBtn3);

    gpio_init(cfg.pinBtn4);
    gpio_set_dir(cfg.pinBtn4, GPIO_IN);
    gpio_pull_up(cfg.pinBtn4);

    gpio_init(cfg.pinAxisX);
    gpio_init(cfg.pinAxisY);

    if (i < MAX_USB_OUT) {
      controller_state_t& frame = inputFrame(i);
      frame.HAS_BTN_START = 0;
      frame.HAS_BTN_SELECT = 0;
    }
  }
}

void RZInputGameport::setup2() {
  for (uint8_t i = 0; i < input_ports; ++i) {
    setPortLed(i, HIGH);
  }
}
