#include "Input_3do.h"

const input_3do_config_t input_3do_config[] = {
  { .clk = HDMI_1_06, .out = HDMI_1_05, .in = HDMI_1_07 },
  { .clk = HDMI_2_06, .out = HDMI_2_05, .in = HDMI_2_07 },
};

RZInput3do::RZInput3do() : RZInputModule() {}

const char* RZInput3do::getUsbId() {
  return "RZR3do";
}

void RZInput3do::configureBcdDeviceVersion() {
  bcd_device_version.platform = BCD_PLAT_3DO;
  bcd_device_version.platform_sub = 0;
}

const char* RZInput3do::getDescription() {
  return "3DO";
}

void RZInput3do::setup() {
  setInputPortCount(input_ports);

  pollInterval = 500;

  for (uint8_t i = 0; i < MAX_USB_OUT; ++i) {
    dtype[i] = THREEDO_DEVICE_NONE;
    controller_state_t& frame = inputFrame(i);
    frame.HAS_BTN_SELECT = 1;
    frame.HAS_BTN_START = 1;
  }

  for (uint8_t i = 0; i < input_ports; ++i) {
    input_3do_config_t c = input_3do_config[i];
    tdo[i] = new ThreedoPort(c.clk, c.out, c.in);
    tdo[i]->begin();
  }
}

void RZInput3do::setup2() {
  uint8_t total_controller_ports = 0;
  for (uint8_t i = 0; i < input_ports; ++i) {
    setPortLed(i, HIGH);
    if (tdo[i]->getControllerCount() > 1) {
      total_controller_ports += THREEDO_MAX_CTRL;
    } else {
      total_controller_ports += 1;
    }
  }
  setInputPortCount(min(total_controller_ports, MAX_USB_OUT));
  delay(1);
}
