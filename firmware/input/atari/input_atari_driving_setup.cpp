#include "Input_AtariDriving.h"

#define DRIVING_CENTER 128

const input_driving_config_t input_driving_config[2] = {
  { .enc_a = HDMI_1_01, .enc_b = HDMI_1_02, .button = HDMI_1_05 },
  { .enc_a = HDMI_2_01, .enc_b = HDMI_2_02, .button = HDMI_2_05 },
};

const int8_t driving_quad_table[16] = {
   0,  1, -1,  0,
  -1,  0,  0,  1,
   1,  0,  0, -1,
   0, -1,  1,  0
};

RZInputDriving::RZInputDriving() : RZInputModule() {}

const char* RZInputDriving::getUsbId() { return "RZRDriv"; }

void RZInputDriving::configureBcdDeviceVersion() {
  bcd_device_version.platform = BCD_PLAT_DRIVING;
  bcd_device_version.platform_sub = 0;
}

const char* RZInputDriving::getDescription() { return "DRIVING CTRL"; }

void RZInputDriving::setup() {
  pollInterval = 100;
  setInputPortCount(input_ports);

  for (uint8_t p = 0; p < input_ports; ++p) {
    const input_driving_config_t& pins = input_driving_config[p];
    pinMode(pins.enc_a, INPUT_PULLUP);
    pinMode(pins.enc_b, INPUT_PULLUP);
    pinMode(pins.button, INPUT_PULLUP);
    lastState[p] = drivingReadEncoderState(pins.enc_a, pins.enc_b);
    lastButton[p] = drivingReadPin(pins.button);
    position[p] = DRIVING_CENTER;

    if (p < MAX_USB_OUT) {
      controller_state_t& frame = inputFrame(p);
      setInputFrameConnected(frame, true);
      frame.sticks_precision_bits = ANALOG_STICK_PRECISION_16;
      setInputFrameTypeName(frame, "Driving");
    }
  }
}

void RZInputDriving::setup2() {
  for (uint8_t i = 0; i < input_ports; ++i)
    setPortLed(i, HIGH);
}

int16_t RZInputDriving::getPosition(uint8_t port) { return (port < input_ports) ? position[port] : 0; }
