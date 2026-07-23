#include "Input_Jaguar.h"

const input_jag_config_t input_jag_config[2] = {
  { .j3_j4 = HDMI_1_07, .j2_j5 = HDMI_1_08, .j1_j6 = HDMI_1_03, .j0_j7 = HDMI_1_04, .b0_b2 = HDMI_1_05, .b1_b3 = HDMI_1_06, .j11_j15 = HDMI_1_01, .j10_j14 = HDMI_1_02, .j9_j13 = HDMI_1_09, .j8_j12 = HDMI_1_12, },
  { .j3_j4 = HDMI_2_07, .j2_j5 = HDMI_2_08, .j1_j6 = HDMI_2_03, .j0_j7 = HDMI_2_04, .b0_b2 = HDMI_2_05, .b1_b3 = HDMI_2_06, .j11_j15 = HDMI_2_01, .j10_j14 = HDMI_2_02, .j9_j13 = HDMI_2_09, .j8_j12 = HDMI_2_12, },
};

RZInputJaguar::RZInputJaguar() : RZInputModule() {}

const char* RZInputJaguar::getUsbId() {
  return "RZRJag";
}

void RZInputJaguar::configureBcdDeviceVersion() {
  bcd_device_version.platform = BCD_PLAT_JAGUAR;
  bcd_device_version.platform_sub = 0;
}

const char* RZInputJaguar::getDescription() {
  return "Jaguar";
}

void RZInputJaguar::setup() {
  setInputPortCount(input_ports);

  pollInterval = 64;

  for (uint8_t i = 0; i < MAX_USB_OUT; ++i)
    dtype[i] = JAG_DEVICE_NONE;

  for (uint8_t i = 0; i < input_ports; ++i) {
    input_jag_config_t c = input_jag_config[i];
    jag[i] = new JagPort(c.j3_j4, c.j2_j5, c.j1_j6, c.j0_j7, c.b0_b2, c.b1_b3, c.j11_j15, c.j10_j14, c.j9_j13, c.j8_j12);
    jag[i]->begin();
    if (i < MAX_USB_OUT) {
      controller_state_t& frame = inputFrame(i);
      setInputFrameConnected(frame, true);
      frame.HAS_BTN_HOME = 0;
      frame.HAS_BTN_SELECT = 1;
      frame.HAS_BTN_START = 1;
      frame.HAS_ANALOG_STICK_MAIN = 0;
      frame.sticks_precision_bits = ANALOG_STICK_PRECISION_8;
      setInputFrameTypeName(frame, "Jaguar");
    }
  }
}

void RZInputJaguar::setup2() {
  for (uint8_t i = 0; i < input_ports; ++i)
    setPortLed(i, HIGH);
}
