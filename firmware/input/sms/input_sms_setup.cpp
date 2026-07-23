#include "Input_Sms.h"

const input_sms_config_t input_sms_config[] = {
  { .up = HDMI_1_01, .down = HDMI_1_02, .left = HDMI_1_03, .right = HDMI_1_04,
    .tl = HDMI_1_05, .th = HDMI_1_06, .tr = HDMI_1_07 },
  { .up = HDMI_2_01, .down = HDMI_2_02, .left = HDMI_2_03, .right = HDMI_2_04,
    .tl = HDMI_2_05, .th = HDMI_2_06, .tr = HDMI_2_07 },
};

const int8_t RZInputSms::drivingQuadTable[16] = {
   0,  1, -1,  0,
  -1,  0,  0,  1,
   1,  0,  0, -1,
   0, -1,  1,  0
};

RZInputSms::RZInputSms() : RZInputModule() {}

uint8_t RZInputSms::getInternalMode() {
  switch (deviceMode) {
    #if defined(ENABLE_INPUT_SMS)
      case RZORD_SMS: return 0;
    #endif
    #if defined(ENABLE_INPUT_JPC)
      case RZORD_JPC: return 1;
    #endif
    default: return 0;
  }
}

const char* RZInputSms::getUsbId() {
  return isJPC ? "RZRNJPC" : "RZRNSMS";
}

void RZInputSms::configureBcdDeviceVersion() {
  bcd_device_version.platform = isJPC ? BCD_PLAT_JPC : BCD_PLAT_SMS;
  bcd_device_version.platform_sub = 0;
}

const char* RZInputSms::getDescription() {
  return isJPC ? "JPN PC" : "Atari/C64/SMS";
}

void RZInputSms::setup() {
  pollInterval = 0;
  setInputPortCount(input_ports);

  isJPC = (getInternalMode() == 1);

  for (uint8_t p = 0; p < input_ports; ++p) {
    const input_sms_config_t& pins = input_sms_config[p];

    pinMode(pins.up, INPUT_PULLUP);
    pinMode(pins.down, INPUT_PULLUP);
    pinMode(pins.left, INPUT_PULLUP);
    pinMode(pins.right, INPUT_PULLUP);
    pinMode(pins.tl, INPUT_PULLUP);

    if (isJPC) {
      pinMode(pins.th, INPUT_PULLUP);
      pinMode(pins.tr, OUTPUT);
      digitalWrite(pins.tr, LOW);
    } else {
      pinMode(pins.tr, INPUT_PULLUP);
      pinMode(pins.th, INPUT_PULLUP);
    }

    if (p < MAX_USB_OUT) {
      controller_state_t& frame = inputFrame(p);
      setInputFrameConnected(frame, true);
      frame.HAS_BTN_SELECT = isJPC ? 1 : 0;
      frame.HAS_BTN_START = isJPC ? 1 : 0;
      frame.HAS_ANALOG_STICK_MAIN = 0;
      frame.sticks_precision_bits = ANALOG_STICK_PRECISION_8;
      setInputFrameTypeName(frame, isJPC ? "JPN PC" : "Atari/C64/SMS");
    }

    lastState[p] = 0xFF;
    lastDrivingState[p] = (digitalRead(pins.up) ? 1 : 0) << 1 | (digitalRead(pins.down) ? 1 : 0);
    drivingPosition[p] = DRIVING_FALLBACK_CENTER;
    drivingActive[p] = false;
  }
}

void RZInputSms::setup2() {
  for (uint8_t i = 0; i < input_ports; ++i)
    setPortLed(i, HIGH);
}
